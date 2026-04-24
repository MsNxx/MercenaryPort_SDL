#pragma once

// Benson text display

#include <cstdint>

struct Audio;

// Text area layout
constexpr int BENSON_TEXT_X = 64;	  // leftmost pixel
constexpr int BENSON_TEXT_Y = 175;	  // top scanline
constexpr int BENSON_TEXT_CHARS = 23; // characters per visible line
constexpr int BENSON_CHAR_W = 8;
constexpr int BENSON_CHAR_H = 7;
constexpr int BENSON_TEXT_W = BENSON_TEXT_CHARS * BENSON_CHAR_W; // 184
constexpr int BENSON_NEW_CHAR_X = 240;

enum BensonState
{
	BENSON_IDLE,
	BENSON_SETUP,
	BENSON_ACTIVE,
};

struct Benson
{
	BensonState state;
	const char *text;
	int textPos;
	int frameCount;
	uint16_t textSpeed; // frames per text line

	// BCD number expansion -- char >= 'a' triggers variable lookup
	bool bcdActive;
	uint32_t bcdValue;
	int bcdShift;	 // current shift (28, 24, 20, ... 0)
	bool bcdLeading; // skipping leading zeros

	const uint32_t *varTable; // ScriptVM's variable table for BCD lookups
};

void BensonInit(Benson &benson);
void BensonDisplay(Benson &benson, const char *text);
void BensonSetSpeed(Benson &benson, uint16_t speed);

// Scroll copies from prevBuf to indexBuf with a left shift
bool BensonTick(Benson &benson, uint8_t *indexBuf, uint8_t *prevBuf,
				Audio *audio = nullptr);
