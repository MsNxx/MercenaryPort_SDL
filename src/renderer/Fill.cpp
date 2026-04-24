// Sky/ground fill

#include "renderer/Fill.h"
#include "renderer/FrameBuffer.h"
#include "renderer/LogMath.h"

#include <cstring>

constexpr uint8_t GROUND = 10;
constexpr uint8_t SKY = 9;
constexpr uint32_t FOV_BASE = 0x00070000;
constexpr uint32_t HORIZON_X_SCALE = 0x000610C0;
constexpr uint32_t HORIZON_Y_SCALE = 0x000713E0;
constexpr int32_t SLOPE_CLAMP = 0x0FFFFFFF;		  // max 16.16 slope magnitude
constexpr int32_t SWEEP_START_LEFT = 0x00010000;  // 16.16 fixed: x=1
constexpr int32_t SWEEP_START_RIGHT = 0x013F0000; // 16.16 fixed: x=319

// Keep exponent, replace mantissa
static uint32_t WithMantissa(uint32_t val, uint16_t mantissa)
{
	return (val & 0xFFFF0000u) | mantissa;
}

static uint32_t NegateMantissa(uint32_t val)
{
	return WithMantissa(val, -static_cast<int16_t>(val));
}

// XOR and subtract grid mask from mantissa
static uint32_t GridMaskAdjust(uint32_t val, uint16_t mask)
{
	return WithMantissa(val, (static_cast<uint16_t>(val) ^ mask) - mask);
}

// Log-divide: log(a) - log(b) -> antilog
static uint32_t LogDivRaw(uint32_t a, uint32_t b)
{
	uint32_t diff =
		WithMantissa(a, LogLookup(a)) - WithMantissa(b, LogLookup(b));
	return WithMantissa(diff, AntiLogLookup(diff));
}

// Log-multiply: log(a) + log(b) -> antilog
static uint32_t LogAddRaw(uint32_t a, uint32_t b)
{
	uint32_t sum =
		WithMantissa(a, LogLookup(a)) + WithMantissa(b, LogLookup(b));
	return WithMantissa(sum, AntiLogLookup(sum));
}

static uint16_t FixedToInt(int32_t fixed16)
{
	return static_cast<uint16_t>(fixed16 >> 16);
}

// Log-float to linear integer
// Returns 0 on negative input, sets overflow if exponent too large
static int32_t LogFloatToLinear(uint32_t val, bool &overflow)
{
	overflow = false;
	if (static_cast<int32_t>(val) < 0)
	{
		return 0;
	}

	int16_t shift = 14 - static_cast<int16_t>(val >> 16);
	if (shift < 0)
	{
		overflow = true;
		return 0;
	}

	int32_t result = static_cast<int16_t>(val & 0xFFFF);
	return (result << 2) >> shift;
}

static int32_t LinearToSlope(int32_t val) { return (val << 16) >> 7; }

static void FillScanline(uint8_t *row, uint8_t colour)
{
	memset(row, colour, FB_WIDTH);
}

static void FillScanlineSplit(uint8_t *row, int splitX, uint8_t colourLeft,
							  uint8_t colourRight)
{
	if (splitX <= 0)
	{
		FillScanline(row, colourRight);
		return;
	}
	if (splitX >= FB_WIDTH)
	{
		FillScanline(row, colourLeft);
		return;
	}
	memset(row, colourLeft, splitX);
	memset(row + splitX, colourRight, FB_WIDTH - splitX);
}

// When the horizon is entirely off-screen, determine whether to
// invert sky/ground based on the camera's pitch, roll, and tilt
static bool ShouldInvertUniformFill(uint8_t pitchHi, uint8_t rollHi,
									int32_t cotRollSign)
{
	const uint16_t rollBias = (rollHi + 3) >> 1;
	uint16_t combined = pitchHi ^ rollBias;
	combined = (combined >> 1) | (combined << 15); // rotate right 1
	combined ^= static_cast<uint16_t>(cotRollSign);
	return static_cast<int16_t>(combined) >= 0;
}

// Sweep a tilted horizon boundary across consecutive scanlines
static void SweepFill(uint8_t *indexBuf, int startY, int count,
					  int32_t boundaryX, int32_t stepPerLine, uint8_t colLeft,
					  uint8_t colRight)
{
	for (int i = 0; i < count; i++)
	{
		const int y = startY + i;
		if (y >= VIEWPORT_H)
		{
			break;
		}

		boundaryX += stepPerLine;
		const uint16_t bx = FixedToInt(boundaryX);

		if (bx > FB_WIDTH - 1)
		{
			const uint8_t fillCol = (bx < 0x8000) ? colLeft : colRight;
			for (int ry = y; ry < VIEWPORT_H; ry++)
			{
				FillScanline(indexBuf + ry * FB_WIDTH, fillCol);
			}
			return;
		}

		FillScanlineSplit(indexBuf + y * FB_WIDTH, bx, colLeft, colRight);
	}
}

