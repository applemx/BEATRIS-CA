cmake_minimum_required(VERSION 3.22)
project(BEATRIS_Cb LANGUAGES CXX)

# ──────────────────────────────────────────────
# 既定設定: Release / C++23 / x64
# ──────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------
# オプション
# ------------------------------------------------------------
option(BEATRIS_SANITIZE        "Enable Address/Undefined sanitizers (Debug only)" OFF)
option(BEATRIS_GUI_SUBSYSTEM   "Build as GUI subsystem (needs WinMain)"          OFF)

# ------------------------------------------------------------
# ルートパス
# ------------------------------------------------------------
set(ROOT_DIR      ${CMAKE_CURRENT_LIST_DIR})
set(INCLUDE_DIR   ${ROOT_DIR}/include)
set(PLATFORM_WIN  ${ROOT_DIR}/platform/win)
set(SRC_DIR       ${ROOT_DIR}/src)
set(EXTERNAL_DIR  ${ROOT_DIR}/external)

#============================================================
# Interface library: 共通ヘッダ & コンパイルフラグ
#============================================================
add_library(beatrisc_core INTERFACE)

target_include_directories(beatrisc_core INTERFACE
    ${INCLUDE_DIR}
    ${PLATFORM_WIN}
    ${EXTERNAL_DIR}/include)

# ---- コンパイラ別フラグ -----------------------------------
if(MSVC OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
            AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))
    target_compile_options(beatrisc_core INTERFACE
        /O2 /arch:AVX2 /permissive- /Zc:__cplusplus /bigobj /utf-8 /EHsc)
    target_compile_definitions(beatrisc_core INTERFACE __BMI2__=1)
else()
    target_compile_options(beatrisc_core INTERFACE
        -O3 -march=native -mbmi2 -mfma
        -Wall -Wextra -Wshadow -Wpedantic -pipe)
endif()

# ---- Sanitizer (Debug 限定) -------------------------------
if(BEATRIS_SANITIZE AND CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT MSVC)
    target_compile_options(beatrisc_core INTERFACE -fsanitize=address,undefined)
    target_link_options   (beatrisc_core INTERFACE -fsanitize=address,undefined)
endif()

#============================================================
# Executable: beatriz
#============================================================
set(SRC
    ${SRC_DIR}/main.cpp
    ${INCLUDE_DIR}/ai_runner.cpp
    ${PLATFORM_WIN}/PPT1Mem.cpp
)

# GUI / Console サブシステム切替
if(BEATRIS_GUI_SUBSYSTEM)
    add_executable(beatriz WIN32 ${SRC})      # GUI: WinMain 必須
    if(MINGW)
        target_link_options(beatriz PRIVATE "-Wl,--subsystem,windows")
    endif()
else()
    add_executable(beatriz ${SRC})            # Console: main() を使う
    if(MINGW)
        target_link_options(beatriz PRIVATE "-Wl,--subsystem,console")
    endif()
endif()

target_link_libraries(beatriz PRIVATE beatrisc_core)

# 出力先を bin/ に固定
set_target_properties(beatriz PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

#============================================================
# ビルド概要表示
#============================================================
message(STATUS "\n─ Build summary ─────────────────────────────")
message(STATUS "  CMAKE_BUILD_TYPE      : ${CMAKE_BUILD_TYPE}")
message(STATUS "  CMAKE_CXX_COMPILER    : ${CMAKE_CXX_COMPILER}")
message(STATUS "  GUI subsystem (WinMain): ${BEATRIS_GUI_SUBSYSTEM}")
