#ifndef DRAW_HPP
#define DRAW_HPP

#include "boardex.hpp"
#include "fallmino.hpp"
#include "board.hpp"
#include "findfinalattachableminostates.hpp"
#include "masks.hpp"
#include <iostream>

using namespace reachability;
using RS = reachability::blocks::SRS;

// 描画処理：盤面の状態と落下中のミノを合成して描画する関数
inline void drawBoard(const BoardEx<10,24>& boardEx, const FallMino<RS,10,24>& currentMino) {
    BoardEx<10,24> displayBoard;
    displayBoard.board = boardEx.board;  // 既存のブロックをコピー

    // 現在のミノのブロックを一時的に描画
    blocks::call_with_block<RS>(currentMino.block_type, [&]<auto B> {
        auto shapeIndex = B.mino_index[currentMino.rotation];
        const auto &coords = B.minos[shapeIndex];
        for (const auto &c : coords) {
            int x = currentMino.pos[0] + c[0];
            int y = currentMino.pos[1] + c[1];
            if (boardEx.board.get(x, y) == 0) {
                displayBoard.board.set(x, y);
            }
        }
    });
    
    // 盤面を文字列に変換して出力
    std::cout << to_string(displayBoard.board) << std::endl;
}

#endif // DRAW_HPP