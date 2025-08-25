//#include "pch.h"
#include "PPT1Mem.h"
#include <Windows.h>
#include <Psapi.h>
#include <string>
#include <tlhelp32.h>
#include <tchar.h>
#include <algorithm>
#include <cstdio>

#ifndef _MSC_VER                    // GCC/Clang など MSVC 以外
  #include <cstdio>
  #define _CRT_WARN 0
  #define _RPT0(level, msg)              std::fprintf(stderr, "%s",  msg)
  #define _RPTWN(level, fmt, ...)        std::fprintf(stderr, fmt, __VA_ARGS__)
#endif

namespace PPT1Mem
{
using namespace PPTDef;


static constexpr TCHAR EXE_NAME[] = TEXT("puyopuyotetris.exe");
static constexpr int PLAYER_MAX = 4;

static HANDLE processHandle = INVALID_HANDLE_VALUE;

static uint64_t matchBaseAddress = 0;
static uint64_t playerBaseAddress[PLAYER_MAX] = {};
static uint64_t playerMainAddress[PLAYER_MAX] = {};
static uint64_t fieldColumnAddress[PLAYER_MAX][10] = {};
static uint64_t nextAddress[PLAYER_MAX] = {};
static uint64_t poppedPieceAddress[PLAYER_MAX] = {};
static uint64_t canHoldAddress[PLAYER_MAX] = {};

static int playerCount = 0;

static uint64_t LobbyPtr()
{
	uint64_t pAddress = 0x140473760;
	uint64_t read = 0;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &read, sizeof(read), nullptr);
	pAddress = read + 0x20;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &read, sizeof(read), nullptr);
	return read;
}
static int32_t SteamPlayerCount()
{
	uint64_t pAddress = LobbyPtr() + 0xB4;
	int32_t ret = 0;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &ret, sizeof(ret), nullptr);
	return std::max(0, std::min(4, ret));
}
static int32_t LocalSteam()
{
	uint64_t pAddress = 0x1405A2010;
	int32_t ret = 0;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &ret, sizeof(ret), nullptr);
	return ret;
}
static int32_t PlayerSteam(int playerIndex)
{
	uint64_t pAddress = LobbyPtr() + 0x118 + static_cast<uint64_t>(playerIndex) * 0x50;
	int32_t ret = 0;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &ret, sizeof(ret), nullptr);
	return ret;
}
static inline uint64_t GetPlayerOffset(int playerIndex) { return 0x378 + static_cast<uint64_t>(playerIndex) * 0x8; }
static void MemorizeMatchBaseAddress()
{
	uint64_t pAddress = 0x140461B20;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &matchBaseAddress, sizeof(matchBaseAddress), nullptr);
}
static void MemorizePlayerBaseAddress(int playerIndex)
{
	uint64_t pAddress = matchBaseAddress + GetPlayerOffset(playerIndex);
	uint64_t read = 0;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &read, sizeof(read), nullptr);
	playerBaseAddress[playerIndex] = read;
}
static void MemorizePlayerMainAddress(int playerIndex)
{
	uint64_t pAddress = playerBaseAddress[playerIndex] + 0xA8;
	uint64_t read = 0;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &read, sizeof(read), nullptr);
	playerMainAddress[playerIndex] = read;
}
static uint64_t GetFieldAddress(int playerIndex)
{
	uint64_t pAddress = playerMainAddress[playerIndex] + 0x3C0;
	uint64_t read = 0;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &read, sizeof(read), nullptr);
	pAddress = read + 0x18;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &read, sizeof(read), nullptr);
	return read;
}
static void MemorizeFieldColumnAddress(int playerIndex)
{
	uint64_t fieldAddress = GetFieldAddress(playerIndex);
	ReadProcessMemory(processHandle, (LPCVOID)fieldAddress, fieldColumnAddress[playerIndex], sizeof(fieldColumnAddress[playerIndex]), nullptr);
}
static void MemorizeNextAddress(int playerIndex)
{
	uint64_t pAddress = playerBaseAddress[playerIndex] + 0xB8;
	uint64_t read = 0;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &read, sizeof(read), nullptr);
	nextAddress[playerIndex] = read + 0x15C;
}
static void MemorizePoppedPieceAddress(int playerIndex)
{
	uint64_t pAddress = playerBaseAddress[playerIndex] + 0xC0;
	uint64_t read = 0;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &read, sizeof(read), nullptr);
	pAddress = read + 0x120;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &read, sizeof(read), nullptr);
	poppedPieceAddress[playerIndex] = read + 0x110;
}
static void MemorizeCanHoldAddress(int playerIndex)
{
	uint64_t pAddress = playerBaseAddress[playerIndex] + 0x40;
	uint64_t read = 0;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &read, sizeof(read), nullptr);
	canHoldAddress[playerIndex] = read + 0x148;
}
static uint64_t GetCurrentStructAddress(int playerIndex)
{
	uint64_t pAddress = playerMainAddress[playerIndex] + 0x3C8;
	uint64_t read = 0;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &read, sizeof(read), nullptr);
	// �J���Ƃ�����z�[���h���Ă���u�ԂƂ���0�ɂȂ�
	return read;
}
uint64_t Debug_GetCurrentStructAddress(int index)//追加
{
    return GetCurrentStructAddress(index);
}

