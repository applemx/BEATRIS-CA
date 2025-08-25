// test_board.cpp
#include "../include/board.hpp"
#include "./test_utils.hpp"
#include "../include/findfinalattachableminostates.hpp"
#include "../include/fallmino.hpp"
#include "../include/boardex.hpp"
#include "../include/masks.hpp"
#include "../include/block.hpp"
#include "../include/ai_evaluate.hpp"
#include "../include/ai_search.hpp"
//#define AI_SEARCH_DEBUG
#include "../include/ai_common.hpp"
#include "../include/draw.hpp"
#include "../include/ai_path.hpp"
#include <iostream>
#include <span>
#include <cassert>
#include <string>
#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>
#include <x86intrin.h>

// board.hpp 内では、ブロックの座標型は reachability::coord（std::array<int,2> として定義）
// 盤面型の型エイリアスを作成（幅10、高さ24 とする例）
using Board = reachability::board_t<10, 24>;
using namespace reachability;

// 各テスト項目を個別の関数に分割

// 1. 初期状態のテスト：盤面は全セル空であるはず（get() が 0 を返す）
void test_initial_state() {
    Board board;
    // 代表的な座標だけチェック（テンプレート引数はリテラルで指定する必要があるため）
    assert((board.template get<0, 0>() == 0));
    assert((board.template get<3, 5>() == 0));
    // 例えば x==10 は範囲外の場合（実際はテンプレート引数で負の数などは使いにくいので省略）
    std::cout << "Test: initial board is empty passed.\n";
}

// 2. set()/get() のテスト：(3,5) にブロックをセットして確認
void test_set_get() {
    Board board;
    board.template set<3, 5>();
    assert((board.template get<3, 5>() == 1));
    std::cout << "Test: set()/get() passed.\n";
}

// 3. operator~ のテスト：セット済みのセルを反転すれば 0 になるはず
void test_operator_not() {
    Board board;
    board.template set<3, 5>();
    Board board_not = ~board;
    assert((board_not.template get<3, 5>() == 0));
    std::cout << "Test: operator~ passed.\n";
}

// 4. move() のテスト：右方向へ 1 マス移動する例
void test_move() {
    Board board;
    board.template set<3, 5>();
    // move() はテンプレート非型引数として coord を受け取る前提
    Board board_moved = board.template move<reachability::coord{{1, 0}}>();
    // (3,5) にあったブロックは、右に 1 移動して (4,5) に現れるはず
    assert((board_moved.template get<4, 5>() == 1));
    std::cout << "Test: move() passed.\n";
}

// 5. to_string() のテスト：盤面の文字列表現を表示
void test_to_string() {
    Board board;
    board.template set<3, 5>();
    std::string s = to_string(board);
    std::cout << "Board representation:\n" << s << "\n";
    std::cout << "Test: to_string() passed.\n";
}

// 6. find_final_attachable_mino_states() のテスト
void test_findfinalattachableminostates(){
    constexpr auto lzt = merge_str({
        "       X  ",
        "       X  ",
        "   X   XX ",
        "        X ",
        "        X ",
        "    X   XX",
        "         X",
        "         X",
        "     X   X",
        "          ",
        "          ",
        "      X   ",
        "          ",
        "          ",
        "       X  ",
        "          ",
        "          ",
        "          "
      });
    // lztを盤面として使用
    Board board(lzt);
    
    using namespace reachability::search;
    using RS = reachability::blocks::SRS;  // SRS回転システムを使用
    
    // 開始位置（盤面の上部中央）
    constexpr reachability::coord start_pos{{4, 20}};
    
    // Tミノの落下位置を計算
    auto result = binary_bfs<RS, start_pos>(board, 'T');
    
    // 結果の検証と表示(上右下左4つ分作成)
    // 0: 初期状態（スポーン時の向き）
    // 1: 右回転（時計回りに90°）
    // 2: 180°回転（逆さま）
    // 3: 左回転（時計回りに270°、または反時計回りに90°）

    std::cout << "Found " << result.size() << " possible final positions for T block\n";
    // 結果の座標を詳細に表示
    if (result.size() > 0) {
        std::cout << "Details of found positions:\n";
        for (size_t i = 0; i < result.size() && i < 4; ++i) { 
            // 元の盤面と結果を比較表示
            std::cout << "\nPosition " << (i+1) << ":\n";
            
            // 元の盤面
            std::cout << "Original board:\n" << to_string(board) << "\n";
            
            // 配置後の盤面
            std::cout << "After placement:\n" << to_string(result[i]) << "\n";
            
            // 比較ビュー（[]が新しく配置されたブロック、..が元々あるブロック）
            std::cout << "Comparison view:\n" << to_string(board, result[i]) << "\n";
        }
    }
    std::cout << "Test: find_final_attachable_mino_states() passed.\n";
}

