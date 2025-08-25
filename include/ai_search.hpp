// tetris_search_debug.hpp — Exploration Engine v3.4 (path復元 & PPT Hold正規化)
// ====================================================================
// * AI_SEARCH_DEBUG 定義時のみ std::cerr へ詳細ログを吐きます
// * 親インデックス方式で道中パス復元＆コピー削減
// * PPT準拠: ホールドは毎ターン1回だけ／空には戻らない／キュー自体は並び不変
// ====================================================================
#pragma once

// ----------------------- 依存ヘッダ -------------------------
#include "board.hpp"
#include "block.hpp"
#include "findfinalattachableminostates.hpp"
#include "ai_evaluate.hpp"
#include <array>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <span>
#include <cstdint>
#include <iterator>
#include <cassert>
#ifdef AI_SEARCH_DEBUG
  #include <iostream>
#endif

// ----------------------- ログマクロ -------------------------
#ifdef AI_SEARCH_DEBUG
  #define AI_DBG(msg) do{ std::cerr << msg << '\n'; }while(0)
#else
  #define AI_DBG(msg) ((void)0)
#endif

namespace ai {
using namespace reachability;
using RS    = reachability::blocks::SRS;
using Board = board_t<10,24>;

// ----------------------- 探索パラメータ ---------------------
struct Conf {
    int depthMax     = 15;
    int beamInit     = 400;
    int beamMin      = 40;
    int shrinkStride = 3;
};
static constexpr Conf conf;

static constexpr int QUEUE_MAX = 32;  // queuePos は 0‑31

// ----------------------- 手順情報 ---------------------------
struct Step {
    char    piece   = 0;
    bool    usedHold= false;
    uint8_t rot     = 0;
    uint8_t x       = 0, y = 0;
    int     cleared = 0;
    int     scoreAfter = 0;
};

// ----------------------- 結果ノード --------------------------
struct Node {
    Board board{};
    char  hold   = 0;       // 現在保持中のホールドミノ (0=空)
    int   depth  = 0;
    int   score  = 0;
    std::vector<Step> path{}; // ベストルート手順
};

// ----------------------- 状態キー生成 -----------------------
inline uint64_t height_signature(const Board& b){
    uint64_t sig = 0;
    for(int x=0; x<10; ++x){
        int y=23; while(y>=0 && !b.get(x,y)) --y;
        sig |= uint64_t(y+1) << (x*5); // 5bit×10=50bit
    }
    return sig;
}

inline uint64_t make_state_key(const Board& b, char hold, int pos){
    uint64_t k = height_signature(b);
    k ^= uint64_t(uint8_t(hold))     << 50;
    k ^= uint64_t(uint8_t(pos & 31)) << 58; // 5bit
    return k;
}

// ----------------------- WorkNode (内部) --------------------
// path を持たず、親 index + 最後の一手だけ保持
struct WorkNode {
    // 盤面と評価
    Board board{};
    char  hold   = 0;
    int   depth  = 0;
    int   score  = 0;

    // キュー
    std::array<char,QUEUE_MAX> next{};
    int queuePos = 0;

