#pragma once//多重インクルード防止
#include <array>
#include <utility>

namespace reachability {//テトリスのブロック管理のための名前空間
  using coord = std::array<int, 2>;//ブロックの座標ｘｙをstd::array<int, 2> で表現

  constexpr coord operator-(const coord &co) {//座標の符号を反転する `-` 演算子をオーバーロード（-を使うことで反転できる！）
    return {-co[0], -co[1]};
  }
  //テトリスのブロックデータを格納する構造体
  template <int shapes, int orientations, int rotations, int block_per_mino, int kick_per_rotation>
  struct block {
    using mino = std::array<coord, block_per_mino>;// 1つのミノの座標リスト
    using kick = std::array<coord, kick_per_rotation>;// 回転時のキックリスト
    //コンパイル時、定数でブロックのパラメータを定義
    static constexpr int SHAPES = shapes;//ブロックの形状数
    static constexpr int ORIENTATIONS = orientations;//ブロックの向き(0,r,2,l)
    static constexpr int ROTATIONS = rotations;//回転の種類(cw,ccw)
    static constexpr int BLOCK_PER_MINO = block_per_mino;//1つのミノのブロック数
    static constexpr int KICK_PER_ROTATION = kick_per_rotation;//キックテーブルの数
    [[no_unique_address]]//メンバ変数のアドレスを最適化する属性
    std::array<mino, shapes> minos;// 各形状のブロックデータ
    [[no_unique_address]]
    std::array<std::array<kick, rotations>, orientations> kicks;// 各向きでのキックデータ
    [[no_unique_address]]
    std::array<int, orientations> mino_index;// ブロックのインデックス
    [[no_unique_address]]
    std::array<coord, orientations> mino_offset;// 各向きのオフセット
    //回転時に新しい向きを計算する関数
    static constexpr int rotation_target(int from, int rotation_num) {
      if constexpr (orientations == 4 && rotations == 2) {
        // cw & ccw
        return (from + rotation_num * 2 + 1) % orientations;
      }
      return (from + rotation_num + 1) % orientations;
    }
  };
}
//ブロックデータの定義
namespace reachability::blocks {
  //ブロックデータの基本構造
  template <int block_per_mino, int orientations>
  struct pure_block {
    using mino = std::array<coord, block_per_mino>; // ブロックの座標情報
    [[no_unique_address]]
    std::array<mino, orientations> minos;// 各向きの座標リスト
  };
  //pure_blockにoffset情報を追加
  template <int shapes, int block_per_mino, int orientations>
  struct block_with_offset {
    using mino = std::array<coord, block_per_mino>;
    [[no_unique_address]]
    std::array<mino, shapes> minos;
    [[no_unique_address]]
    std::array<int, orientations> mino_index;
    [[no_unique_address]]
    std::array<coord, orientations> mino_offset;
  };
  //pure_blockをblock_with_offsetに変換する関数
  template <int block_per_mino, int orientations>
  constexpr block_with_offset<orientations, block_per_mino, orientations> convert(const pure_block<block_per_mino, orientations> &b) {
    block_with_offset<orientations, block_per_mino, orientations> ret;
    for (int i = 0; i < orientations; ++i) {
      ret.minos[i] = b.minos[i];
      ret.mino_index[i] = i;
      ret.mino_offset[i] = {0, 0};
    }
    return ret;
  }
  //pure_blockをblock_with_offsetに変換する関数
  template <int shapes, int block_per_mino, int orientations>
  constexpr block_with_offset<shapes, block_per_mino, orientations> convert(
    const pure_block<block_per_mino, shapes> &b,
    const std::array<int, orientations> &mino_index,
    std::array<coord, orientations> mino_offset
  ) {
    return {
      b.minos,
      mino_index,
      mino_offset
    };
  }
  //回転時のキック情報を格納する構造体
  template <int orientations, int rotations, int kick_per_rotation>
  struct pure_kick {
    using kick = std::array<coord, kick_per_rotation>;
    [[no_unique_address]]
    std::array<std::array<kick, rotations>, orientations> kicks;
  };
  //座標の加算を行う関数
  constexpr coord operator+(const coord &co1, const coord &co2) {
    return {co1[0] + co2[0], co1[1] + co2[1]};
  }
  //`block_with_offset` と `pure_kick` を統合して `block` を作成
  template <int shapes, int orientations, int rotations, int block_per_mino, int kick_per_rotation>
  constexpr block<shapes, orientations, rotations, block_per_mino, kick_per_rotation> operator+(
    const block_with_offset<shapes, block_per_mino, orientations> &b,
    const pure_kick<orientations, rotations, kick_per_rotation> &k
  ) {
    std::array<std::array<std::array<coord, kick_per_rotation>, rotations>, orientations> kicks;
    for (int i = 0; i < orientations; ++i) {
      for (int j = 0; j < rotations; ++j) {
        const auto target = block<shapes, orientations, rotations, block_per_mino, kick_per_rotation>::rotation_target(i, j);
        const auto offset = -b.mino_offset[i] + b.mino_offset[target];
        for (int l = 0; l < kick_per_rotation; ++l) {
          kicks[i][j][l] = k.kicks[i][j][l] + offset;
        }
      }
    }
    return {
      b.minos,
      kicks,
      b.mino_index,
      b.mino_offset
    };
  }
 //回転しないブロックのデータ
  inline constexpr pure_kick<1, 0, 0> no_rotation;
 //各ブロックの定義(座標のみ)
  inline constexpr pure_block<4, 4> T = {{{
    {{{-1, 0}, {0, 0}, {1, 0}, {0, 1}}},  // 0
    {{{0, 1}, {0, 0}, {0, -1}, {1, 0}}},  // R
    {{{1, 0}, {0, 0}, {-1, 0}, {0, -1}}}, // 2
    {{{0, -1}, {0, 0}, {0, 1}, {-1, 0}}}  // L
  }}};
  inline constexpr pure_block<4, 2> Z  = {{{
    {{{-1, 1}, {0, 1}, {0, 0}, {1, 0}}},   // 0
    {{{-1, -1}, {-1, 0}, {0, 0}, {0, 1}}}  // L
  }}};
  inline constexpr pure_block<4, 2> S = {{{
    {{{1, 1}, {0, 1}, {0, 0}, {-1, 0}}},   // 0
    {{{-1, 1}, {-1, 0}, {0, 0}, {0, -1}}}  // L
  }}};
  inline constexpr pure_block<4, 4> J = {{{
    {{{-1, 1}, {-1, 0}, {0, 0}, {1, 0}}}, // 0
    {{{1, 1}, {0, 1}, {0, 0}, {0, -1}}},  // R
    {{{1, -1}, {1, 0}, {0, 0}, {-1, 0}}}, // 2
    {{{-1, -1}, {0, -1}, {0, 0}, {0, 1}}} // L
  }}};
  inline constexpr pure_block<4, 4> L = {{{
    {{{-1, 0}, {0, 0}, {1, 0}, {1, 1}}},  // 0
    {{{0, 1}, {0, 0}, {0, -1}, {1, -1}}}, // R
    {{{1, 0}, {0, 0}, {-1, 0}, {-1, -1}}},// 2
    {{{0, -1}, {0, 0}, {0, 1}, {-1, 1}}}  // L
  }}};
  inline constexpr pure_block<4, 1> O = {{{
    {{{0, 0}, {1, 0}, {0, 1}, {1, 1}}},
  }}};
  inline constexpr pure_block<4, 2> I = {{{
    {{{-1, 0}, {0, 0}, {1, 0}, {2, 0}}},   // 0
    {{{0, 1}, {0, 2}, {0, 0}, {0, -1}}},   // L
  }}};

