#pragma once
#include <Windows.h>
#include <initializer_list>
#include <thread>
#include <chrono>

#include "PPT1Mem.h"
#include "PPTDef.h"

// RunBot から毎フレーム代入（対象プレイヤー）
inline int g_input_player_index = 0;

// --- 低レベル：拡張キー判定 ---
inline bool isExtendedVK(WORD vk){
    switch(vk){
        case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN:
        case VK_HOME: case VK_END: case VK_PRIOR: case VK_NEXT:
        case VK_INSERT: case VK_DELETE: case VK_DIVIDE: case VK_NUMLOCK:
            return true;
        default: return false;
    }
}

// --- サンプル準拠の SendInput（wVk + wScan + SCANCODE / EXTENDED）---
inline void inputKey(WORD vk, bool isExtended, bool isKeyUp){
    INPUT ip{};
    ip.type = INPUT_KEYBOARD;
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;
    ip.ki.wVk   = vk;
    ip.ki.wScan = static_cast<WORD>(::MapVirtualKeyW(ip.ki.wVk, MAPVK_VK_TO_VSC));
    ip.ki.dwFlags = KEYEVENTF_SCANCODE;
    if (isKeyUp)     ip.ki.dwFlags |= KEYEVENTF_KEYUP;
    if (isExtended)  ip.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    ::SendInput(1, &ip, sizeof(INPUT));
}

inline void keyDown(WORD vk){ inputKey(vk, isExtendedVK(vk), false); }
inline void keyUp  (WORD vk){ inputKey(vk, isExtendedVK(vk), true ); }

inline void pressKeys(std::initializer_list<WORD> keys){ for (WORD k: keys) keyDown(k); }
inline void releaseKeys(std::initializer_list<WORD> keys){ for (WORD k: keys) keyUp(k); }

// 入力結果の詳細（ピースが変わったか/ロックしたか）
struct InputResult {
    bool ok            = false; // 何らかの反応を検知
    bool pieceChanged  = false; // ピースが切り替わった（新スポーン/ホールド入替含む）
    bool lockedChanged = false; // lock 状態が変わった（ロック→解除/解除→ロック）
};

// ------------------------------------------------------------
// 押しっぱ → 1フレーム以上待つ → 反応監視 → 同時解放
//   - keys: 同時押しキー
//   - maxFrames: 反応待ち
//   - sleepMs: ポーリング間隔
// ------------------------------------------------------------
inline InputResult inputkey(int playerIndex,
                            std::initializer_list<WORD> keys,
                            int maxFrames = 14,
                            int sleepMs   = 2)
{
    InputResult R{};

    PPTDef::Current before{}, now{};
    PPT1Mem::GetCurrent(playerIndex, before);
    const int  startF   = PPT1Mem::GetMatchFrameCount();
    const auto startPtr = PPT1Mem::Debug_GetCurrentStructAddress(playerIndex);

    bool hasHold=false, hasHard=false, watchX=false, watchY=false, watchRot=false;
    for (WORD k : keys) {
        if (k == 0x56)           hasHold = true;              // 'V' Hold
        if (k == VK_SPACE)       hasHard = true;              // Hard
        if (k == VK_LEFT || k == VK_RIGHT) watchX = true;
        if (k == 0x43 /*'C'*/)   watchY = true;               // Soft/Fast
        if (k == VK_UP || k == VK_DOWN)   watchRot = true;    // 回転
    }

    // 同時押し
    pressKeys(keys);

    // 最低 1F 待機（位相合わせ）
    int firstF = PPT1Mem::GetMatchFrameCount();
    while (PPT1Mem::GetMatchFrameCount() == firstF) {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
        if (PPT1Mem::MemorizeMatchAddress() == 0) break;
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
        if (PPT1Mem::MemorizeMatchAddress() == 0) break;

        const int  curF   = PPT1Mem::GetMatchFrameCount();
        const auto curPtr = PPT1Mem::Debug_GetCurrentStructAddress(playerIndex);
        PPT1Mem::GetCurrent(playerIndex, now);

        // ピース切替（新スポーン/ホールド入替/アンロック→落下開始）
        bool pieceChanged =
            (curPtr != 0 && curPtr != startPtr) ||
            (now.type   != before.type) ||
            (before.locked == PPTDef::Locked::Yes && now.locked == PPTDef::Locked::No);

        bool lockedChanged = (now.locked != before.locked);

        // 強い反応
        if (pieceChanged) { R.ok = true; R.pieceChanged = true; R.lockedChanged = lockedChanged; break; }

        // 通常の入力反応
        if (!hasHold && !hasHard) {
            if ((watchX   && now.x        != before.x) ||
                (watchY   && now.y        != before.y) ||
                (watchRot && now.rotation != before.rotation))
            { R.ok = true; R.pieceChanged = false; R.lockedChanged = lockedChanged; break; }
        }

        // ハードは y or lock の変化で
        if (hasHard) {
            if (now.y != before.y || lockedChanged) { R.ok = true; R.pieceChanged = pieceChanged; R.lockedChanged = lockedChanged; break; }
        }

        if (curF < startF) break;
        if (curF - startF >= maxFrames) break;
    }

    // 一括解放
    releaseKeys(keys);
    return R;
}

// 既存ラッパ
inline void hardDrop(){ (void)inputkey(g_input_player_index, {VK_SPACE}); }
inline void hold()    { (void)inputkey(g_input_player_index, {0x56}); }  // 'V'
inline void softDrop(){ (void)inputkey(g_input_player_index, {0x43}); }  // 'C'
