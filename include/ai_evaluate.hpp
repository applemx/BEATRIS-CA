// my_eval.cpp
#pragma once
#include "block.hpp"
#include "utils.hpp"
#include "board.hpp"
#include "fallmino.hpp"
#include "boardex.hpp"
#include "masks.hpp"
#include <iostream>
#include <cassert>
#include <string>
#include <array>
#include <cstddef>
#include <limits>
#include <string_view>
#include <array>
#include <type_traits>
#include <cstdint>
#include <bit>
#include <experimental/simd>
#include <iostream>

using Board = reachability::board_t<10, 24>;
using namespace reachability;

// Tspinかどうかを判定する関数
constexpr bool is_tspin(Board board, int x, int y) {
    return (board & tspinMask[x][y]).bitcount() >= 3;
}

namespace ai {

namespace {                 // 無名名前空間 = この翻訳単位だけで参照
//------------------------------------------------------------
// ① 評価用の係数 —— まずは経験則値。あとで学習で最適化可能
//------------------------------------------------------------
constexpr int  W_LINES      = +40;   // 消した行数 (n² で掛ける)
constexpr int  W_HOLES      = -20;   // 穴の個数
constexpr int  W_HEIGHT     =  -1;   // 各列高さの合計
constexpr int  W_BUMPINESS  =  -1;   // 隣接列の高さ差
constexpr int  W_WELLS      =  -3;   // 井戸（両隣より低い列）の深さ合計
//------------------------------------------------------------

//------------------------------------------------------------
// ② 高さ・穴数などを 1 パスで算出
//------------------------------------------------------------
template<unsigned W, unsigned H>
struct Stats {
    int height[W]      = {};   // 各列の最上段 y+1 （empty なら 0）
    int holes          = 0;    // 穴 = 列の最初のブロックの下にある空白
    int wellDepthSum   = 0;    // 井戸の深さ（周囲より低いセル）
    int aggHeight      = 0;    // 列高さの合計
    int bumpiness      = 0;    // |h[i]-h[i+1]| の合計

    template<class Board>
    Stats(const Board& bd)
    {
        // 高さと穴を列単位で計算
        for (int x = 0; x < static_cast<int>(W); ++x) {
            bool blockFound = false;
            for (int y = static_cast<int>(H) - 1; y >= 0; --y) {
                if (bd.get(x, y)) {
                    if (!blockFound) {
                        height[x] = y + 1;          // 最上段
                        blockFound = true;
                    }
                } else if (blockFound) {
                    ++holes;                        // ブロックより下の空白
                }
            }
        }

        // 井戸深さ：左右より低いセル数
        for (int x = 0; x < static_cast<int>(W); ++x) {
            int left  = (x == 0          ? H : height[x-1]);
            int right = (x == static_cast<int>(W)-1 ? H : height[x+1]);
            int top   = height[x];
            if (top < left && top < right) {
                wellDepthSum += std::min(left, right) - top;
            }
        }

        // 集約高さ・バンピネス
        for (int x = 0; x < static_cast<int>(W); ++x) {
            aggHeight += height[x];
            if (x+1 < static_cast<int>(W))
                bumpiness += std::abs(height[x] - height[x+1]);
        }
    }
};

//------------------------------------------------------------
// ③ 重み付き合計を返す評価関数
//------------------------------------------------------------
template<unsigned W, unsigned H>
int evaluate(const reachability::board_t<W,H>& board,
             int linesCleared /* board.clear_full_lines() の戻り値 */)
{
    const Stats<W,H> st(board);

    int score =
          W_LINES     * linesCleared * linesCleared   // n 行同時消しを強く評価
        + W_HOLES     * st.holes
        + W_HEIGHT    * st.aggHeight
        + W_BUMPINESS * st.bumpiness
        + W_WELLS     * st.wellDepthSum;

    return score;
}

} // ←無名名前空間終わり

} // namespace ai