  template <typename RS>
  [[gnu::always_inline]] constexpr auto call_with_block(char ch, auto f) {
    switch (ch) {
      case 'T': return f.template operator()<RS::T>();
      case 'Z': return f.template operator()<RS::Z>();
      case 'S': return f.template operator()<RS::S>();
      case 'J': return f.template operator()<RS::J>();
      case 'L': return f.template operator()<RS::L>();
      case 'O': return f.template operator()<RS::O>();
      case 'I': return f.template operator()<RS::I>();
      default: std::unreachable();
    }
  }

  struct SRS { // used as a namespace but usable as a template parameter
    static constexpr pure_kick<4, 2, 5> common_kick = {{{//通常のブロック (Iブロック以外) 用の SRS キックテーブル
      {{ // 0
        {{{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}}},  // -> R
        {{{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}}}      // -> L
      }},
      {{ // R
        {{{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}}},      // -> 2
        {{{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}}}       // -> 0
      }},
      {{ // 2
        {{{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}}},     // -> L
        {{{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}}}   // -> R
      }},
      {{ // L
        {{{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}},   // -> 0
        {{{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}}    // -> 2
      }}
    }}};
    static constexpr pure_kick<4, 2, 5> I_kick = {{{//Iブロック専用の SRS キックテーブル
      {{
        {{{1, 0}, {-1, 0}, {2, 0}, {-1, -1}, {2, 2}}},
        {{{0, -1}, {-1, -1}, {2, -1}, {-1, 1}, {2, -2}}},
      }},
      {{
        {{{0, -1}, {-1, -1}, {2, -1}, {-1, 1}, {2, -2}}},
        {{{-1, 0}, {1, 0}, {-2, 0}, {1, 1}, {-2, -2}}},
      }},
      {{
        {{{-1, 0}, {1, 0}, {-2, 0}, {1, 1}, {-2, -2}}},
        {{{0, 1}, {1, 1}, {-2, 1}, {1, -1}, {-2, 2}}},
      }},
      {{
        {{{0, 1}, {1, 1}, {-2, 1}, {1, -1}, {-2, 2}}},
        {{{1, 0}, {-1, 0}, {2, 0}, {-1, -1}, {2, 2}}},
      }}
    }}};
     //各ブロックの SRS 回転補正つきのデータ定義
    static constexpr auto T = convert(blocks::T) + common_kick;
    static constexpr auto Z = convert<2, 4, 4>(blocks::Z, {{0, 1, 0, 1}}, {{
      {{0, 0}},
      {{1, 0}},
      {{0, -1}},
      {{0, 0}}
    }}) + common_kick;
    static constexpr auto S = convert<2, 4, 4>(blocks::S, {{0, 1, 0, 1}}, {{
      {{0, 0}},
      {{1, 0}},
      {{0, -1}},
      {{0, 0}}
    }}) + common_kick;
    static constexpr auto J = convert(blocks::J) + common_kick;
    static constexpr auto L = convert(blocks::L) + common_kick;
    static constexpr auto O = convert(blocks::O) + no_rotation;
    static constexpr auto I = convert<2, 4, 4>(blocks::I, {{0, 1, 0, 1}}, {{
      {{0, 0}},
      {{0, -1}},
      {{-1, 0}},
      {{0, 0}}
    }}) + I_kick;
    SRS() = delete;
  };
}