void test_block_t(){
    auto t_block = reachability::blocks::SRS::T;
    int current_orientation = 0;
    // rotation_type: 0 を時計回り (cw)、1 を反時計回り (ccw) としている想定
    int rotation_type = 0; 

    int next_orientation = decltype(t_block)::rotation_target(current_orientation, rotation_type);

    std::cout << "Tブロックを向き " << current_orientation 
              << " から " << next_orientation << " に回転します。\n";

    // 現在の T ブロックのミノ座標を表示
    std::cout << "現在の座標: ";
    for (const auto& pos : t_block.minos[current_orientation]) {
        std::cout << "(" << pos[0] << ", " << pos[1] << ") ";
    }
    std::cout << "\n";

     // 回転後の T ブロックのミノ座標を表示
     std::cout << "回転後の座標: ";
     for (const auto& pos : t_block.minos[next_orientation]) {
         std::cout << "(" << pos[0] << ", " << pos[1] << ") ";
     }
     std::cout << "\n";
 
     // 回転時に適用するキックオフセットの例（ここでは最初の補正値を表示）
     std::cout << "適用するキックオフセット: ";
     for (const auto& offset : t_block.kicks[current_orientation][rotation_type]) {
         std::cout << "(" << offset[0] << ", " << offset[1] << ") ";
     }
     std::cout << "\n";
    
}

void test_spawn_mino() {
    // 盤面用の文字列を結合
    constexpr auto lzt = merge_str({
        "       X  ",
        "       X  ",
        "   X   XX ",
        "        X ",
        "        X ",
        "    X   XX",
        "         X",
        "         X",
        "     X   X",
        "          ",
        "          ",
        "      X   ",
        "          ",
        "          ",
        "       X  ",
        "          ",
        "          ",
        "          "
    });

    // Board（幅10, 高さ24）を生成
    
    BoardEx<10, 24> boardex;
    boardex.init();

    // 回転システムとして SRS を使用
    using RS = reachability::blocks::SRS;

    // テストするミノの種類（7種）
    const char minoTypes[7] = {'T', 'Z', 'S', 'J', 'L', 'O', 'I'};

    for (char type : minoTypes) {
        // ミノを生成（各ミノはスポーン時に盤面との衝突判定を行う）
        FallMino<RS, 10, 24> mino(type);
        std::cout << "Testing spawn for mino type '" << type << "':\n";
        std::cout << "[Before Spawn] Board:\n" << to_string(boardex.board) << "\n";
        
        int spawnResult = boardex.template Spawn<RS>(mino);
        if (spawnResult == 1) {
            std::cout << "Spawn success. Mino pos = (" << mino.pos[0] << ", " << mino.pos[1] << ")\n";
        } else {
            std::cout << "Spawn fail. (spawnResult = " << to_string(boardex.board) << ")\n";
        }
        std::cout << "--------------------------------\n";
    }
}

