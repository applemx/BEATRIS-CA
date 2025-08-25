#pragma once
#include "board.hpp"
#include "block.hpp"

//定数としてのmaskをたくさん定義したいんだよな。

using namespace reachability;
board_t<10, 24> mask_board;

//定数ゾーン

const board_t<10, 24> NOT_FULL_MASK_ALL {
    std::array<std::uint64_t, board_t<10,24>::num_of_under>{
        0x0ULL, 0x0ULL,
        0x0ULL, 0x0ULL
    }
};
//TODO　仕方なく使ってる。使いたくなかった
const board_t<10, 24> NOT_BOTTOM_ROW_MASK{
    std::array<std::uint64_t, board_t<10,24>::num_of_under>{
        0x0FFFFFFFFFFFFC00ULL,
        0x0FFFFFFFFFFFFFFFULL,
        0x0FFFFFFFFFFFFFFFULL,
        0x0FFFFFFFFFFFFFFFULL
    }
};
///全64ビットが1の定数　constで妥協。constexprじゃなくてもまぁ大丈夫と学びました
const board_t<10, 24> FULL_MASK_ALL {
    std::array<std::uint64_t, board_t<10,24>::num_of_under>{
        0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL,
        0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL
    }
};

//下位60ビットが1の定数
const board_t<10, 24> FULL_MASK_USED{
    std::array<std::uint64_t, board_t<10,24>::num_of_under>{
        board_t<10,24>::mask, board_t<10,24>::mask,
        board_t<10,24>::mask, board_t<10,24>::mask
    }
};

const board_t<10, 24> TSPIN09_MASK{
    std::array<std::uint64_t, board_t<10,24>::num_of_under>{0ULL, 0ULL, 0ULL, 0b11ULL<<62}
};

constexpr auto makeTable()
{
    std::array<std::array<board_t<10,24>,24>,10> tbl{};

    using namespace reachability::blocks;

    constexpr int H=24;
    constexpr int W=10;//TODO
    static_for<H>([&][[gnu::always_inline]](auto y){
        static_for<W>([&][[gnu::always_inline]](auto x){
            
            if constexpr ((x == 0 && y == 0)||(x == 9 && y == 0)||(y==23)){
                //何もしない
            }
            else if constexpr (y == 0){
                tbl[x][y] |= TSPIN09_MASK;
                tbl[x][y].template set<x-1  , y+1  >(); // 左下
                tbl[x][y].template set<x+1  , y+1  >(); // 右下
            } else if constexpr (x == 0) {
                //処理としては、0,9用のmaskをかけて、カベ判定を調整する
                //0,9の時は、壁があるので、1個異常でtspinとなる。そのため、あまりの
                //4bitの部分を2bit分1にすることで、後で調整する
                tbl[x][y] |= TSPIN09_MASK;
                tbl[x][y].template set<x+1  , y-1  >(); // 右上
                tbl[x][y].template set<x+1  , y+1  >(); // 右下
            } else if constexpr (x == 9){
                tbl[x][y] |= TSPIN09_MASK;
                tbl[x][y].template set<x-1  , y-1  >(); // 左上
                tbl[x][y].template set<x-1  , y+1  >(); // 左下
            } else {
                tbl[x][y].template set<x-1  , y-1  >(); // 左上
                tbl[x][y].template set<x+1  , y-1  >(); // 右上
                tbl[x][y].template set<x-1  , y+1  >(); // 左下
                tbl[x][y].template set<x+1  , y+1  >(); // 右下
            }
        });
    });
    //妥協してこれ。解決策分かんなかった；；TODO
    static_for<H>([&][[gnu::always_inline]](auto y){
        static_for<W>([&][[gnu::always_inline]](auto x){
            if constexpr (y == 0){
                tbl[x][y] &= NOT_BOTTOM_ROW_MASK;
                tbl[x][y] |= TSPIN09_MASK;
            }
        });
    });

    return tbl;
}

inline const auto tspinMask = makeTable();