#pragma once
#include <iostream>
#include "board.hpp"
#include "block.hpp"
#include "findfinalattachableminostates.hpp"


namespace reachability{
template <typename RS,unsigned W,unsigned H> //kick種類,ブロックの種類
struct FallMino{
    static constexpr int SPAWN_X = 4;
    static constexpr int SPAWN_Y = 19;
    
    char block_type;//ブロックの種類

    using coord = std::array<int, 2>;//ブロックの座標ｘｙをstd::array<int, 2> で表現
    coord pos;//ブロックの座標 プリミティブがたにしたい TODO いや、コンパイラに-o2で渡してるから問題なさそう
    int rotation;//ブロックの回転状態(0..3)
    int lastSrs;//最後にSRSを適用した回転状態
    int bugGravity;//重力適用のバグ修正用

    ///コンストラクタ
    FallMino(char block){
        block_type = block;
        pos = {SPAWN_X, SPAWN_Y};
        rotation = 0;
        lastSrs = -1;
        bugGravity = 0;
    }

    /// ミノ情報を初期化 (ホールド時リセット用など)
    void init() {
        pos = {SPAWN_X, SPAWN_Y};
        rotation = 0;
        lastSrs = -1;
        bugGravity = 0;
    }

    // 左へ移動
    void moveLeft() {
      --pos[0];
    }

    // 右へ移動
    void moveRight() {
      ++pos[0];
    }

    // 下へ移動
    void moveDown() {
      --pos[1];
    }

    // 時計回りに回転
    void rotateCW() {
      rotation = (rotation + 1) % 4;
    }

    // 反時計回りに回転
    void rotateCCW() {
      rotation = (rotation + 3) % 4;
    }

};
}
