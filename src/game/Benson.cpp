#include "game/Benson.h"

#include "audio/Audio.h"
#include "data/Gen_MData.h"

#include <cstddef>

#include "renderer/FrameBuffer.h"

// Frames 1-23: one character per frame; 24: buffer sync; 25+: wait
constexpr int CHAR_FRAMES = 23;

// Bitplane-0 helpers -- bit 0 of pixel index = ST bitplane 0

static void SetBit0(uint8_t *indexBuf, int x, int y)
{
	indexBuf[y * FB_WIDTH + x] |= 1;
}

static void ClearBit0(uint8_t *indexBuf, int x, int y)
{
	indexBuf[y * FB_WIDTH + x] &= 0x0E;
}

// Cross-buffer scroll (E1:3933-3968): copy bit 0 left by 8 pixels

static void ScrollLeftCrossBuffer(const uint8_t *src, uint8_t *dst)
{
	for (int row = 0; row < BENSON_CHAR_H; row++)
	{
		const int y = BENSON_TEXT_Y + row;
		const uint8_t *srcLine = src + y * FB_WIDTH;
		uint8_t *dstLine = dst + y * FB_WIDTH;

		// Copy from src shifted left by 8 pixels into dst
		for (int x = BENSON_TEXT_X;
			 x < BENSON_TEXT_X + BENSON_TEXT_W - BENSON_CHAR_W; x++)
		{
			uint8_t srcBit = srcLine[x + BENSON_CHAR_W] & 1;
			dstLine[x] = (dstLine[x] & 0x0E) | srcBit;
		}
		// Clear the rightmost character cell in dst
		for (int x = BENSON_NEW_CHAR_X; x < BENSON_NEW_CHAR_X + BENSON_CHAR_W;
			 x++)
		{
			dstLine[x] &= 0x0E;
		}
	}
}

// Draw a single glyph at the rightmost character cell

static void DrawGlyph(uint8_t *indexBuf, char ch)
{
	int glyphIndex = ch - 0x20;
	const uint8_t *glyph = NULL;

	if (glyphIndex >= 0 && glyphIndex < gen_m::BENSON_GLYPH_COUNT)
	{
		glyph = gen_m::BENSON_FONT + glyphIndex * gen_m::BENSON_GLYPH_STRIDE;
	}

	for (int row = 0; row < BENSON_CHAR_H; row++)
	{
		const int y = BENSON_TEXT_Y + row;
		const uint8_t bits = glyph ? glyph[row] : 0;

		for (int col = 0; col < BENSON_CHAR_W; col++)
		{
			if ((bits >> (7 - col)) & 1)
			{
				SetBit0(indexBuf, BENSON_NEW_CHAR_X + col, y);
			}
			else
			{
				ClearBit0(indexBuf, BENSON_NEW_CHAR_X + col, y);
			}
		}
	}
}

void BensonInit(Benson &benson)
{
	benson.state = BENSON_IDLE;
	benson.text = NULL;
	benson.textPos = 0;
	benson.frameCount = 0;
	benson.textSpeed = 150;
	benson.bcdActive = false;
	benson.bcdValue = 0;
	benson.bcdShift = 0;
	benson.bcdLeading = true;
	benson.varTable = NULL;
}

void BensonDisplay(Benson &benson, const char *text)
{
	benson.text = text;
	benson.textPos = 0;
	benson.frameCount = 0;
	benson.state = BENSON_SETUP;
}

void BensonSetSpeed(Benson &benson, uint16_t speed)
{
	benson.textSpeed = speed;
}

