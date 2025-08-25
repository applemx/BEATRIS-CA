#include <Windows.h>
#include <vector>
#include <cstdlib>
#include <thread>
#include <cstdio>
#include <chrono>
#include <string>
#include <span>

#include "../platform/win/PPT1Mem.h"
#include "../platform/win/PPTDef.h"
#include "../platform/win/input.hpp"
#include "board.hpp"
#include "ai_search.hpp"
#include "ai_path.hpp"

using Board = reachability::board_t<10,24>;
static int g_frame = 0;

static char typeToChar(PPTDef::Type t)
{
    using enum PPTDef::Type;
    switch (t) {
    case I: return 'I'; case O: return 'O'; case T: return 'T';
    case S: return 'S'; case Z: return 'Z'; case J: return 'J';
    case L: return 'L'; default: return 0;
    }
}

void RunBot(int playerIndex)
{
    int st = PPT1Mem::MemorizeMatchAddress();
    std::fwprintf(stderr, L"[DBG] MemorizeMatchAddress = %d (0:menu 1:single 2:multi)\n", st);
    std::fwprintf(stderr, L"[DBG] playerMainAddress[%d] = 0x%llX\n",playerIndex, PPT1Mem::Debug_GetPlayerMainAddress(playerIndex));

    // ランタイム状態
    static uint64_t     lastPiecePtr    = 0; // 前回の Current 構造体ポインタ
    static uint64_t     lastExecutedPtr = 0; // 入力を送ったピースのポインタ
    static PPTDef::Type   lastType      = PPTDef::Type::Nothing;
    static PPTDef::Locked lastLocked    = PPTDef::Locked::No;
    static char           hold_slot     = 0; // 実ホールド内容（0=空）

    int  prevFrame     = -1;
    bool warnedNoPiece = false;

    while (true)
    {
        if (PPT1Mem::MemorizeMatchAddress() == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            prevFrame     = -1;
            warnedNoPiece = false;
            lastPiecePtr  = 0;
            lastExecutedPtr = 0;
            lastType      = PPTDef::Type::Nothing;
            lastLocked    = PPTDef::Locked::No;
            continue;
        }

        int frame = PPT1Mem::GetMatchFrameCount();
        if (frame == prevFrame) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        prevFrame = frame;
        g_frame   = frame;

        // input.hpp のターゲットを毎フレーム共有
        g_input_player_index = playerIndex;

        PPTDef::Field_t field;
        PPTDef::Current cur;
        PPTDef::Next_t  next;
        PPT1Mem::GetField  (playerIndex, field);
        PPT1Mem::GetCurrent(playerIndex, cur);
        PPT1Mem::GetNext   (playerIndex, next);

        uint64_t curPtr = PPT1Mem::Debug_GetCurrentStructAddress(playerIndex);

        if (cur.type == PPTDef::Type::Nothing) {
            if (!warnedNoPiece) {
                std::fwprintf(stderr, L"[WARN] F%04d: 落下ミノが検出できません (player=%d)\n", frame, playerIndex);
                warnedNoPiece = true;
            }
            lastPiecePtr = 0;
            continue;
        }
        warnedNoPiece = false;

        // --- 新ミノ判定（ptr/type/locked のいずれか変化）---
        bool ptrChanged  = (curPtr != 0 && curPtr != lastPiecePtr);
        bool typeChanged = (lastType != PPTDef::Type::Nothing && cur.type != lastType);
        bool unlockSpawn = (lastLocked == PPTDef::Locked::Yes && cur.locked == PPTDef::Locked::No);
        bool isNewPiece  = ptrChanged || typeChanged || unlockSpawn;

        // 同一ピースに 2 度送らない
        if (!isNewPiece && curPtr == lastExecutedPtr) {
            lastType   = cur.type;
            lastLocked = cur.locked;
            continue;
        }

        // --- Board を構築 ---
        Board board;
        for (int x = 0; x < 10; ++x)
            for (int y = 0; y < 24; ++y)
                if (field[x][y] != PPTDef::Type::Nothing && field[x][y] != PPTDef::Type::Clearing)
                    board.set(x, y);

        // --- 実キュー（current + next）---
        std::vector<char> queue;
        if (char c0 = typeToChar(cur.type); c0) queue.push_back(c0);
        for (int i = 0; i < PPTDef::NEXT_NUM; ++i)
            if (char c = typeToChar(next[i]); c) queue.push_back(c);

        // --- 仮想キュー（ホールド反映）---
        std::vector<char> vq = queue;
        if (hold_slot != 0 && !vq.empty())
            vq.insert(vq.begin() + 1, hold_slot);

        // --- 探索 ---
        ai::Node res = ai::search(board, std::span(vq.data(), vq.size()));
        if (res.path.empty()) {
            std::fwprintf(stderr, L"[WARN] F%04d: 探索失敗 (詰み?)\n", frame);
            hardDrop(); // 苦し紛れ
            // 次は新ミノ扱い
            lastPiecePtr = 0; lastExecutedPtr = 0;
            lastType = PPTDef::Type::Nothing; lastLocked = PPTDef::Locked::No;
            continue;
        }
        const ai::Step& mv = res.path[0];

        // --- 入力トークン列 ---
        auto tokens = ai::build_input_path(board, mv, {});
        if (tokens.empty()) {
            hardDrop();
            lastPiecePtr = 0; lastExecutedPtr = 0;
            lastType = PPTDef::Type::Nothing; lastLocked = PPTDef::Locked::No;
            continue;
        }

        // デバッグ
        std::fwprintf(stderr, L"-----------------------------\n");
        std::fwprintf(stderr, L"[PLAN] F%04d piece=%lc usedHold=%lc dst=(%d,%d,r%d) tokens=%zu\n",
                      frame, typeToChar(cur.type),
                      mv.usedHold ? L'Y' : L'N',
                      int(mv.x), int(mv.y), int(mv.rot),
                      tokens.size());
        for (const auto& t : tokens)
            std::fwprintf(stderr, L"[DBG] F%04d token=%s\n", frame, t.c_str());
        std::fwprintf(stderr, L"-----------------------------\n");

        // VK マッピング
        auto isMove = [](const std::string& t){ return t=="left" || t=="right"; };
        auto isRot  = [](const std::string& t){ return t=="cw"   || t=="ccw";   };
        auto mapVK  = [](const std::string& t, WORD& vk)->bool{
            if (t=="left")  { vk = VK_LEFT;  return true; }
            if (t=="right") { vk = VK_RIGHT; return true; }
            if (t=="soft")  { vk = 0x43;     return true; } // 'C' Fast/Soft Drop
            if (t=="cw")    { vk = VK_UP;    return true; } // Rotate Right = ↑
            if (t=="ccw")   { vk = VK_DOWN;  return true; } // Rotate Left  = ↓
            return false;
        };

        bool didHard = false;

        for (size_t i = 0; i < tokens.size(); ++i) {
            const std::string& t = tokens[i];

            if (t == "hold") { hold();  continue; }
            if (t == "hard") {
                hardDrop();
                didHard = true;
                break; // ピース確定
            }

            WORD vk1=0, vk2=0;
            if (!mapVK(t, vk1)) continue;

            // 移動＋回転は 1F 同時押し
            bool combined = false;
            if (i+1 < tokens.size() && ((isMove(t) && isRot(tokens[i+1])) || (isRot(t) && isMove(tokens[i+1])))) {
                if (mapVK(tokens[i+1], vk2)) combined = true;
            }

            if (combined) { inputkey(playerIndex, {vk1, vk2}); ++i; }
            else          { inputkey(playerIndex, {vk1}); }

            std::fprintf(stderr, "[DBG] F%04d: inputkey token=%s vk1=0x%04X vk2=0x%04X\n",
                         frame, t.c_str(), vk1, vk2);
        }

        // ホールド内容のローカル更新
        if (mv.usedHold) {
            char q0 = (!queue.empty()) ? queue[0] : 0;
            hold_slot = q0;
        }

        if (didHard) {
            // 次は必ず新ミノ扱い（同一ポインタ再利用でも進む）
            lastPiecePtr = 0; lastExecutedPtr = 0;
            lastType = PPTDef::Type::Nothing; lastLocked = PPTDef::Locked::No;
            continue;
        }

        // このピースには送信済み
        lastExecutedPtr = curPtr;
        // 次フレーム判定用
        lastPiecePtr = curPtr;
        lastType     = cur.type;
        lastLocked   = cur.locked;
    }
}
