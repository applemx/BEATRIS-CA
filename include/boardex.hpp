#pragma once
#include <array>
#include <cstring>   // std::memsetなど
#include "masks.hpp"
#include "board.hpp"
#include "block.hpp"
#include "fallmino.hpp"

namespace reachability{
    /// BoardEx: board_t<W,H> を内包し、HoikoのTetriz_Common.h 相当の
    /// 機能 (衝突判定, ミノ固定, ライン消去, T-Spin判定など) を追加する.

template <unsigned W, unsigned H>
struct BoardEx: public board_t<W, H>{
public:
    using BoardType = board_t<W,H>;
    BoardType board;
    ///コンストラクタ
    BoardEx(){
        init();
    }
    void init() {
        // board_tはデフォルトコンストラクタで0クリア
        board = BoardType();
    }

    /// ミノを盤面にスポーンさせる
    /// 盤面に出現させる (衝突チェック含む).
    /// 衝突がひどくて出せなければ 0, そうでなければ1など返す
    template <typename RS>
    int Spawn(const FallMino<RS,W,H> &mino) {
        
        if (collision(mino)) {
            return 0; // 衝突
        }
        return 1; // 出現成功
    }


    ///衝突判定-mino の4ブロックが既存ブロックか枠外に当たるか
    template <typename RS>
    bool collision(const FallMino<RS,W,H> &mino) const {
        bool isCollision = false;
        // block.hpp の call_with_block<RS>(...) でミノ形状を取得
        blocks::call_with_block<RS>(mino.block_type, [&]<auto B>{
            // B は  block<shapes,orientations,rotations,...> の具象
            auto shapeIndex = B.mino_index[mino.rotation]; // 回転状態に対応するindex
            const auto &coords = B.minos[shapeIndex]; // 4つの相対座標
            
            if ((board & FULL_MASK_USED) != board) {
                isCollision = true;
                return;
            }
            for (const auto &c : coords) {
                int x = mino.pos[0] + c[0];
                int y = mino.pos[1] + c[1];
                
                // そのセルが既に埋まっている? setを使わない256の用意の仕方
                //  board.get(x,y) が 1ならブロック、0なら空、2なら範囲外(実装次第)
                if (board.get(x,y) == 1) {
                    isCollision = true;
                    return;
                }
                else if (board.get(x,y) == 2) {
                    isCollision = true;
                    return;
                }
            }
        });
        return isCollision;
    }

    template <typename RS>
    void lock(const FallMino<RS,W,H> &mino) {
        blocks::call_with_block<RS>(mino.block_type, [&]<auto B>{
            auto shapeIndex = B.mino_index[mino.rotation];
            const auto &coords = B.minos[shapeIndex];
            for (const auto &c : coords) {
                int x = mino.pos[0] + c[0];
                int y = mino.pos[1] + c[1];
                board.set(x,y);
            }
        });
    }

    template <typename RS>
    bool hard_drop(const FallMino<RS,W,H> &mino) {
        FallMino<RS,W,H> temp = mino;
        if (collision<RS>(temp)) {
            return false;
        }
        while (!collision<RS>(temp)) {
            temp.pos[1]--;
        }
        temp.pos[1]++;
        lock<RS>(temp);
        return true;
    }

    
};
}