bool BensonTick(Benson &benson, uint8_t *indexBuf, uint8_t *prevBuf,
				Audio *audio)
{
	if (benson.state == BENSON_IDLE)
	{
		return false;
	}

	if (benson.state == BENSON_SETUP)
	{
		benson.state = BENSON_ACTIVE;
		benson.frameCount = 0;
		return true;
	}

	// BENSON_ACTIVE
	benson.frameCount++;

	if (benson.frameCount <= CHAR_FRAMES)
	{
		// Scroll existing text: cross-buffer copy from prevBuf to indexBuf,
		// shifted left by one character (E1:3933-3968)
		ScrollLeftCrossBuffer(prevBuf, indexBuf);

		// Draw the next character at the rightmost position
		// Characters >= 'a' are BCD placeholders, leading zeros skipped
		char ch = ' ';

		if (benson.bcdActive)
		{
			// Extract next significant nibble
			// Skip leading zeros in a loop (no frame consumed)
			while (benson.bcdShift >= 0)
			{
				int nibble = static_cast<int>(
					(benson.bcdValue >> benson.bcdShift) & 0xF);
				benson.bcdShift -= 4;

				if (benson.bcdLeading && nibble == 0 && benson.bcdShift >= 0)
				{
					// Leading zero -- skip without rendering
					continue;
				}

				// Non-zero nibble (or last nibble): render it
				benson.bcdLeading = false;
				ch = static_cast<char>('0' + nibble);
				break;
			}

			if (benson.bcdShift < 0)
				benson.bcdActive = false;
		}
		else if (benson.text != NULL && benson.text[benson.textPos] != '\0')
		{
			ch = benson.text[benson.textPos];
			benson.textPos++;

			// Check for BCD placeholder (>= 'a')
			if (ch >= 'a' && ch <= 'z' && benson.varTable != NULL)
			{
				int slot = ch - 'a';
				benson.bcdValue = benson.varTable[slot];
				benson.bcdShift = 28; // start from MSB nibble
				benson.bcdActive = true;
				benson.bcdLeading = true;

				// Extract the first significant nibble this frame
				while (benson.bcdShift >= 0)
				{
					int nibble = static_cast<int>(
						(benson.bcdValue >> benson.bcdShift) & 0xF);
					benson.bcdShift -= 4;

					if (benson.bcdLeading && nibble == 0 &&
						benson.bcdShift >= 0)
					{
						continue;
					}

					benson.bcdLeading = false;
					ch = static_cast<char>('0' + nibble);
					break;
				}

				if (benson.bcdShift < 0)
					benson.bcdActive = false;
			}
		}

		// $0435A4-$0435CA: unconditional $0C then conditional $00
		// for spaces produces a ~4us click pulse on real hardware
		if (audio != nullptr)
		{
			AudioWriteReg(*audio, YM_TONE_A_FINE, static_cast<uint8_t>(ch * 2));
			AudioWriteReg(*audio, YM_AMP_A, 0x0C);
			if (ch == ' ')
			{
				AudioWriteReg(*audio, YM_AMP_A, 0x00);
			}
		}

		DrawGlyph(indexBuf, ch);
	}
	// Frame 24: VBLHandler does the double-buffer sync (loc_0436AC)
	// Frames 25 to textSpeed-1: wait
	else if (benson.frameCount >= benson.textSpeed)
	{
		// Line done.  Check for more text in the string
		if (benson.text != NULL && benson.text[benson.textPos] != '\0')
		{
			// More characters -- start a new line
			benson.frameCount = 0;
		}
		else
		{
			// String finished -- silence and go idle
			// Matches $04350C-$04351A: amp A = 0 on text end
			if (audio != NULL)
			{
				AudioWriteReg(*audio, YM_AMP_A, 0x00);
			}
			benson.state = BENSON_IDLE;
			benson.text = NULL;
			return false;
		}
	}
	else
	{
		// Frames between CHAR_FRAMES and textSpeed: silence
		// Matches $043548 BGT -> $043514: amp A = 0
		if (audio != NULL && benson.frameCount == CHAR_FRAMES + 1)
		{
			AudioWriteReg(*audio, YM_AMP_A, 0x00);
		}
	}

	return true;
}
