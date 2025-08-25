#pragma once

#include <cstdint>

namespace PPTDef
{
enum class CharacterPPT1 : int8_t
{
	Ringo,
	Risukuma,
	Schezo,
	O,
	Tee,

	Maguro,
	Amitie,
	Feli,
	Ai,
	Ess,

	Klug,
	Sig,
	DracoCentauros,
	Zed,
	JayAndElle,

	ArleAndCarbuncle,
	Suketoudara,
	DarkPrince,
	Rulue,
	Raffina,

	Witch,
	Ecolo,
	Ex,
	Lemres
};
enum class CharacterPPT2 : int8_t
{
	Ringo,
	Arle,
	Amitie,
	Maguro,
	Carbuncle,
	Tee,
	O,
	Ess,
	Ai,
	JayAndElle,

	Sig,
	Risukuma,
	Suketoudara,
	Witch,
	DracoCentauros,
	Zed,
	Klug,
	Lemres,
	Raffina,
	Rulue,

	Schezo,
	Marle,
	Ecolo,
	Ally,
	Sonic,
	Feli,
	DarkPrince,
	Ex,
	Squares,
	MsAccord,

	Lidelle,
	Serilly,
	YuAndRei,
	Harpy,
	Legamunt,
	OceanPrince,
	Rafisol,
	PossessedKlug,
	Ragnus,
	Rozatte
};
enum class StagePPT1 : int8_t
{
	Char = 1,
	PuyoOrTetris,
	ControlType,
	OK,
	Team
};
enum class PuyoOrTetris : int8_t
{
	PuyoPuyo,
	Tetris
};
struct CharacterSelectionPPT1
{
	bool isActive = false;
	StagePPT1 stage = StagePPT1::Char;
	CharacterPPT1 character = CharacterPPT1::Ringo;
	PuyoOrTetris puyoOrTetris = PuyoOrTetris::PuyoPuyo;
};
struct CharacterSelectionPPT2
{
	bool isActive = false;
	CharacterPPT2 character = CharacterPPT2::Ringo;
};

enum class Type : int32_t
{
	Clearing = -2,
	Nothing,
	S,
	Z,
	J,
	L,
	T,
	O,
	I,
	Garbage = 9
};
enum class Rotation : int32_t
{
	Up,
	Right,
	Down,
	Left
};
enum class Locked : int8_t
{
	No,
	Yes
};
struct Current
{
	Type type = Type::Nothing;
	int32_t x = 0;
	int32_t y = 0;
	int32_t distanceToGround = 0;
	Rotation rotation = Rotation::Up;
	Locked locked = Locked::No;
};
struct ComboB2B
{
	int8_t combo = 0;
	int8_t b2b = 0;
};

static constexpr int FIELD_WIDTH = 10;
static constexpr int FIELD_HEIGHT = 40;
static constexpr int NEXT_NUM = 5;

using Field_t = Type[FIELD_WIDTH][FIELD_HEIGHT];
using Next_t = Type[NEXT_NUM];
}
