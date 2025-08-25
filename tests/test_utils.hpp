#pragma once
#include <array>
#include <initializer_list>
#include <string_view>
#include <chrono>
#include "../include/board.hpp"

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
// 盤面の幅と高さを定義
constexpr int WIDTH = 10, HEIGHT = 24;

// BOARD型をreachability::board_t<WIDTH, HEIGHT>として定義
using BOARD = reachability::board_t<WIDTH, HEIGHT>;
constexpr auto merge_str(std::initializer_list<std::string_view> &&b_str) {
    std::array<char, WIDTH*HEIGHT> res = {};
    unsigned pos = 0;
    // 盤面の高さが指定された文字列リストのサイズより大きい場合、空白で埋める
    if (b_str.size() < HEIGHT) {
      for (pos = 0; pos < WIDTH * (HEIGHT - b_str.size()); ++pos) {
        res[pos] = ' ';
      }
    }
    // 文字列リストを逆順にしてres配列に格納
    for (auto &s : b_str) {
      for (unsigned i = 0; i < WIDTH; ++i) {
        res[pos + i] = s[WIDTH - i - 1];
      }
      pos += WIDTH;
    }
    // BOARD型の配列に変換して返す
    return BOARD::convert_to_array(std::string_view{res.data(), res.size()});
}