#pragma once
#include "block.hpp"
#include "utils.hpp"
#include <tuple>
#include <queue>
#include <array>
#include <type_traits>
#include <span> 

namespace reachability::search {
  using namespace blocks;
  //指定されたminoの形状が使用可能な位置を計算する関数
  //data:既存の盤面の状態を示す
  //盤面の空いている位置を調べ,minoが配置可能な場所を返す　newjadeさんの記事の事前計算だね
  template <std::array mino, typename board_t>
  constexpr board_t usable_positions(board_t data) {
    board_t positions = ~board_t();//初期状態では全ての位置が使用可能
    static_for<mino.size()>([&][[gnu::always_inline]](auto i) {
      positions &= (~data).template move<-mino[i]>();// `data` のブロックがある位置を除外 //このmoveはboardの非破壊的move
    });
    return positions;// 使用可能な場所を返す
  }
  //ブロックを落とした際の着地可能位置を計算する関数
  //usable:使用可能な位置
  //usable:のうち、下に移動できない (固定される) 位置を計算
  template <typename board_t>
  constexpr board_t landable_positions(board_t usable) {
    return usable & ~usable.template move<coord{0, 1}>();// 下に移動できない場所が着地点
  }
  //連続したラインを持つブロックの検出(特定のラインがすべて埋まっているか確認)　だから、行が全て埋まっていたらその行を渡す
  //`usable`: 使用可能な位置の情報
  //1行以上の連続したラインがあるかを確認
  template <typename board_t>
  constexpr board_t consecutive_lines(board_t usable) {
    const auto indicator01 = usable.get_heads();// 最上部のブロックの位置を取得
    return indicator01.has_single_bit();// 単一のビットのみが立っている場合は連続ラインと判定
  }

