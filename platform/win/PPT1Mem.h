#pragma once

#include "PPTDef.h"
#include <cstdint>

namespace PPT1Mem
{
uint64_t Debug_GetPlayerMainAddress(int index);//追加
 uint64_t Debug_GetCurrentStructAddress(int index);//追加
extern bool FindProcessHandle();
extern void CloseProcessHandle();

extern bool IsOnline();
extern int FindPlayer();

extern int32_t GetAppFrameCount();

// �������̃A�h���X���L������
// ���j���[:0, ������:1, �������A�h���X�ω�:2
extern int MemorizeMatchAddress();

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// MemorizeMatchAddress�ˑ�

extern int PlayerCount();
extern int32_t GetMatchFrameCount();
extern bool IsOpening();
extern void GetField(int playerIndex, PPTDef::Field_t& field);
extern void GetCurrent(int playerIndex, PPTDef::Current& current);
extern void GetNext(int playerIndex, PPTDef::Next_t& next);
extern PPTDef::Type GetHold(int playerIndex);
extern bool CanHold(int playerIndex);
extern void GetComboB2B(int playerIndex, PPTDef::ComboB2B& comboB2B);
extern int8_t GetClearedLine(int playerIndex);

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

extern PPTDef::CharacterSelectionPPT1 GetCharacterSelection(int playerIndex);
}
