#pragma once
#include <Windows.h>
#include <initializer_list>
#include <thread>
#include <chrono>
#include "PPT1Mem.h"
#include "PPTDef.h"

// RunBot から毎フレーム代入
inline int g_input_player_index = 0;

// 低レベル送出
inline bool isExtendedVK(WORD vk){
    switch(vk){
        case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN:
        case VK_HOME: case VK_END: case VK_PRIOR: case VK_NEXT:
        case VK_INSERT: case VK_DELETE: case VK_DIVIDE: case VK_NUMLOCK:
            return true;
        default: return false;
    }
}
inline WORD toScan(WORD vk){ return (WORD)::MapVirtualKeyW(vk, MAPVK_VK_TO_VSC); }
inline void keyDown(WORD vk){
    INPUT in{}; in.type=INPUT_KEYBOARD;
    in.ki.wScan=toScan(vk);
    in.ki.dwFlags=KEYEVENTF_SCANCODE | (isExtendedVK(vk)?KEYEVENTF_EXTENDEDKEY:0);
    ::SendInput(1,&in,sizeof(INPUT));
}
inline void keyUp(WORD vk){
    INPUT in{}; in.type=INPUT_KEYBOARD;
    in.ki.wScan=toScan(vk);
    in.ki.dwFlags=KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP | (isExtendedVK(vk)?KEYEVENTF_EXTENDEDKEY:0);
    ::SendInput(1,&in,sizeof(INPUT));
}

// 押しっぱ→反映検知→離す（同時押しOK）
inline bool inputkey(int playerIndex,
                     std::initializer_list<WORD> keys,
                     int maxFrames = 8,
                     int sleepMs   = 1)
{
    PPTDef::Current before{}, now{};
    PPT1Mem::GetCurrent(playerIndex, before);
    const int  startF  = PPT1Mem::GetMatchFrameCount();
    const auto startPtr= PPT1Mem::Debug_GetCurrentStructAddress(playerIndex);

    bool hasHold=false, hasHard=false, watchX=false, watchY=false, watchRot=false;
    for (WORD k : keys) {
        if (k == 0x56)     hasHold = true;            // 'V' = Hold
        if (k == VK_SPACE) hasHard = true;            // Hard
        if (k == VK_LEFT || k == VK_RIGHT) watchX = true;
        if (k == 0x43 /*'C'*/)            watchY = true;  // Soft は 'C'
        if (k == VK_UP || k == VK_DOWN)   watchRot = true; // 右回転=↑ 左回転=↓
    }

    for (WORD k : keys) keyDown(k);

    bool ok = false;
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));

        // 画面遷移など
        if (PPT1Mem::MemorizeMatchAddress() == 0) break;

        const int  curF   = PPT1Mem::GetMatchFrameCount();
        const auto curPtr = PPT1Mem::Debug_GetCurrentStructAddress(playerIndex);
        PPT1Mem::GetCurrent(playerIndex, now);

        // --- 強い判定: 新ミノに切替（ptr/type/locked）
        if ((curPtr != 0 && curPtr != startPtr) ||
            (now.type   != before.type) ||
            (before.locked == PPTDef::Locked::Yes && now.locked == PPTDef::Locked::No))
        { ok = true; break; }

        // --- 通常の入力反応（移動・回転・ソフト）
        if (!hasHold && !hasHard) {
            if ((watchX  && now.x        != before.x) ||
                (watchY  && now.y        != before.y) ||
                (watchRot&& now.rotation != before.rotation))
            { ok = true; break; }
        }

        // --- ハードは落下=Y 変化 or ロックでも OK
        if (hasHard) {
            if (now.y != before.y || now.locked != before.locked) { ok = true; break; }
        }

        // タイムアウト/フレーム巻き戻り
        if (curF < startF) break;
        if (curF - startF >= maxFrames) break;
    }

    for (WORD k : keys) keyUp(k);
    return ok;
}

// 既存ラッパ
inline void hardDrop(){ (void)inputkey(g_input_player_index, {VK_SPACE}); }
inline void hold()    { (void)inputkey(g_input_player_index, {0x56}); } // 'V'
