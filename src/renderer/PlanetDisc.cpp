// Intro planet disc -- $041E16-$041FA0, sub_041FA4

#include "renderer/PlanetDisc.h"
#include "data/Gen_MData.h"
#include "renderer/FrameBuffer.h"
#include "renderer/LogMath.h"
constexpr int DISC_CENTER_X = 160;
constexpr int DISC_CENTER_Y = 68;
constexpr int DISC_SCANLINES = 68; // D5 = $0043 + 1

constexpr uint8_t DISC_COLOUR = 10; // intro palette dark green

// Growth factor (log-multiplied with D3 each frame)
constexpr uint32_t GROWTH_FACTOR = 0xFFFF1FC0;
constexpr uint32_t INITIAL_D1 = 0x000D1000;
constexpr uint32_t INITIAL_D3 = 0x0006F000;

// sub_041FA4 constants
constexpr uint32_t SCANLINE_INIT = 0xFFFF1000; // D1 init
constexpr uint32_t SCANLINE_STEP = 0x00001200; // A4 = increment per pair

void PlanetDiscInit(PlanetDisc &disc)
{
	disc.scaleFactor = INITIAL_D3;
	disc.refValue = INITIAL_D1;
	disc.lastHalfWidth = 0;
	disc.complete = false;
}

bool PlanetDiscTick(PlanetDisc &disc, uint8_t *indexBuf)
{
	if (disc.complete)
	{
		return true;
	}

	// Outer loop at $041E38-$041EC0: update D3, compute disc param

	// $041E38-$041E50: D3 = D3 * GROWTH_FACTOR (log-multiply)
	disc.scaleFactor = LogMultiply(disc.scaleFactor, GROWTH_FACTOR);
	uint32_t D3 = disc.scaleFactor;

	// $041E52-$041E58: D2 = D3; sort D2/D1 so D1 >= D2
	uint32_t D1 = disc.refValue;
	uint32_t D2 = D3;
	if (static_cast<int32_t>(D2) >= static_cast<int32_t>(D1))
	{
		uint32_t tmp = D2;
		D2 = D1;
		D1 = tmp;
	}

	// $041E5A-$041EC0: D1 = LogFloatAdd(D1, D2)
	// D2 has negative mantissa -> subtraction
	D1 = LogFloatAdd(D1, D2);

	// Save modified D1 back for next frame
	disc.refValue = D1;

	// $041EC0: ANDI.W #$FFFE,D1
	D1 &= 0xFFFFFFFE;

	// $041EC8-$041EDE: D0 = $000D1000 / D1 (log-divide)
	uint32_t D0 = LogDivide(INITIAL_D1, D1);

	// sub_041FA4: draw the disc using D0 as the scale parameter

	// $041FA4-$041FB0: D0 = D0 * D0 (square D0 via log-domain)
	//   MOVE.W 0(A5,D0.W),D0  ; log(mantissa)
	//   ADD.L  D0,D0           ; log + log = log(D0^2), exp doubled
	//   ROR/AND/antilog
	D0 = LogMultiply(D0, D0);

	// $041FB4-$041FC2: init scanline iteration
	uint32_t scanParam = SCANLINE_INIT; // D1 = $FFFF1000
	uint32_t scanParamSave = scanParam; // D4 = D1

	bool discOverwritesSentinel = false;
	int maxHalfWidth = 0;

	for (int pair = 0; pair < DISC_SCANLINES; pair++)
	{
		// $041FD2-$041FE0: D1 = D1 * D1 (square scanline param)
		uint32_t d1sq = LogMultiply(scanParam, scanParam);

		// $041FE2: NEG.W D1 -- negate mantissa
		d1sq = (d1sq & 0xFFFF0000u) |
			   static_cast<uint16_t>(-static_cast<int16_t>(d1sq & 0xFFFF));

		// $041FE4: D2 = D0 (disc radius squared)
		D2 = D0;

		// $041FE6-$041FEA: sort D2/D1 for LogFloatAdd
		uint32_t addA = d1sq;
		uint32_t addB = D2;
		if (static_cast<int32_t>(addB) >= static_cast<int32_t>(addA))
		{
			uint32_t tmp = addA;
			addA = addB;
			addB = tmp;
		}

		// $041FEC-$042052: D1 = LogFloatAdd(addA, addB)
		// = D0^2 + (-scanParam^2) = R^2 - y^2
		uint32_t circleVal = LogFloatAdd(addA, addB);

		// $042052: ANDI.W #$FFFE
		circleVal &= 0xFFFFFFFE;

		// $042056-$042058: TST.W D1; BMI -> skip (outside disc)
		int16_t cvMant = static_cast<int16_t>(circleVal & 0xFFFF);
		if (cvMant < 0)
		{
			goto advance;
		}

		{
			// $04205C-$042068: sqrt via log-domain halving
			// ASR.L #1 halves both exponent and mantissa -> sqrt
			uint16_t logCV = LogLookup(circleVal & 0xFFFF);
			int32_t fullVal =
				static_cast<int32_t>((circleVal & 0xFFFF0000u) | logCV);
			fullVal >>= 1; // ASR.L #1
			// Now LSR.W #4 on low word
			uint16_t lowW = static_cast<uint16_t>(fullVal & 0xFFFF);
			lowW >>= 4;
			lowW &= 0x0FFE;
			// Antilog
			int aIdx = lowW / 2;
			uint16_t finalMant = 0;
			if (aIdx >= 0 && aIdx < gen_m::ANTILOG_WORDS)
			{
				finalMant = gen_m::ANTILOG_TABLE[aIdx];
			}
			// Reconstruct with the shifted exponent
			uint32_t halfWidthLF =
				(static_cast<uint32_t>(fullVal) & 0xFFFF0000u) | finalMant;

			// $04206C-$042082: LogFloatToScreen conversion
			int32_t halfWidth = LogFloatToScreen(halfWidthLF);
			if (halfWidth == 0x7FFFFFFF)
			{
				halfWidth = 160;
			}
			// $042084: CMPI.W #$00A0 -- clamp to 160
			if (halfWidth > 160)
			{
				halfWidth = 160;
			}
			if (halfWidth < 0)
			{
				halfWidth = 0;
			}
			if (halfWidth > maxHalfWidth)
			{
				maxHalfWidth = halfWidth;
			}

			// Fill scanline pair
			int yUp = DISC_CENTER_Y - 1 - pair;
			int yDown = DISC_CENTER_Y + pair;
			int xLeft = DISC_CENTER_X - halfWidth;
			int xRight = DISC_CENTER_X + halfWidth;
			if (xLeft < 0)
			{
				xLeft = 0;
			}
			if (xRight > FB_WIDTH)
			{
				xRight = FB_WIDTH;
			}

			if (yUp >= 0 && yUp < VIEWPORT_H)
			{
				for (int x = xLeft; x < xRight; x++)
				{
					indexBuf[yUp * FB_WIDTH + x] = DISC_COLOUR;
				}
				// Check if we've overwritten the "sentinel" area
				// (top of framebuffer = scanline 0)
				if (yUp == 0 && halfWidth >= 160)
				{
					discOverwritesSentinel = true;
				}
			}
			if (yDown >= 0 && yDown < VIEWPORT_H)
			{
				for (int x = xLeft; x < xRight; x++)
				{
					indexBuf[yDown * FB_WIDTH + x] = DISC_COLOUR;
				}
			}
		}

	advance:
		// $042120-$0421A0: advance scanline parameter
		// D4 = D4 + A4 (add $00001200 to saved scanline param)
		// D1 = D4
		scanParamSave = LogFloatAdd(scanParamSave, SCANLINE_STEP);
		scanParam = scanParamSave;
	}

	disc.lastHalfWidth = maxHalfWidth;

	if (discOverwritesSentinel)
	{
		disc.complete = true;
		return true;
	}

	return false;
}