// language: cpp
// filepath: /root/projects/BEATRIS-C/test/test_board.cpp
void test_stringto256() {
    // テスト用の盤面データ（文字列表現）
    constexpr auto test_board = merge_str({
        "          ",
        "          ",
        "          ",
        "          ",
        "          ",
        "          ",
        "          ",
        "          ",
        "          ",
        "          ",
        "          ",
        "          ",
        "          ",
        "          ",
        "          ",
        "          ",
        "          ",
        "          ",
        "          ",
        "          ",
        "X         ",
        "X         ",
        "X         ",
        "X         "
    });
    
    // 文字列から盤面データを生成
    Board board(test_board);
    
    // 盤面を表示
    std::cout << "Original board representation:\n" << to_string(board) << "\n";
    
    // セルごとに状態を調べて内部ビット表現を再構築する
    // これにより内部データにアクセスせずに256ビットのビットパターンを表示できる
    std::cout << "Internal 256-bit representation (4 x 64bit):\n";
    
    std::array<uint64_t, Board::num_of_under> manual_bits = {};
    
    // 各セルを調べて手動でビットパターンを構築
    for (int y = 0; y < Board::height; ++y) {
        for (int x = 0; x < Board::width; ++x) {
            // セルの状態を取得(1=ブロックあり、0=なし)
            if (board.get(x, y) == 1) {
                // このセルに対応するbitを計算
                int under_index = y / Board::lines_per_under;
                int bit_pos = (y % Board::lines_per_under) * Board::width + x;
                manual_bits[under_index] |= (uint64_t(1) << bit_pos);
            }
        }
    }
    
    // 16進数で表示
    for (size_t i = 0; i < Board::num_of_under; ++i) {
        std::cout << "Block " << i << ": 0x" 
                  << std::hex << std::uppercase << std::setfill('0') << std::setw(16)
                  << manual_bits[i] << std::dec << "\n";
    }
    
    // バイナリ表示
    std::cout << "\nBinary representation:\n";
    for (size_t i = 0; i < Board::num_of_under; ++i) {
        std::cout << "Block " << i << ": ";
        uint64_t val = manual_bits[i];
        
        // 各ビットを表示（グループ分けして見やすく）
        for (int byte = 0; byte < 8; ++byte) {
            if (byte > 0) std::cout << " ";
            for (int bit = 0; bit < 8; ++bit) {
                int pos = byte * 8 + bit;
                std::cout << ((val >> pos) & 1);
            }
        }
        std::cout << "\n";
    }
    
    // 追加表示: 行ごとの連続ビット表示（テトリス盤面として見やすい形式）
    std::cout << "\nBoard layout (rows x columns):\n";
    for (int y = 0; y < Board::height; ++y) {
        std::cout << "Row " << std::setw(2) << std::setfill('0') << y << ": ";
        for (int x = 0; x < Board::width; ++x) {
            std::cout << (board.get(x, y) == 1 ? "1" : "0");
            
            // 5列ごとに空白を入れて見やすく
            if ((x + 1) % 5 == 0 && x < Board::width - 1)
                std::cout << " ";
        }
        std::cout << "\n";
    }
}

// language: cpp
// filepath: /root/projects/BEATRIS-C/test/test_board.cpp
// 一連のフロー (Spawn → collision → lock) をテストする関数
void test_spawn_lock_flow() {
    using RS = reachability::blocks::SRS;  // 回転システム
    
    // テスト用の盤面データ (文字列表現)
    constexpr auto test_board_str = merge_str({
        "          ",
        "          ",
        "          ",
        "          ",
        "          ",
        "          ",
        "       X  ",
        "       X  ",
        "   X   XX ",
        "        X ",
        "        X ",
        "    X   XX",
        "         X",
        "         X",
        "     X   X",
        "          ",
        "          ",
        "          ",
        "          ",
        "X         ",
        "X         ",
        "X         ",
        "X         ",
        "          "
    });

    // 盤面を生成し初期化
    BoardEx<10, 24> boardex;
    boardex.init();
    boardex.board = Board(test_board_str);

    // 結果を表示
    std::cout << "board after spawn/lock:\n" 
              << to_string(boardex.board) << "\n";

    // テストするミノを生成
    char testBlockType = 'T';
    FallMino<RS, 10, 24> mino(testBlockType);

    // 1. Spawnを呼び出す（衝突チェックも内部で実施）
    int spawnResult = boardex.template Spawn<RS>(mino);
    if (spawnResult == 0) {
        std::cout << "[Spawn Failed] for block type " << testBlockType << "\n";
        return;
    }
    // 2. 衝突判定の確認
    bool collide = boardex.template collision<RS>(mino);
    std::cout << "Collision after spawn? " << (collide ? "Yes" : "No") << "\n";

    // 3. lock (衝突していないならロックして盤面にセット)
    if (!collide) {
        boardex.template lock<RS>(mino);
        std::cout << "[Lock] Mino locked on board.\n";
    }

    // 結果を表示
    std::cout << "Final board after spawn/lock:\n" 
              << to_string(boardex.board) << "\n";
}

