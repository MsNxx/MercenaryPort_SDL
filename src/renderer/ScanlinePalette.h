#pragma once

// Per-scanline palette state for Timer B raster splits

#include "renderer/FrameBuffer.h"

#include <cstdint>
#include <cstring>

struct ScanlinePaletteState
{
	static constexpr int PALETTE_SIZE = 16;

	// Full 16-colour ARGB palette per viewport scanline
	uint32_t lines[VIEWPORT_H][PALETTE_SIZE];

	// Set all scanlines to a uniform base palette
	void Reset(const uint32_t baseLut[PALETTE_SIZE])
	{
		for (int y = 0; y < VIEWPORT_H; y++)
			std::memcpy(lines[y], baseLut, PALETTE_SIZE * sizeof(uint32_t));
	}

	// Apply a Timer B split: entries 8-11 change at splitLine
	// Scanlines [0, splitLine) use top values
	// Scanlines [splitLine, VIEWPORT_H) use bottom values
	void ApplySplit(int splitLine, uint32_t topE8, uint32_t topE9,
					uint32_t topE10, uint32_t topE11, uint32_t botE8,
					uint32_t botE9, uint32_t botE10, uint32_t botE11)
	{
		int split = splitLine;
		if (split < 0)
			split = 0;
		if (split > VIEWPORT_H)
			split = VIEWPORT_H;

		for (int y = 0; y < split; y++)
		{
			lines[y][8] = topE8;
			lines[y][9] = topE9;
			lines[y][10] = topE10;
			lines[y][11] = topE11;
		}
		for (int y = split; y < VIEWPORT_H; y++)
		{
			lines[y][8] = botE8;
			lines[y][9] = botE9;
			lines[y][10] = botE10;
			lines[y][11] = botE11;
		}
	}

	// Override entries 8-11 for a single scanline
	void SetLine8to11(int line, uint32_t e8, uint32_t e9, uint32_t e10,
					  uint32_t e11)
	{
		if (line >= 0 && line < VIEWPORT_H)
		{
			lines[line][8] = e8;
			lines[line][9] = e9;
			lines[line][10] = e10;
			lines[line][11] = e11;
		}
	}
};