static uint64_t GetHoldStructAddress(int playerIndex)
{
	uint64_t pAddress = playerMainAddress[playerIndex] + 0x3D0;
	uint64_t read = 0;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &read, sizeof(read), nullptr);
	return read;
}

uint64_t Debug_GetPlayerMainAddress(int index)//追加
    {
        // playerMainAddress はこの cpp 内で static 宣言されている変数
        return playerMainAddress[index];
    }
}

bool PPT1Mem::FindProcessHandle()
{
	CloseProcessHandle();

	// �N�����Ă���S�v���Z�X�̏����擾
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		_RPT0(_CRT_WARN, "Error: hSnapshot is INVALID_HANDLE_VALUE\n");
		return false;
	}

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);

	// �v���Z�X�̎擾 (����̂�Process32First)
	if (!Process32First(hSnapshot, &pe)) {
		CloseHandle(hSnapshot);
		_RPT0(_CRT_WARN, TEXT("Error: couldn't get entry by Process32First\n"));
		return false;
	}
	do {
		// ���s�t�@�C�����Ŕ�r
		if (_tcscmp(pe.szExeFile, EXE_NAME) == 0) {
			// �Ώۂ̃v���Z�X�𔭌�
			CloseHandle(hSnapshot);
			//HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
			HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
			if (hProcess) {
				processHandle = hProcess;
				return true;
			}
			else {
				_RPT0(_CRT_WARN, TEXT("Error: couldn't OpenProcess\n"));
			}
			return false;
		}

	} while (Process32Next(hSnapshot, &pe));

	// �n���h�������
	CloseHandle(hSnapshot);

	_RPTWN(_CRT_WARN, TEXT("Error: %s is not found\n"), EXE_NAME);
	return false;
}
void PPT1Mem::CloseProcessHandle()
{
	if (processHandle == INVALID_HANDLE_VALUE)
	{
		return;
	}

	CloseHandle(processHandle);
	processHandle = INVALID_HANDLE_VALUE;
}
bool PPT1Mem::IsOnline()
{
	uint64_t pAddress = 0x14059894C;
	int8_t read = 0;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &read, sizeof(read), nullptr);
	return read == 1;
}
int PPT1Mem::FindPlayer()
{
	if (SteamPlayerCount() < 2)  // In Challenge mode we are the only player.
		return 0;

	int32_t localSteam = LocalSteam();

	for (int i = 0; i < 4; i++)
		if (localSteam == PlayerSteam(i))
			return i;                       // Find who we are by comparing Steam ID.

	return 0;   // Failed to find
}
int32_t PPT1Mem::GetAppFrameCount()
{
	uint64_t pAddress = 0x140461B7C;
	int32_t ret = 0;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &ret, sizeof(ret), nullptr);
	return ret;
}
int PPT1Mem::MemorizeMatchAddress()
{
	MemorizeMatchBaseAddress();
	if (matchBaseAddress == 0)
	{
		playerCount = 0;
		playerBaseAddress[0] = 0;
		return 0;
	}

	uint64_t prevAddress = playerBaseAddress[0];
	MemorizePlayerBaseAddress(0);
	if (prevAddress == playerBaseAddress[0])
	{
		// 1P�݂̂Ŕ�������Ă���
		return 1;
	}

	for (int playerIndex = 1; playerIndex < PLAYER_MAX; ++playerIndex)
	{
		MemorizePlayerBaseAddress(playerIndex);
	}

	playerCount = 0;

	for (int playerIndex = 0; playerIndex < PLAYER_MAX; ++playerIndex)
	{
		if (playerBaseAddress[playerIndex] == 0)
		{
			break;
		}
		++playerCount;

		MemorizePlayerMainAddress(playerIndex);
		MemorizeFieldColumnAddress(playerIndex);
		MemorizeNextAddress(playerIndex);
		MemorizePoppedPieceAddress(playerIndex);
		MemorizeCanHoldAddress(playerIndex);
	}

	return 2;
}
int PPT1Mem::PlayerCount()
{
	return playerCount;
}
int32_t PPT1Mem::GetMatchFrameCount()
{
	uint64_t pAddress = matchBaseAddress + 0x424;
	int32_t ret = 0;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &ret, sizeof(ret), nullptr);
	return ret;
}
bool PPT1Mem::IsOpening()
{
	uint64_t pAddress = poppedPieceAddress[0];
	int8_t read = 0;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &read, sizeof(read), nullptr);
	// �Ō�ɏo�������l�N�X�g�~�m�͊J���̂Ƃ�0xFF
	// ��1P�Ŕ���
	return read == -1;
}
void PPT1Mem::GetField(int playerIndex, Field_t& field)
{
	for (int x = 0; x < 10; ++x)
		ReadProcessMemory(processHandle, (LPCVOID)fieldColumnAddress[playerIndex][x], &field[x], sizeof(field[0]), nullptr);
}
void PPT1Mem::GetCurrent(int playerIndex, PPTDef::Current& current)
{
	uint64_t currentStructAddress = GetCurrentStructAddress(playerIndex);
	if (currentStructAddress == 0)
	{
		current.type = Type::Nothing;
		return;
	}
	uint64_t pAddress = currentStructAddress + 0x8;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &current, sizeof(current), nullptr);
}
void PPT1Mem::GetNext(int playerIndex, Next_t& next)
{
	ReadProcessMemory(processHandle, (LPCVOID)nextAddress[playerIndex], next, sizeof(next), nullptr);
}
PPTDef::Type PPT1Mem::GetHold(int playerIndex)
{
	uint64_t holdStructAddress = GetHoldStructAddress(playerIndex);
	if (holdStructAddress < 0x0800000) return Type::Nothing; // hold is empty
	Type ret = Type::Nothing;
	uint64_t pAddress = holdStructAddress + 0x8;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &ret, sizeof(ret), nullptr);
	return ret;
}
bool PPT1Mem::CanHold(int playerIndex)
{
	uint64_t pAddress = canHoldAddress[playerIndex];
	int8_t ret = 0;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &ret, sizeof(ret), nullptr);
	return ret == 0;
}
void PPT1Mem::GetComboB2B(int playerIndex, ComboB2B& comboB2B)
{
	uint64_t pAddress = playerMainAddress[playerIndex] + 0x3DC;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &comboB2B, sizeof(comboB2B), nullptr);
}
int8_t PPT1Mem::GetClearedLine(int playerIndex)
{
	uint64_t pAddress = playerMainAddress[playerIndex] + 0x2F8;
	int8_t ret = 0;
	ReadProcessMemory(processHandle, (LPCVOID)pAddress, &ret, sizeof(ret), nullptr);
	return ret;
}
PPTDef::CharacterSelectionPPT1 PPT1Mem::GetCharacterSelection(int playerIndex)
{
	uint64_t pAddress = 0x140460690;
	uint64_t base = 0;
	ReadProcessMemory(processHandle, (PVOID)pAddress, &base, sizeof(base), nullptr);

	CharacterSelectionPPT1 ret;
	{
		pAddress = base + 0x274;
		int8_t read = 0;
		ReadProcessMemory(processHandle, (PVOID)pAddress, &read, sizeof(read), nullptr);
		ret.isActive = read != 0;
		if (!ret.isActive)
		{
			return ret;
		}
	}
	{
		pAddress = base + 0x1B8 + playerIndex * 0x30;
		StagePPT1 read = StagePPT1::Char;
		ReadProcessMemory(processHandle, (PVOID)pAddress, &read, sizeof(read), nullptr);
		ret.stage = read;
	}
	{
		pAddress = base + 0x1C8 + playerIndex * 0x30;
		CharacterPPT1 read = CharacterPPT1::Ringo;
		ReadProcessMemory(processHandle, (PVOID)pAddress, &read, sizeof(read), nullptr);
		ret.character = read;
	}
	{
		pAddress = base + 0x980 + playerIndex * 0x28;
		PuyoOrTetris read = PuyoOrTetris::PuyoPuyo;
		ReadProcessMemory(processHandle, (PVOID)pAddress, &read, sizeof(read), nullptr);
		ret.puyoOrTetris = read;
	}
	return ret;
}