void print_tspinMask(int x, int y)
{
    if (x < 0 || x >= 10 || y < 0 || y >= 24) {
        std::cerr << "print_tspinMask: (x,y) out of range\n";
        return;
    }
    std::cout << "tspinMask[" << x << "][" << y << "]\n";
    std::cout << to_string(tspinMask[x][y]);
    dump_blocks(tspinMask[x][y]);
    std::cout << '\n';
}   

void print_tspinMask_all()
{
    for (int x = 0; x < 10; ++x) {
        for (int y = 0; y < 24; ++y) {
            std::cout << "tspinMask[" << x << "][" << y << "]\n";
            std::cout << to_string(tspinMask[x][y]);
            dump_blocks(tspinMask[x][y]);
            std::cout << '\n';
        }
    }

}   

void test_tspinmaskmove(){

    using clk = std::chrono::steady_clock;
    constexpr auto lzt = merge_str({
        "       X  ",
        "       X  ",
        "   X   XX ",
        "        X ",
        "        X ",
        "    X   XX",
        "         X",
        "         X",
        "     X   X",
        "          ",
        "          ",
        "      X   ",
        "          ",
        "          ",
        "       X  ",
        "          ",
        "          ",
        "          "
    });
    constexpr auto dt = merge_str({
        "        XX",
        "XXXX  XXXX",
        "XXX   XXXX",
        "XXX XXXXXX",
        "XXX  XXXXX",
        "XX   XXXXX",
        "XXX XXXXXX",
        "XXX XXXXXX",
        "XXXX XXXXX"
    });

    // dtを盤面として使用
    Board board(dt);
    
    using namespace reachability::search;
    using RS = reachability::blocks::SRS;  // SRS回転システムを使用
    
    // 開始位置（盤面の上部中央）
    constexpr reachability::coord start_pos{{4, 20}};
    
    // Tミノの落下位置を計算
    auto all1 = clk::now();
    char block_type = 'T';
    auto tbinary1 = clk::now();
    auto result = binary_bfs<RS, start_pos>(board, block_type);
    auto tbinary2 = clk::now();

    auto tbinary3 = clk::now();
    auto result1 = binary_bfs<RS, start_pos>(board, block_type);
    auto tbinary4 = clk::now();
    
    // 結果の検証と表示(上右下左4つ分作成) 
    // 0: 初期状態（スポーン時の向き）
    // 1: 右回転（時計回りに90°）
    // 2: 180°回転（逆さま）
    // 3: 左回転（時計回りに270°、または反時計回りに90°）



    auto t0 = clk::now();
    
    static_for<4>([&](auto rot_const){
        constexpr std::size_t ROT = rot_const;

        //std::cout << "After placement (rotation " << ROT << "):\n"
        //           << to_string(result[ROT]) << "\n";
    
        int move_idx = 0;
        result[ROT].list_bits_256([&](uint8_t x, uint8_t y){
            //std::cout << "  Move " << move_idx++
            //          << ": (" << int(x) << ", " << int(y) << ")\n";
            if (is_tspin(board, x, y))
            {
                 //std::cout << "    T‑spin detected!\n";
            }
               
    
            Board blk  = Board::make_piece_board<ROT, RS>(x, y, 'T');   // ← ここも ROT
            Board next = board | blk;
    
            //std::cout << "    Next board:\n" << to_string(next) << "\n";
        });
    
        //std::cout << "rot " << ROT << ": " << move_idx << " moves\n"
        //          << "---------------------------\n";
    });
    auto all2 = clk::now();
    auto t1 = clk::now();
    auto t2 = clk::now();
    auto t3 = clk::now();
    is_tspin(board, 0, 0);
    is_tspin(board, 0, 0);
    is_tspin(board, 0, 0);
    auto t4 = clk::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    auto ns2 = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    auto ns3 = std::chrono::duration_cast<std::chrono::nanoseconds>(t4 - t3).count();
    auto ns4 = std::chrono::duration_cast<std::chrono::nanoseconds>(tbinary2 - tbinary1).count();
    auto ns5 = std::chrono::duration_cast<std::chrono::nanoseconds>(tbinary4 - tbinary3).count();
    auto all_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(all2 - all1).count();


    std::cout << "[chrono] elapsed = " << ns << " ns\n";
    std::cout << "[chrono] elapsed2 = " << ns2 << " ns\n";
    std::cout << "[chrono] elapsed3 = " << ns3 << " ns\n";
    std::cout << "[chrono] tbinary = " << ns4 << " ns\n";
    std::cout << "[chrono] tbinary2 = " << ns5 << " ns\n";
    std::cout << "[chrono] all = " << all_ns << " ns\n";


    

}