  // バイナリBFS (高速探索) による到達可能な盤面を探索する関数
  // - `block`: 探索対象のブロック
  // - `start`: ブロックの初期位置
  // - `init_rot`: 初期の回転状態
  // - `data`: 盤面の状態
  template <block block, coord start, unsigned init_rot, typename board_t>
  constexpr std::array<board_t, block.SHAPES> binary_bfs(board_t data) {
    constexpr int orientations = block.ORIENTATIONS;// ブロックの回転状態の数
    constexpr int rotations = block.ROTATIONS;// 回転操作の数
    constexpr int kicks = block.KICK_PER_ROTATION;// 壁蹴りの数
    constexpr int shapes = block.SHAPES;// ブロックの形状の数
    
    board_t usable[shapes];// 各形状ごとの使用可能位置を計算　newjadeさんの記事の事前計算の部分を全パターンやってる部分
    static_for<shapes>([&][[gnu::always_inline]](auto i) {
      usable[i] = usable_positions<block.minos[i]>(data);
    });
    // 移動可能な方向 (左, 右, 下)
    constexpr std::array<coord, 3> MOVES = {{{-1, 0}, {1, 0}, {0, -1}}};
    // 初期配置位置の計算 (盤面上のスタート位置を調整)
    constexpr coord start2 = start + block.mino_offset[init_rot];
    constexpr auto init_rot2 = block.mino_index[init_rot];
    if (!usable[init_rot2].template get<start2[0], start2[1]>()) [[unlikely]] {
      return {};// 初期位置が無効なら空の結果を返す
    }
    // 各回転状態ごとの訪問状態を管理 (BFSのための探索キュー的な役割)
    bool need_visit[orientations] = { };
    need_visit[init_rot] = true;
    // 到達可能な盤面データをキャッシュ
    std::array<board_t, orientations> cache;
    const auto consecutive = consecutive_lines(usable[init_rot2]);
    // 連続したラインがある場合の処理 (完全なラインを削除しながら探索)
    if (consecutive.template get<start2[1]>()) [[likely]] {
      const auto current = usable[init_rot2] & usable[init_rot2].template move<coord{0, -1}>();
      const auto covered = usable[init_rot2] & ~current;
      const auto expandable = can_expand(current, covered);
      auto whole_line_usable = (expandable | ~covered.get_heads()).all_bits().populate_highest_bit();
      constexpr int removed_lines = board_t::height - start2[1];// 取り除く行数の計算
      if constexpr (removed_lines > 0) {
        whole_line_usable |= ~(~board_t()).template move<coord{0, -removed_lines}>();
      }
      auto good_lines = whole_line_usable.remove_ones_after_zero();
      if constexpr (removed_lines > 1) {
        good_lines &= (~board_t()).template move<coord{0, -(removed_lines - 1)}>();
      }
      cache[init_rot] = good_lines & usable[init_rot2];
    } else {
      cache[init_rot].template set<start2[0], start2[1]>();
    }
    // BFS (幅優先探索) による到達可能範囲の拡張
    for (bool updated = true; updated;) [[unlikely]] {
      updated = false;
      static_for<orientations>([&][[gnu::always_inline]](auto i){
        if (!need_visit[i]) {
          return;
        }
        constexpr auto index = block.mino_index[i];
        need_visit[i] = false;
        while (true) {
          board_t result = cache[i];
          static_for<MOVES.size()>([&][[gnu::always_inline]](auto j) {
            result |= cache[i].template move<MOVES[j]>();
          });
          result &= usable[index];
          if (result != cache[i]) [[likely]] {
            cache[i] = result;
          } else {
            break;
          }
        }
        // ブロックの回転処理 (BFS探索中の拡張処理)
        static_for<rotations>([&][[gnu::always_inline]](auto j){
          constexpr int target = block.rotation_target(i, j);// 回転後のターゲット状態
          board_t to = cache[target];// 現在のターゲットキャッシュ
          constexpr auto index2 = block.mino_index[target];// 回転後の形状のインデックス
          board_t temp = cache[i];// 現在の盤面キャッシュ
          // 壁蹴り (kicks) を適用して回転可能か確認
          static_for<kicks>([&][[gnu::always_inline]](auto k){
            to |= temp.template move<block.kicks[i][j][k]>();// 壁蹴り適用
            temp &= ~usable[index2].template move<-block.kicks[i][j][k]>();// 適用後のブロックを排除
          });
          to &= usable[index2];// 使用可能な領域内に制限
          // 以前のキャッシュと比較し、更新があるかチェック
          auto old_cache = cache[target];
          cache[target] = to;
          if (to != old_cache) {// キャッシュが変化した場合は再探索が必要
            need_visit[target] = true;
            if constexpr (target < i)
              updated = true;
          }
        });
      });
    }
    // 最終的な到達可能位置を格納する配列を作成
    std::array<board_t, shapes> ret;
    // 各回転状態のキャッシュを `shapes` に格納 (ブロックの形状ごとに統合)
    static_for<orientations>([&][[gnu::always_inline]](auto i){
      constexpr auto index = block.mino_index[i];
      ret[index] |= cache[i];
    });
    // 着地可能な位置のみを取得 (最終的にブロックが静止する位置を抽出)
    static_for<shapes>([&][[gnu::always_inline]](auto i){
      ret[i] &= landable_positions(usable[i]);
    });
    return ret;// 最終的な到達可能な位置を返す
  }

  // バイナリBFS (binary_bfs) のエントリーポイント関数
  // - `RS`: 回転システム (SRS など)
  // - `start`: 初期座標
  // - `init_rot`: 初期回転状態 (デフォルト 0)
  // - `board_t`: 盤面の型

  // template <typename RS, coord start, unsigned init_rot=0, typename board_t>
  // [[gnu::noinline]]
  // constexpr static_vector<board_t, 4> binary_bfs(board_t data, char b) {
  //   return call_with_block<RS>(b, [=]<block 
  //     auto ret = binary_bfs<B, start, init_rot>(data);B>() {
  //     return static_vector<board_t, 4>{std::span{ret}};
  //   });
  // }

  template <typename RS, coord start, unsigned init_rot=0, typename board_t>
  [[gnu::noinline]]
  constexpr static_vector<board_t, 4> binary_bfs(board_t data, char b) {
    return call_with_block<RS>(b, [=]<block B>() {
      auto ret = binary_bfs<B, start, init_rot>(data);
      return static_vector<board_t, 4>{std::span{ret}};
    });
  }
}