void FillViewport(uint8_t *indexBuf, const Camera &cam)
{
	if (cam.renderMode != 0)
	{
		for (int y = 0; y < VIEWPORT_H; y++)
		{
			FillScanline(indexBuf + y * FB_WIDTH, GROUND);
		}
		return;
	}

	const uint8_t pitchHi = static_cast<uint8_t>(cam.pitch >> 8);
	const uint8_t rollHi = static_cast<uint8_t>(cam.roll >> 8);

	bool coloursInverted =
		(((pitchHi + 1) ^ rollHi ^ cam.horizonMirrorMask) & 2) != 0;

	const uint32_t adjustedCotRoll =
		GridMaskAdjust(cam.cotRoll, cam.horizonMirrorMask);
	const int16_t cotRollMantissa = static_cast<int16_t>(adjustedCotRoll);
	const int32_t cotRollSign = -static_cast<int32_t>(cotRollMantissa);

	// Horizon slope: per-scanline X step in 16.16 fixed-point
	const uint32_t slopeLogFloat = NegateMantissa(adjustedCotRoll) + FOV_BASE;
	bool slopeOverflow;
	const int32_t slopeLinear = LogFloatToLinear(slopeLogFloat, slopeOverflow);
	int32_t horizonSlope;
	if (slopeOverflow)
	{
		horizonSlope = (cotRollSign >= 0) ? SLOPE_CLAMP : -SLOPE_CLAMP;
	}
	else
	{
		horizonSlope = LinearToSlope(slopeLinear);
	}

	// Horizon X: where the boundary crosses the screen horizontally
	const uint32_t tanPitchFov = cam.tanPitch + FOV_BASE;
	const uint32_t horizonXLog = GridMaskAdjust(
		LogFloatAdd(LogDivRaw(NegateMantissa(tanPitchFov), cam.sinRoll),
					LogDivRaw(HORIZON_X_SCALE, cam.tanRoll)),
		cam.horizonMirrorMask);

	bool xOverflow;
	const int32_t horizonX =
		LogFloatToLinear(horizonXLog, xOverflow) + FB_WIDTH / 2;

	const uint8_t colLeft = coloursInverted ? GROUND : SKY;
	const uint8_t colRight = coloursInverted ? SKY : GROUND;

	// Diagonal sweep across full viewport
	if (!xOverflow && horizonX >= 0 && horizonX < FB_WIDTH)
	{
		SweepFill(indexBuf, 0, VIEWPORT_H, (FB_WIDTH - 1 - horizonX) << 16,
				  -horizonSlope, colLeft, colRight);
		return;
	}

	// Horizon X off-screen -- compute horizon Y instead
	const uint32_t yCorrection =
		(cotRollSign >= 0) ? NegateMantissa(HORIZON_Y_SCALE) : HORIZON_Y_SCALE;
	const uint32_t horizonYLog = LogFloatAdd(
		LogDivRaw(tanPitchFov, cam.cosRoll),
		LogAddRaw(yCorrection,
				  GridMaskAdjust(cam.tanRoll, cam.horizonMirrorMask)));

	bool yOverflow;
	const int32_t horizonY =
		LogFloatToLinear(horizonYLog, yOverflow) + VIEWPORT_H / 2;

	// Horizontal split: uniform region above, swept region below
	if (!yOverflow && horizonY >= 0 && horizonY < VIEWPORT_H)
	{
		const int uniformLines = VIEWPORT_H - horizonY;
		const int sweepLines = horizonY;

		const bool topInverted =
			(cotRollSign >= 0) ? !coloursInverted : coloursInverted;

		for (int y = 0; y < uniformLines; y++)
		{
			FillScanline(indexBuf + y * FB_WIDTH, topInverted ? SKY : GROUND);
		}

		const int32_t sweepStart =
			(cotRollSign < 0) ? SWEEP_START_LEFT : SWEEP_START_RIGHT;
		SweepFill(indexBuf, uniformLines, sweepLines, sweepStart, -horizonSlope,
				  colLeft, colRight);
		return;
	}

	// Horizon entirely off-screen -- single colour
	if (ShouldInvertUniformFill(pitchHi, rollHi, cotRollSign))
	{
		coloursInverted = !coloursInverted;
	}

	for (int y = 0; y < VIEWPORT_H; y++)
	{
		FillScanline(indexBuf + y * FB_WIDTH, coloursInverted ? SKY : GROUND);
	}
}