void test_tspinmaskmove2(){
    using clk = std::chrono::steady_clock;
    constexpr auto dt = merge_str({
        "        XX",
        "XXXX  XXXX",
        "XXX   XXXX",
        "XXX XXXXXX",
        "XXX  XXXXX",
        "XX   XXXXX",
        "XXX XXXXXX",
        "XXX XXXXXX",
        "XXXX XXXXX"
    });
    Board board(dt);
    using namespace reachability::search;
    using RS = reachability::blocks::SRS;
    constexpr reachability::coord start_pos{{4, 20}};
    constexpr int N = 5;
    constexpr char block_types[] = {'T','Z','S','J','L','O','I'};

    for(char block_type : block_types){
        std::array<long long, N> a_elapsed, a_elapsed2, a_elapsed3;
        std::array<long long, N> a_tbinary1, a_tbinary2, a_all;

        for(int i = 0; i < N; ++i){
            // elapsed2 (t1 - t0)
            auto t0 = clk::now();
            auto t1 = clk::now();
            // elapsed  (t2 - t1)
            auto t2 = clk::now();

            // tbinary1: 初回 binary_bfs
            auto tb0 = clk::now();
            auto result = binary_bfs<RS, start_pos>(board, block_type);
            auto tb1 = clk::now();

            // tbinary2: ２回目 binary_bfs
            auto tb2 = clk::now();
            auto result2 = binary_bfs<RS, start_pos>(board, block_type);
            auto tb3 = clk::now();

            // all: list_bits_256～make_piece_board～OR 合成を含む全処理
            auto all0 = clk::now();
            static_for<4>([&](auto rot_const){
                constexpr std::size_t ROT = rot_const;
                result[ROT].list_bits_256([&](uint8_t x, uint8_t y){
                    is_tspin(board, x, y);
                    Board blk  = Board::make_piece_board<ROT, RS>(x, y, block_type);
                    Board next = board | blk;
                });
            });
            auto all1 = clk::now();

            // 計測値をナノ秒で保存
            a_elapsed [i] = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
            a_elapsed2[i] = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
            a_elapsed3[i] = std::chrono::duration_cast<std::chrono::nanoseconds>(tb3 - tb2).count();
            a_tbinary1[i] = std::chrono::duration_cast<std::chrono::nanoseconds>(tb1 - tb0).count();
            a_tbinary2[i] = std::chrono::duration_cast<std::chrono::nanoseconds>(tb3 - tb2).count();
            a_all     [i] = std::chrono::duration_cast<std::chrono::nanoseconds>(all1 - all0).count();
        }

        // 結果出力
        std::cout << "=== Block " << block_type << " ===\n";
        auto print = [&](const char* name, auto& arr){
            std::cout << name;
            for(auto v: arr) std::cout << ' ' << v;
            std::cout << " ns\n";
        };
        print("elapsed   :", a_elapsed);
        print("elapsed2  :", a_elapsed2);
        print("elapsed3  :", a_elapsed3);
        print("tbinary1  :", a_tbinary1);
        print("tbinary2  :", a_tbinary2);
        print("all       :", a_all);
        std::cout << "\n";
    }
}


