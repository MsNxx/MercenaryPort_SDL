#pragma once

// ST $0RGB palette to ARGB8888 conversion

#include "DevUtils.h"
#include <cstdint>

constexpr int PALETTE_ENTRIES = 16;

// Default outdoor viewport palette (entries 8-11), packed as two ST words
#if SECOND_CITY
constexpr uint32_t PAL_DEFAULT_89 = 0x07770744;	  // 8=white, 9=orange
constexpr uint32_t PAL_DEFAULT_1011 = 0x04000550; // 10=red, 11=amber
#else
constexpr uint32_t PAL_DEFAULT_89 = 0x07770357;	  // 8=white, 9=blue
constexpr uint32_t PAL_DEFAULT_1011 = 0x00400000; // 10=green, 11=black
#endif

uint8_t Expand3to8(uint8_t ch3);

void BuildArgbLut(const uint16_t stWords[PALETTE_ENTRIES],
				  uint32_t argbOut[PALETTE_ENTRIES]);

void ResolveFramebuffer(const uint8_t *indexBuf, int pixelCount,
						const uint32_t argbLut[PALETTE_ENTRIES],
						uint32_t *argbOut);

// Per-scanline palette resolve -- viewport uses scanlineLut, HUD uses baseLut
void ResolveFramebufferScanline(const uint8_t *indexBuf, int width, int height,
								int viewportHeight,
								const uint32_t scanlineLut[][PALETTE_ENTRIES],
								const uint32_t baseLut[PALETTE_ENTRIES],
								uint32_t *argbOut);
