#include <Windows.h>
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <locale>      //  ←★ 追加
#include <clocale>     //  ←★ 追加
#include <io.h>        //  _setmode
#include <fcntl.h>     //  _O_BINARY, _O_U8TEXT
#include "PPT1Mem.h"

extern void RunBot(int playerIndex);   // ai_runner.cpp

//----------------------------------
//  ゲームウィンドウを前面へ
//----------------------------------
static bool BringGameToFront()
{
    HWND hwnd = nullptr;
    EnumWindows([](HWND h, LPARAM p) -> BOOL {
        wchar_t title[256] = {};
        GetWindowTextW(h, title, 256);
        if (wcsstr(title, L"PuyoPuyoTetris") != nullptr || wcsstr(title, L"ぷよぷよ") != nullptr) {
            *reinterpret_cast<HWND*>(p) = h;
            return FALSE;
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&hwnd));

    if (!hwnd) return false;
    if (IsIconic(hwnd)) ShowWindow(hwnd, SW_RESTORE);
    return SetForegroundWindow(hwnd);
}

//----------------------------------
//  エントリーポイント
//----------------------------------
int main(int argc, wchar_t* argv[])
{
    //----------------------------------------------------------------
    // ★ 文字化け対策：コンソールを UTF‑8 初期化 + ロケール生成の例外回避
    //----------------------------------------------------------------
    ::SetConsoleOutputCP(CP_UTF8);

    // C ランタイムのロケールを環境変数に合わせる（"LANG" など）
    const char* c_loc = std::setlocale(LC_ALL, "");

    // C++ ストリーム側のロケール：MinGW では "" が無効な場合があるので例外安全に
    try {
        std::locale::global(std::locale(c_loc ? c_loc : "C"));
    } catch (...) {
        // LANG 未設定などで失敗したら classic() にフォールバック
        std::locale::global(std::locale::classic());
    }

    // UTF‑8 変換ファセットを imbue（global で設定済みなら再度同じロケールを取得）
    std::wcout.imbue(std::locale());
    std::wcerr.imbue(std::locale());

    // MSYS2 mintty ではバイナリモードで UTF‑8 をそのまま吐く
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stderr), _O_U8TEXT);(std::locale());             // wide → narrow 変換を UTF‑8 に
    std::wcerr.imbue(std::locale());
    _setmode(_fileno(stdout), _O_U8TEXT);        // MSYS2: UTF‑8 バイト列を書き出す
    _setmode(_fileno(stderr), _O_U8TEXT);

    //----------------------------------------------------------------
    // コマンドライン解析
    //----------------------------------------------------------------
    int manualPlayer = -1;
    for (int i = 1; i < argc; ++i) {
        if (std::wstring_view(argv[i]) == L"--player" && i + 1 < argc) {
            manualPlayer = _wtoi(argv[++i]);
        }
    }

    //----------------------------------------------------------------
    // プロセス検出
    //----------------------------------------------------------------
    if (!PPT1Mem::FindProcessHandle()) {
        std::wcerr << L"[ERROR] ぷよテトのプロセスが見つからない、またはハンドル取得失敗\n"
                   << L"        (管理者権限で実行 & ゲームを起動済みか確認)\n";
        return 0;
    }
    std::wcout << L"[OK] プロセスハンドル取得成功\n";

    //----------------------------------------------------------------
    // ゲームウィンドウを最前面化
    //----------------------------------------------------------------
    if (!BringGameToFront()) {
        std::wcout << L"[WARN] ゲームウィンドウを検出できませんでした。手動でフォーカスしてください。\n";
    }

    //----------------------------------------------------------------
    // プレイヤー番号決定
    //----------------------------------------------------------------
    int me;
    if (manualPlayer >= 0 && manualPlayer <= 3) {
        me = manualPlayer;
        std::wcout << L"[INFO] --player 指定によりプレイヤー番号 " << me << L" を使用\n";
    } else {
        me = PPT1Mem::FindPlayer();
        std::wcout << L"[INFO] 自動検出したプレイヤー番号: " << me << L"\n";
    }

    //----------------------------------------------------------------
    // メインループへ
    //----------------------------------------------------------------
    RunBot(me);

    PPT1Mem::CloseProcessHandle();
    return 0;
}