void test_line_clear()
{
    std::cout << "=== test_line_clear ===\n";
    constexpr auto full_dt = merge_str({
        "XXXXXXXXXX","XXXXXXXXXX","XXXXXXXXXX","XXXXXXXXXX",
        "XXXXXXXXXX","XXXXXXXXXX","XXXXXXXXXX","XXXXXXXXXX",
        "XXXXXXXXXX","XXXXXXXXXX","XXXXXXXXXX","XXXXXXXXXX",
        "XXXXXXXXXX","XXXXXXXXXX","XXXXXXXXXX","XXXXXXXXXX",
        "XXXXXXXXXX","XXXXXXXXXX","XXXXXXXXXX","XXXXXXXXXX",
        "XXXXXXXXXX","XXXXXXXXXX","XXXXXXXXXX","XXXXXXXXXX"
    });
    constexpr auto mini_dt = merge_str({
        "000000X000","00000X0000","0000000000","0000000000",
        "XXXXXXXXXX","XXXX0XXXXX","XXXXXXXXXX","XXXXXXXXXX"
        "0000X00000","0000X00000","00000X0000","00000X0000"
        "00000X0000","00000X0000","00000X0000","00000X0000",
    });
    constexpr auto mini_dt2 = merge_str({
        "X000000000","0X00000000","00X0000000","000X000000",
        "0000X00000","XXXXXXXXXX","00000X0000","000000X000",
        "0000000X00","00000000X0","000000000X","XXXXXXXXXX",
        "X000000000","0X00000000","00X0000000","000X000000",
        "XXXXXXXXXX","0000X00000","00000X0000","000000X000",
        "XXXXXXXXXX","0000000X00","00000000X0","000000000X"
    });
    constexpr auto mini_dt3 = merge_str({
        "0XXXXXXXXX","X0XXXXXXXX","XX0XXXXXXX","XXX0XXXXXX",
        "XXXX0XXXXX","XXXXXXXXXX","XXXXX0XXXX","XXXXXX0XXX",
        "XXXXXXX0XX","XXXXXXXX0X","XXXXXXXXX0","XXXXXXXXXX",
        "0XXXXXXXXX","X0XXXXXXXX","XX0XXXXXXX","XXX0XXXXXX",
        "XXXXXXXXXX","XXXX0XXXXX","XXXXX0XXXX","XXXXXX0XXX",
        "XXXXXXXXXX","XXXXXXX0XX","XXXXXXXX0X","XXXXXXXXX0"
    });
    constexpr auto random_1 = merge_str({
        "X0XX0XXX0X", "XXXX0XXXXX", "XX0XXX0XXX", "0XXXXXXXXX",
        "XXXXXX00XX", "XXXXX0XXXX", "X0XXXXXXXX", "XXXXXXXX0X",
        "XXX0XXX0XX", "X0XX0XXXXX", "XXXX000XXX", "XXXXXXXXXX",
        "0X0XXXXXXX", "XXXXXXX0XX", "XXX0XXXXXX", "XX0X0XXXXX",
        "XXXXX0X0XX", "XXXXXX0XXX", "X0X0XX0XXX", "XXXXXXXXXX",
        "0XXXXXXXX0", "XXXX0XXXXX", "XX0XXXX0XX", "XXX00XXXXX"
    });
    constexpr auto random_2 = merge_str({
        "0XXXXXXXXX","X0XXXXXXXX","XX0XXXXXXX","XXX0XXXXXX",
        "XXXX0XXXXX","XXXXXXX0XX","XXXXX0XXXX","XXXXXX0XXX",
        "XXXXXXX0XX","XXXXXXXX0X","XXXXXXXXX0","XXXXXXXXXX",
        "0XXXXXXXXX","X0XXXXXXXX","XX0XXXXXXX","XXX0XXXXXX",
        "XXXX0XXXXX","XXXX0XXXXX","XXXXX0XXXX","XXXXXX0XXX",
        "XXXXXXXXXX","XXXXXXXXXX","XXXXXXXX0X","XXXXXXXXXX"
    });
    // dtを盤面として使用
    Board board(random_2);


    std::cout << "Board after line clear:\n" 
    << to_string(board) << "\n";

    unsigned long long c0 = __rdtsc();
    int line = board.clear_full_lines();   // ここを測る
    unsigned long long c1 = __rdtsc();
    std::cout << (double)(c1-c0) << " cycles\n";
    std::cout << "Cleared lines: " << line << "\n";
    std::cout << "Board after line clear:\n" 
              << to_string(board) << "\n";

}


