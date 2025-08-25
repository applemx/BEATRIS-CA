#pragma once
#include <type_traits>
#include <utility>
#include <span>
#include <algorithm>
namespace reachability {

  // コンパイル時ループを実装するための `static_for`
  // `F`: 各ループ回で実行する関数
  // `S...`: ループ回数を表す `std::index_sequence`
  template<typename F, std::size_t... S>
  [[gnu::always_inline]]
  constexpr void static_for(F&& function, std::index_sequence<S...>) {
      int unpack[] = {0,
          (void(function(std::integral_constant<std::size_t, S>{})), 0)...
      };

      (void) unpack;// 配列の未使用警告を抑制
  }
  // ループ回数を指定して `static_for` を実行
  //`iterations`: 繰り返し回数
  //`F`: 実行する関数
  template<std::size_t iterations, typename F>
  [[gnu::always_inline]]
  constexpr void static_for(F&& function) {
      static_for(std::forward<F>(function), std::make_index_sequence<iterations>());
  }
  // `static_vector` クラス: 固定長のコンパイル時配列を提供
  // - `T`: 配列の要素型
  // - `N`: 配列の最大サイズ
  template <typename T, std::size_t N>
  struct static_vector {
    static constexpr std::size_t capacity = N;// 最大容量 (コンパイル時決定)
    T data[N];// 内部データ配列
    std::size_t used;// 実際に使用されている要素数

    // `std::span` を用いて初期化するコンストラクタ
    // - `M` は `std::span` のサイズ (N以下である必要がある)
    template <std::size_t M>
    constexpr static_vector(std::span<T, M> arr): used(M) {
      static_assert(M <= N);
      std::copy(arr.begin(), arr.end(), data);// 配列をコピー
    }
     // デフォルトコンストラクタは禁止 (明示的な初期化を強制)
    static_vector() = delete;
    // ベクターの要素数を取得
    constexpr std::size_t size() const {
      return used;
    }
    // `operator[]` を用いた要素アクセス (読み取り専用)
    constexpr const T &operator[](std::size_t i) const {
      return data[i];
    }
    // `operator[]` を用いた要素アクセス (書き込み可能)
    constexpr T &operator[](std::size_t i) {
      return data[i];
    }
  };
}