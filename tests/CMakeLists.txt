# tests/CMakeLists.txt   ← ← ここで cmake を回す

#--- ヘッダ -------------------------------------------------
message(STATUS "CXX_COMPILER_ID = ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CXX_SIMULATE_ID = ${CMAKE_CXX_SIMULATE_ID}")
message(STATUS "CXX_COMPILER    = ${CMAKE_CXX_COMPILER}")

cmake_minimum_required(VERSION 3.16)
project(BEATRIS_Cb LANGUAGES CXX)

# ルート (= 1 つ上) への参照をまとめておく
set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)           # ../
set(INCLUDE_DIR ${ROOT_DIR}/include)                 # ../include

#==========================================================
# 共通設定
#==========================================================
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(BEATRIS_SANITIZE "Enable Address/Undefined sanitizers (Debug only)" OFF)

#==========================================================
# インターフェースライブラリ（ヘッダパス共有）
#==========================================================
add_library(beatrisc_core INTERFACE)
target_include_directories(beatrisc_core INTERFACE
    ${INCLUDE_DIR}                # ヘッダ本体
    ${CMAKE_CURRENT_LIST_DIR}     # tests/ 内のヘッダ・ソース
)
target_compile_features(beatrisc_core INTERFACE cxx_std_23)

#==========================================================
# コンパイラ別フラグ
#==========================================================
if(MSVC OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
            AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))
    target_compile_options(beatrisc_core INTERFACE
        /O2 /arch:AVX2 /permissive- /Zc:__cplusplus /bigobj
    )
    target_compile_definitions(beatrisc_core INTERFACE __BMI2__=1)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    target_compile_options(beatrisc_core INTERFACE
        -O3 -march=native -mbmi2 -mfma -Wall -Wextra -Wshadow -Wno-unknown-attributes -pipe
    )
endif()

if(BEATRIS_SANITIZE AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU"))
    target_compile_options(beatrisc_core INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(beatrisc_core INTERFACE    -fsanitize=address,undefined)
endif()

#==========================================================
# テスト用実行ファイル
#==========================================================
add_executable(test_board test_board.cpp)   # ← パスは tests/ 直下なので短く
target_link_libraries(test_board PRIVATE beatrisc_core)
set_target_properties(test_board PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 追加テストの例
# add_executable(test_lineclear test_lineclear.cpp)
# target_link_libraries(test_lineclear PRIVATE beatrisc_core)
# set_target_properties(test_lineclear PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