void test_search_7bag()
{
    using Board = reachability::board_t<10,24>;

    Board board;                                  // 空盤面
    auto queue = ai::common::generate_queue(20);  // 20 手先

    std::cout << "Queue: ";
    // 1回だけ探索（depthMax はヘッダの conf で制御）
    auto node = ai::search(board, std::span(queue.data(), queue.size()));

    std::cout << "=== Best path (" << node.path.size() << " moves) ===\n";

    Board cur = board;
    for(size_t i=0; i<node.path.size(); ++i){
        const auto& s = node.path[i];

        Board piece = ai::make_piece_board_runtime(s.rot, s.piece, s.x, s.y);
        cur = cur | piece;
        int cleared = cur.clear_full_lines();

        std::cout << i
                  << ": piece=" << s.piece
                  << (s.usedHold ? " (H)" : "")
                  << " rot="<<(int)s.rot
                  << " x="<<(int)s.x<<" y="<<(int)s.y
                  << " cleared="<<cleared
                  << " score="<<s.scoreAfter << "\n";
        std::cout << to_string(cur) << "\n";
    }

    std::cout << "Final Score = " << node.score << "\n";

    std::cout << "Queue: ";
    for (auto q : queue) std::cout << q << ' ';
    std::cout << "\n";
}

void test_aipath() {
    using namespace std::chrono_literals;

    Board board;                                   // 盤面
    auto queue = ai::common::generate_queue(512);  // 充分長い袋を用意
    char hold_slot = 0;                            // 実際のホールド（0=空）

    const size_t MAX_TURNS = 512;                  // 100手回す
    const bool   ANIMATE   = true;                // true で1手ずつ描画

    auto sleep_draw = [&](int ms){
        if (ANIMATE) std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    };

        auto box4x4 = [&](char p)->std::array<std::string,4>{
        // 4x4 を上から下へ（表示用）。spawn向きの簡易形状。
        // 必要最小限：視認性重視＆関数内完結。
        std::array<std::string,4> g = {"....","....","....","...."};
        switch(p){
            case 'I': g = {"....","XXXX","....","...."}; break;
            case 'O': g = {".XX."," .??", "....","...."}; break; // dummy, will fix below
            case 'T': g = {".X..","XXX.","....","...."}; break;
            case 'S': g = {".XX.","XX..","....","...."}; break;
            case 'Z': g = {"XX..",".XX.","....","...."}; break;
            case 'J': g = {"X...","XXX.","....","...."}; break;
            case 'L': g = {"..X.","XXX.","....","...."}; break;
            default:  g = {"....","....","....","...."}; break;
        }
        // Oミノの行を整形（スペースを'.'扱いに）
        if (p=='O'){ g = {".XX.",".XX.","....","...."}; }

        // 文字→"[]"/"--" に変換
        for (auto& row : g){
            std::string out; out.reserve(8);
            for(char c: row) out += (c=='X' ? "[]" : "--");
            row = std::move(out);
        }
        return g;
    };

    // 空ボックス（4x4 全部空）
    auto empty_box = [&]{
        std::array<std::string,4> g;
        for (auto& r: g) r = std::string(8, '-'); // "--------" = "--"×4
        return g;
    };

    // ヘッダ（HOLD + NEXT×5）を作って出力
    auto print_header = [&](const std::vector<char>& q, char hold){
        // HOLD ボックス
        auto holdBox = (hold ? box4x4(hold) : empty_box());

        // NEXT 5 ボックス（足りなければ空）
        std::array<std::array<std::string,4>,5> nextBoxes;
        for(int i=0;i<5;++i){
            if ((int)q.size()>i) nextBoxes[i] = box4x4(q[i]);
            else {
                auto e = empty_box();
                for(int r=0;r<4;++r) nextBoxes[i][r] = e[r];
            }
        }

        // ラベル行
        std::cout << "HOLD          NEXT x5\n";

        // 4段ぶんを横に並べて表示
        for(int r=0;r<4;++r){
            std::cout << holdBox[r] << "   |  ";
            for(int i=0;i<5;++i){
                std::cout << nextBoxes[i][r];
                if (i!=4) std::cout << ' ';
            }
            std::cout << '\n';
        }
    };

    for (size_t turn = 0; turn < MAX_TURNS && !queue.empty(); ++turn) {
        // --- 仮想キュー（search は「ホールド空」前提なのでここで整える） ---
        std::vector<char> vq = queue;
        if (hold_slot != 0 && !vq.empty()) {
            // 実態: 現在 Q0, hold=H。ホールド使用＝H を置く分岐も評価できるよう [Q0, H, Q1, ...] にする
            vq.insert(vq.begin() + 1, hold_slot);
        }

        // --- 探索（毎手やり直し） ---
        auto node = ai::search(board, std::span(vq.data(), vq.size()));
        if (node.path.empty()) {
            std::cout << "[turn " << turn << "] search failed: no path\n";
            break;
        }
        const auto& s = node.path.front(); // 今手の最善手だけ使う

        // 進行ログ
        std::cout << "[turn " << turn << "] piece=" << s.piece
                  << " rot=" << int(s.rot)
                  << " x=" << int(s.x) << " y=" << int(s.y)
                  << " usedHold=" << (s.usedHold ? "Y" : "N")
                  << "  (hold_slot=" << (hold_slot ? hold_slot : '-') << " / queue0=" << queue.front() << ")\n";

        // --- 入力列を作って、1手ぶん操作を再生 ---
        auto tokens = ai::build_input_path(board, s, {});
        int x = 4, y = 20, rot = 0;
        char piece = s.piece;

        if (!ai::can_place(board, piece, x, y, rot)) {
            std::cout << "  spawn blocked; abort\n";
            break;
        }

        auto draw = [&](std::string_view tag){
            if (!ANIMATE) return;
            std::cout << "[" << tag << "]\n";
            print_header(queue, hold_slot);
            if (tag == "hard"){
                std::cout << "[" << tag << "]\n"<< to_string(board) << "\n";
            }else{
                std::cout << "[" << tag << "]\n"
                          << to_string(board | ai::make_piece_board_runtime(rot, piece, x, y)) << "\n";
            }
            sleep_draw(80);
        };

        draw("spawn");
        for (const auto& t : tokens) {
            if (t == "left")       { if (ai::can_place(board, piece, x-1, y,   rot)) { --x;    draw("left");  } }
            else if (t == "right") { if (ai::can_place(board, piece, x+1, y,   rot)) { ++x;    draw("right"); } }
            else if (t == "soft")  { if (ai::can_place(board, piece, x,   y-1, rot)) { --y;    draw("soft");  } }
            else if (t == "cw")    { if (auto r = ai::try_rotate(board, piece, x, y, rot, 0)) { x=r->x; y=r->y; rot=r->rot; draw("cw"); } }
            else if (t == "ccw")   { if (auto r = ai::try_rotate(board, piece, x, y, rot, 1)) { x=r->x; y=r->y; rot=r->rot; draw("ccw"); } }
            else if (t == "hard") {
                // 床まで落として確定
                while (ai::can_place(board, piece, x, y-1, rot)) --y;
                board = board | ai::make_piece_board_runtime(rot, piece, x, y);
                int cleared = board.clear_full_lines();
                //std::cout << "  lock at y=" << y << " (cleared=" << cleared << ")\n";
                if (ANIMATE) { draw("hard"); sleep_draw(120); }
                break;
            }
        }

        // --- 実ホールド・実キューを更新（ここが肝） ---
        if (!s.usedHold) {
            // ホールド無し：Q0 を置いた扱い → キュー 1 枚進める
            if (!queue.empty()) queue.erase(queue.begin());
            // hold_slot は不変
        } else {
            if (hold_slot == 0) {
                // もともと空ホールド：Q0 を hold に送り、Q1 を置いた扱い → hold=Q0, キューは 2 枚進む
                char q0 = queue.size() ? queue[0] : 0;
                if (queue.size() >= 2) queue.erase(queue.begin(), queue.begin()+2);
                else queue.clear();
                hold_slot = q0;
            } else {
                // すでに何か持ってる：Q0 と hold をスワップし、hold を置いた扱い → hold=Q0, キューは 1 枚進む
                char q0 = queue.size() ? queue[0] : 0;
                if (!queue.empty()) queue.erase(queue.begin());
                hold_slot = q0;
            }
        }
    }

    std::cout << "Loop finished.\n";
}


int main() {
    std::ios::sync_with_stdio(false);
    std::cout.setf(std::ios::unitbuf); // すべての << でフラッシュ
    //test_initial_state();
    //test_set_get();
    // test_operator_not();
    // test_move();
    //test_to_string();
    // test_findfinalattachableminostates();
    // test_block_t();
    //test_spawn_mino();
    //test_stringto256();
    //test_spawn_lock_flow();
    //print_tspinMask(5, 1);
    //print_tspinMask(5, 0);
    //print_tspinMask_all();
    //test_tspinmaskmove();
    //test_tspinmaskmove2();
    //test_line_clear();
    //test_search_7bag();
    test_aipath();
    std::cout << "All board tests passed.\n";
    return 0;
}
