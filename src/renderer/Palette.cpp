#include "renderer/Palette.h"

uint8_t Expand3to8(uint8_t ch3)
{
	return static_cast<uint8_t>((ch3 * 255) / 7);
}

void BuildArgbLut(const uint16_t stWords[PALETTE_ENTRIES],
				  uint32_t argbOut[PALETTE_ENTRIES])
{
	for (int i = 0; i < PALETTE_ENTRIES; i++)
	{
		uint8_t r = Expand3to8((stWords[i] >> 8) & 7);
		uint8_t g = Expand3to8((stWords[i] >> 4) & 7);
		uint8_t b = Expand3to8((stWords[i] >> 0) & 7);
		argbOut[i] = 0xFF000000u | (uint32_t(r) << 16) | (uint32_t(g) << 8) |
					 uint32_t(b);
	}
}

void ResolveFramebuffer(const uint8_t *indexBuf, int pixelCount,
						const uint32_t argbLut[PALETTE_ENTRIES],
						uint32_t *argbOut)
{
	for (int i = 0; i < pixelCount; i++)
	{
		argbOut[i] = argbLut[indexBuf[i] & 0x0F];
	}
}

void ResolveFramebufferScanline(const uint8_t *indexBuf, int width, int height,
								int viewportHeight,
								const uint32_t scanlineLut[][PALETTE_ENTRIES],
								const uint32_t baseLut[PALETTE_ENTRIES],
								uint32_t *argbOut)
{
	const uint8_t *src = indexBuf;
	uint32_t *dst = argbOut;

	// Viewport scanlines: per-line palette
	for (int y = 0; y < viewportHeight && y < height; y++)
	{
		const uint32_t *lut = scanlineLut[y];
		for (int x = 0; x < width; x++)
			*dst++ = lut[*src++ & 0x0F];
	}

	// HUD scanlines: base palette
	for (int y = viewportHeight; y < height; y++)
	{
		for (int x = 0; x < width; x++)
			*dst++ = baseLut[*src++ & 0x0F];
	}
}
