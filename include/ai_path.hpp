#pragma once
#include <vector>
#include <string>
#include <queue>
#include <array>
#include <optional>
#include <limits>
#include <algorithm>
#include <type_traits>
#include "ai_search.hpp"  // Board, RS, ai::Step

namespace ai {

// --- アクション重み ------------------------------------------------------------
struct TokenCost {
    int left = 1;
    int right = 1;
    int soft = 2;
    int cw = 1;
    int ccw = 1;
    int hard = 0;   // 最後に必ず1回付与する分（選択には影響しないが、返却列の見積り用）
    int hold = 0;   // goal.usedHold が true のとき先頭に付与（選択には影響しない）
};

// （任意）経路の合計コストをあとで見積もるときに使う
inline int total_cost(const std::vector<std::string>& path, const TokenCost& c) {
    int t = 0;
    for (auto& s : path) {
        if (s=="left") t += c.left;
        else if (s=="right") t += c.right;
        else if (s=="soft") t += c.soft;
        else if (s=="cw") t += c.cw;
        else if (s=="ccw") t += c.ccw;
        else if (s=="hard") t += c.hard;
        else if (s=="hold") t += c.hold;
    }
    return t;
}

// --- 低レベル：衝突チェック（盤外=2／ブロック=1 は不可） ---------------------------
inline bool can_place(const Board& bd, char piece, int x, int y, int rot){
    bool ok = true;
    reachability::blocks::call_with_block<RS>(piece, [&]<auto B>{
        const int idx = B.mino_index[rot % B.ORIENTATIONS];
        const auto& cs = B.minos[idx];
        for (const auto& c : cs) {
            const int gx = x + c[0];
            const int gy = y + c[1];
            int v = bd.get(gx, gy);         // 0=empty, 1=block, 2=OOB
            if (v != 0){ ok = false; return; }
        }
    });
    return ok;
}

// --- 「最終配置の上が空か」を判定（上に1つでもあれば false） ----------------------
inline bool is_clear_above(const Board& bd, char piece, int gx, int gy, int grot){
    bool ok = true;
    reachability::blocks::call_with_block<RS>(piece, [&]<auto B>{
        const int idx = B.mino_index[grot % B.ORIENTATIONS];
        const auto& cs = B.minos[idx];
        for (const auto& c : cs){
            const int x = gx + c[0];
            const int y = gy + c[1];
            for (int yy = y + 1; yy < Board::height; ++yy){ // final より上
                int v = bd.get(x, yy); // 0=空, 1=ブロック
                if (v != 0){ ok = false; return; }
            }
        }
    });
    return ok;
}

struct RotResult { int x, y, rot; };

inline std::optional<RotResult> try_rotate(const Board& bd, char piece,
                                           int x, int y, int rot, int dir)
{
    std::optional<RotResult> res;
    reachability::blocks::call_with_block<RS>(piece, [&]<auto B>{
        using Block = std::decay_t<decltype(B)>; // B は“値”なので型は decltype(B)

        // ★ 回転を持たないミノ（O等）はコンパイル時にここで打ち切り
        if constexpr (Block::ROTATIONS == 0 || Block::ORIENTATIONS <= 1) {
            return;
        } else {
            if (dir < 0 || dir >= Block::ROTATIONS) return;

            const int from = ((rot % Block::ORIENTATIONS) + Block::ORIENTATIONS) % Block::ORIENTATIONS;
            const int to   = Block::rotation_target(from, dir);

            // ★ ROTATIONS>0 の場合のみこのコードが実体化される
            for (const auto& k : B.kicks[from][dir]) {
                const int nx = x + k[0], ny = y + k[1];
                if (can_place(bd, piece, nx, ny, to)) {
                    res = RotResult{nx, ny, to};
                    return;
                }
            }
        }
    });
    return res;
}

// --- 重み付き最短経路（Dijkstra） -------------------------------------------------
// 返り値: トークン列（必要なら先頭に "hold"、末尾に "hard" 付き）
inline std::vector<std::string>
build_input_path(const Board& bd, const Step& goal, const TokenCost& cost = {}) {
    std::vector<std::string> out;
    if (goal.usedHold) out.push_back("hold");  // 定数分なので探索の相対比較には影響なし

    // 初期スポーン（必要なら引数化してね）
    constexpr int W = Board::width, H = Board::height;
    constexpr int SPAWN_X = 4, SPAWN_Y = 20;
    const char piece = goal.piece;
    struct S { int x,y,rot; };
    const S start{SPAWN_X, SPAWN_Y, 0};
    const S target{goal.x, goal.y, int(goal.rot)};

    if (!can_place(bd, piece, start.x, start.y, start.rot)) {
        throw std::runtime_error("Spawn blocked: cannot place piece at initial position");
        out.push_back("hard");
        return out; // 湧き潰れ
    }

    // 直落ち（soft不要）判定：ゴールの真上にブロックが無ければ true
    bool straight_drop = is_clear_above(bd, piece, target.x, target.y, target.rot);

    // Dijkstra
    static int  dist[W][H][4];
    static bool vis [W][H][4];
    struct Parent { int px,py,prot; char act; };
    static Parent par[W][H][4];

    for (int x=0;x<W;++x) for (int y=0;y<H;++y) for (int r=0;r<4;++r){
        dist[x][y][r] = std::numeric_limits<int>::max();
        vis [x][y][r] = false;
        par [x][y][r] = {-1,-1,-1,0};
    }

    auto idx_ok = [&](int x,int y,int r){
        return (0<=x && x<W && 0<=y && y<H && 0<=r && r<4);
    };

    using QN = std::pair<int, S>; // (cost, state)
    auto cmp = [](const QN& a, const QN& b){ return a.first > b.first; };
    std::priority_queue<QN, std::vector<QN>, decltype(cmp)> pq(cmp);

    dist[start.x][start.y][start.rot] = 0;
    pq.push({0, start});


    auto relax = [&](const S& from, const S& to, char act, int w){
        if (!idx_ok(to.x,to.y,to.rot)) return;
        if (!can_place(bd, piece, to.x,to.y,to.rot)) return;
        int nd = dist[from.x][from.y][from.rot] + w;
        if (nd < dist[to.x][to.y][to.rot]) {
            dist[to.x][to.y][to.rot] = nd;
            par[to.x][to.y][to.rot] = {from.x,from.y,from.rot,act};
            pq.push({nd, to});
        }
    };

    while(!pq.empty()){
        auto [d, s] = pq.top(); pq.pop();
        if (vis[s.x][s.y][s.rot]) continue;
        vis[s.x][s.y][s.rot] = true;

        // 到達条件を切替
        bool same_shape_rot = false;
        switch (piece) {
            case 'O':
                same_shape_rot = true;                          // どのrotでも同形
                break;
            case 'S': case 'Z': case 'I':
                same_shape_rot = ((s.rot & 1) == (target.rot & 1)); // 0/2, 1/3 を同形扱い
                break;
            default:
                same_shape_rot = (s.rot == target.rot);         // J/L/T は完全一致
                break;
        }
        bool reached;
        if (straight_drop) {
            reached = (s.x == target.x && same_shape_rot);
        } else {
            reached = (s.x == target.x && s.y == target.y && same_shape_rot);
            if (s.x == target.x && s.y == target.y && !reached) {
                std::cout << "[WARN] target.rot=" << target.rot
                        << " s.rot=" << s.rot << " (same_shape=0)\n";
            }
        }


        if (reached) {
            // 復元
            std::vector<std::string> tail;
            for (Parent p = par[s.x][s.y][s.rot]; p.px!=-1; p = par[p.px][p.py][p.prot]){
                switch(p.act){
                    case 'L': tail.push_back("left"); break;
                    case 'R': tail.push_back("right"); break;
                    case 'S': tail.push_back("soft"); break;
                    case 'C': tail.push_back("cw");   break;
                    case 'Z': tail.push_back("ccw");  break;
                }
            }
            std::reverse(tail.begin(), tail.end());
            out.insert(out.end(), tail.begin(), tail.end());
            out.push_back("hard"); // 最後に確定
            return out;
        }

        // 遷移（左右・回転は常に展開）
        relax(s, S{s.x-1, s.y,   s.rot}, 'L', cost.left);
        relax(s, S{s.x+1, s.y,   s.rot}, 'R', cost.right);

        if (auto cw = try_rotate(bd, piece, s.x, s.y, s.rot, 0))
            relax(s, S{cw->x,cw->y,cw->rot}, 'C', cost.cw);
        if (auto ccw = try_rotate(bd, piece, s.x, s.y, s.rot, 1))
            relax(s, S{ccw->x,ccw->y,ccw->rot}, 'Z', cost.ccw);

        // soft は「直落ち不可」モードのときだけ展開
        if (!straight_drop)
            relax(s, S{s.x,   s.y-1, s.rot}, 'S', cost.soft);
    }

    // 到達不可
    out.push_back("hard");
    return out;
}

} // namespace ai