    // 経路復元用
    int  parent = -1;  // pool 内 親 index（root:-1）
    int  id     = -1;  // pool 内 自 index
    Step move{};       // 親→自の手（root未使用）
};

// ----------------------- ランタイム生成 ---------------------
// rot を実行時指定で Board を生成（テスト再生用）
inline Board make_piece_board_runtime(int rot, char piece, uint8_t x, uint8_t y){
    switch(rot){
    case 0: return Board::template make_piece_board<0,RS>(x,y,piece);
    case 1: return Board::template make_piece_board<1,RS>(x,y,piece);
    case 2: return Board::template make_piece_board<2,RS>(x,y,piece);
    default:return Board::template make_piece_board<3,RS>(x,y,piece);
    }
}

// ----------------------- 子ノード展開 -----------------------
inline void expand(const WorkNode& parent,
                   std::vector<WorkNode>& out,
                   std::unordered_set<uint64_t>& dedup,
                   std::vector<WorkNode>& pool)
{
    constexpr coord SPAWN{4,20};

    auto push_child = [&](Board brd, char hold, int newPos, Step st){
        if(newPos >= QUEUE_MAX){ AI_DBG("[Skip] newPos OOB="<<newPos); return; }

        int cleared = brd.clear_full_lines();

        WorkNode ch{};
        ch.board    = brd;
        ch.hold     = hold;
        ch.depth    = parent.depth + 1;
        ch.score    = ai::evaluate(ch.board, cleared);
        ch.next     = parent.next;
        ch.queuePos = newPos;

        ch.parent   = parent.id;
        st.cleared    = cleared;
        st.scoreAfter = ch.score;
        ch.move     = st;

        uint64_t key = make_state_key(ch.board, ch.hold, ch.queuePos);
        if(dedup.insert(key).second){
            ch.id = static_cast<int>(pool.size());
            pool.push_back(ch);
            out.push_back(ch);
            AI_DBG("  [+] push depth="<<ch.depth<<" pos="<<newPos<<" score="<<ch.score);
        }
    };

    // --- キュー取得 ---
    if(parent.queuePos >= QUEUE_MAX){ AI_DBG("[Stop] parent.queuePos OOB"); return; }
    const char cur = parent.next[parent.queuePos];
    if(cur == 0){ AI_DBG("[Stop] queue empty at pos="<<parent.queuePos); return; }

    AI_DBG("[Expand] depth="<<parent.depth
           <<" cur="<<cur
           <<" hold="<<int(parent.hold));

    // ========================================================
    // (1) 通常配置: cur をそのまま置く
    // ========================================================
    {
        const int newPos = parent.queuePos + 1; // キュー1個消費
        auto land = search::binary_bfs<RS,SPAWN>(parent.board, cur);
        static_for<4>([&](auto rc){
            constexpr std::size_t ROT = rc;
            land[ROT].list_bits_256([&](uint8_t x,uint8_t y){
                Board blk = Board::template make_piece_board<ROT,RS>(x,y,cur);
                // #ifdef AI_SEARCH_DEBUG
                //   std::cerr << to_string(parent.board | blk) << "\n";
                // #endif
                Step st{cur,false,(uint8_t)ROT,x,y,0,0};
                push_child(parent.board | blk, parent.hold, newPos, st);
            });
        });
    }

    // ========================================================
    // (2) Hold配置
    //    - 空: cur を hold に入れて queue[curPos+1] を使う → キュー2個消費
    //    - 埋: cur と hold を交換して hold を使う       → キュー1個消費
    // ========================================================
    {
        char newHold = cur;
        if(parent.hold == 0){
            // 初回ホールド
            int idx2 = parent.queuePos + 1;
            if(idx2 >= QUEUE_MAX){ AI_DBG("[Hold] idx2 OOB="<<idx2); goto HOLD_END; }
            char use = parent.next[idx2];
            if(use == 0){ AI_DBG("[Hold] queue empty idx2="<<idx2); goto HOLD_END; }

            auto landH = search::binary_bfs<RS,SPAWN>(parent.board, use);
            static_for<4>([&](auto rc){
                constexpr std::size_t ROT = rc;
                landH[ROT].list_bits_256([&](uint8_t x,uint8_t y){
                    Board blk = Board::template make_piece_board<ROT,RS>(x,y,use);
                    // #ifdef AI_SEARCH_DEBUG
                    //   std::cerr << to_string(parent.board | blk) << "\n";
                    //   std::cerr << "Hold(new)=" << newHold << " Use=" << use << "\n";
                    // #endif
                    Step st{use,true,(uint8_t)ROT,x,y,0,0};
                    push_child(parent.board | blk, newHold, parent.queuePos + 2, st);
                });
            });
        }else{
            // 交換ホールド
            char use = parent.hold;
            auto landH = search::binary_bfs<RS,SPAWN>(parent.board, use);
            static_for<4>([&](auto rc){
                constexpr std::size_t ROT = rc;
                landH[ROT].list_bits_256([&](uint8_t x,uint8_t y){
                    Board blk = Board::template make_piece_board<ROT,RS>(x,y,use);
                    // #ifdef AI_SEARCH_DEBUG
                    //   std::cerr << to_string(parent.board | blk) << "\n";
                    //   std::cerr << "Hold(sw)=" << newHold << " Use=" << use << "\n";
                    // #endif
                    Step st{use,true,(uint8_t)ROT,x,y,0,0};
                    push_child(parent.board | blk, newHold, parent.queuePos + 1, st);
                });
            });
        }
    }
HOLD_END:;
}

// ----------------------- ビーム幅計算 -----------------------
inline int beam_width(int depth){
    int w = conf.beamInit >> (depth / conf.shrinkStride);
    return std::max(w, conf.beamMin);
}

// ----------------------- メイン探索 ------------------------
inline Node search(Board init, std::span<const char> queue)
{
    // root 設定
    WorkNode root{};
    root.board    = init;
    root.hold     = 0;
    root.depth    = 0;
    root.score    = ai::evaluate(init,0);
    std::fill(root.next.begin(), root.next.end(), 0);
    std::copy_n(queue.begin(), std::min<std::size_t>(QUEUE_MAX, queue.size()), root.next.begin());
    root.queuePos = 0;
    root.parent   = -1;
    root.id       = 0;

    std::vector<WorkNode> pool;
    pool.reserve(1<<16);
    pool.push_back(root);

    std::vector<WorkNode> frontier{root};
    std::vector<WorkNode> next;
    std::unordered_set<uint64_t> dedup;
    dedup.reserve(1<<16);
    dedup.insert(make_state_key(root.board, root.hold, root.queuePos));

    AI_DBG("[Search] start frontier=1 depthMax="<<conf.depthMax);

    // 深さループ
    for(int d=0; d<conf.depthMax; ++d){
        next.clear();
        AI_DBG("--- depth="<<d<<" frontier="<<frontier.size());
        for(const auto& n: frontier) expand(n,next,dedup,pool);
        if(next.empty()){ AI_DBG("[Stop] next empty at depth="<<d); break; }

        int bw = beam_width(d+1);
        std::partial_sort(next.begin(),
                          next.begin()+std::min<int>(bw,next.size()),
                          next.end(),
                          [](const WorkNode& a,const WorkNode& b){return a.score>b.score;});
        if(static_cast<int>(next.size())>bw) next.resize(bw);
        frontier.swap(next);
    }

    // 最良ノード決定
    const WorkNode& best = *std::max_element(frontier.begin(), frontier.end(),
        [](const WorkNode& a,const WorkNode& b){ return a.score < b.score; });

    AI_DBG("[Search] best depth="<<best.depth<<" score="<<best.score);

    // パス復元
    std::vector<Step> path;
    for(int id = best.id; id != -1; id = pool[id].parent){
        if(id != 0) path.push_back(pool[id].move); // rootは手無し
    }
    std::reverse(path.begin(), path.end());

    Node ret;
    ret.board = best.board;
    ret.hold  = best.hold;
    ret.depth = best.depth;
    ret.score = best.score;
    ret.path  = std::move(path);
    return ret;
}

} // namespace ai
