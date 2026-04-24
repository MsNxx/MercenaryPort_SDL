// Intro starfield -- sub_0426CC, sub_0424E0 (E1)

#include "renderer/StarField.h"
#include "renderer/LogMath.h"

#include "renderer/FrameBuffer.h"
constexpr uint8_t SKY_COLOUR = 8;

// Initial cameraZ from $04244C: MOVE.L #$0000E000,($0624D8)
constexpr uint32_t INITIAL_CAMERA_Z = 0x0000E000;

// Initial frequency counter from E2 workspace
// sub_0424E0 decrements by 4 each frame from $0700 toward $0400
constexpr uint16_t INITIAL_FREQ_B = 0x0700; // $0624D4: channel B
constexpr uint16_t INITIAL_FREQ_C = 0x0780; // $0624D6: channel C

// Camera advance multiplier constant from sub_0424E0 ($0424FE):
// MOVE.L #$00001030, D2 -- a log-float with exponent 0, mantissa
// $1030 = 4144, representing 4144/4096 ~ 1.0117
constexpr uint32_t CAMERA_ADVANCE_FACTOR = 0x00001030;

// Initial star coordinates from $042486-$042496:
// X = $00001000, Y = $00001000, Z = $0000F000
constexpr uint32_t INIT_STAR_X = 0x00001000;
constexpr uint32_t INIT_STAR_Y = 0x00001000;
constexpr uint32_t INIT_STAR_Z = 0x0000F000;

// Map draw type (jump table offset at $04299A) to palette index

static uint8_t DrawTypeToColour(uint16_t drawType)
{
	switch (drawType)
	{
	case 0x0000:
		return 9;
	case 0x0020:
		return 1;
	case 0x0040:
		return 2;
	case 0x0060:
		return 11;
	case 0x0080:
		return 4;
	case 0x00A0:
		return 5;
	case 0x00C0:
		return 6;
	case 0x00E0:
		return 5;
	case 0x0100:
		return 11;
	case 0x0120:
		return 9;
	case 0x0140:
		return 9;
	case 0x0160:
		return 11;
	case 0x0180:
		return 12;
	case 0x01A0:
		return 13;
	case 0x01C0:
		return 11;
	case 0x01E0:
		return 5;
	default:
		return 9;
	}
}

// PRNG -- xorshift replacement for the original's 4KB rotating-table RNG

static uint16_t PrngWord(uint32_t &state)
{
	state ^= state << 13;
	state ^= state >> 17;
	state ^= state << 5;
	return static_cast<uint16_t>(state);
}

// Star respawn
// $0427E2-$042924.  Generates random X, Y, Z coordinates
// in log-float format, with Z distributed across 4 depth bands

static void RespawnStar(Star &star, uint32_t &rng)
{
	// Random X: signed 16-bit, doubled, normalised to log-float
	// Original: read PRNG word, EXT.L, ADD.L D0,D0, normalise
	int32_t rawX =
		static_cast<int32_t>(static_cast<int16_t>(PrngWord(rng))) * 2;
	// Random Y: same process
	int32_t rawY =
		static_cast<int32_t>(static_cast<int16_t>(PrngWord(rng))) * 2;

	star.x = IntToLogFloat(rawX);
	star.y = IntToLogFloat(rawY);

	// Random Z depth band (0-3): ANDI.W #$0003,D2 at $0428E0
	uint16_t band = PrngWord(rng) & 0x0003;

	// Scale X and Y by depth band
	// Original: ADD.L D3,D0 / ADD.L D3,D1 where D3 = band << 16
	// adds `band` to the exponent, making distant stars
	// spread wider in world space
	uint32_t bandShift = static_cast<uint32_t>(band) << 16;
	star.x += bandShift;
	star.y += bandShift;

	// Z: exponent = band + 8, random mantissa in $1000-$1FFF
	// Original: ADDI.W #8,D2; SWAP D2; ANDI.W #$0FFE,D2; ORI.W #$1000
	uint16_t zMant = (PrngWord(rng) & 0x0FFE) | 0x1000;
	star.z = (static_cast<uint32_t>(band + 8) << 16) | zMant;

	// Draw type: $042800 ANDI.W #$01E0,D0; MOVE.W D0,4(A0)
	// A0 is at the star's Z field (+8), so offset +4 from A0 =
	// star base + 12 = the drawType field
	// gives values $0000,$0020,$0040,...,$01E0 (16 possible
	// values in steps of $20), used as jump table offsets into the
	// pixel draw variants at $04299A
	// Bits 5-8 of the PRNG word -> star brightness/colour
	star.drawType = PrngWord(rng) & 0x01E0;

	star.screenX = -1;
	star.screenY = -1;
}

// Projection ($0426E8-$0427DA)
// Depth subtraction then log-domain perspective divide

static bool ProjectStar(Star &star, uint32_t cameraZ)
{
	uint32_t starZ = star.z;
	uint32_t camZ = cameraZ;

	// Sort so the larger 32-bit value is in D2, smaller in D3
	// Original: CMP.L D2,D3; BLT loc_0426F6; EXG D3,D2
	// CMP.L D2,D3 computes D3-D2 for flags; BLT branches if D3 < D2
	uint32_t D2 = starZ;
	uint32_t D3 = camZ;
	if (static_cast<int32_t>(D3) >= static_cast<int32_t>(D2))
	{
		uint32_t tmp = D2;
		D2 = D3;
		D3 = tmp;
	}

	uint32_t depth = LogFloatAdd(D2, D3);

	// Word-align depth mantissa: ANDI.W #$FFFE,D2
	depth &= 0xFFFFFFFEu;

	// Check depth mantissa: TST.W D2; BMI -> respawn
	int16_t depthMant = static_cast<int16_t>(depth & 0xFFFF);
	if (depthMant < 0)
	{
		return false;
	}

	// Store updated depth back to star
	star.z = depth;

	// Project X
	// screenX = antilog(log(starX) - log(depth)) -> integer -> +160
	uint32_t projX = LogDivide(star.x, depth);
	int32_t linearX = LogFloatToScreen(projX);
	if (linearX == 0x7FFFFFFF)
	{
		return false; // overflow
	}
	int screenX = linearX + STAR_CENTER_X;
	if (screenX < 0 || screenX >= FB_WIDTH)
	{
		return false;
	}

	// Project Y
	uint32_t projY = LogDivide(star.y, depth);
	int32_t linearY = LogFloatToScreen(projY);
	if (linearY == 0x7FFFFFFF)
	{
		return false;
	}
	int screenY = linearY + STAR_CENTER_Y;
	if (screenY < 0 || screenY >= VIEWPORT_H)
	{
		return false;
	}

	star.screenX = static_cast<int16_t>(screenX);
	star.screenY = static_cast<int16_t>(screenY);
	return true;
}

void StarfieldInit(Starfield &sf, uint8_t *indexBuf)
{
	sf.cameraZ = INITIAL_CAMERA_Z;
	sf.freqCounter = INITIAL_FREQ_B;
	sf.freqCounter2 = INITIAL_FREQ_C;
	sf.rngState = 0xDEADBEEF;

	// Set all stars to the dummy initial position
	// Original: $042486 loop sets X=$1000, Y=$1000, Z=$F000
	for (int i = 0; i < STAR_COUNT; i++)
	{
		sf.stars[i].x = INIT_STAR_X;
		sf.stars[i].y = INIT_STAR_Y;
		sf.stars[i].z = INIT_STAR_Z;
		sf.stars[i].drawType = 0;
		sf.stars[i].screenX = -1;
		sf.stars[i].screenY = -1;
	}

	// Run one projection pass.  The dummy position fails projection,
	// triggering respawn.  In the original, the respawn at $0427E2
	// jumps back to $042760 (BRA loc_042760) to re-project the
	// freshly spawned star -- if that also fails, it respawns again
	// loops until every star is on-screen.  We replicate that
	// by looping respawn+project until success
	for (int i = 0; i < STAR_COUNT; i++)
	{
		Star &star = sf.stars[i];
		constexpr int MAX_ATTEMPTS = 50;
		for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++)
		{
			if (ProjectStar(star, sf.cameraZ))
			{
				break;
			}
			RespawnStar(star, sf.rngState);
		}
		// Draw the star pixel
		if (star.screenX >= 0 && star.screenY >= 0)
		{
			uint8_t colour = DrawTypeToColour(star.drawType);
			indexBuf[star.screenY * FB_WIDTH + star.screenX] = colour;
		}
	}
}

void StarfieldDraw(Starfield &sf, uint8_t *indexBuf)
{
	for (int i = 0; i < STAR_COUNT; i++)
	{
		Star &star = sf.stars[i];

		// Erase old pixel (restore to sky colour)
		if (star.screenX >= 0 && star.screenY >= 0 && star.screenX < FB_WIDTH &&
			star.screenY < VIEWPORT_H)
		{
			indexBuf[star.screenY * FB_WIDTH + star.screenX] = SKY_COLOUR;
			star.screenX = -1;
			star.screenY = -1;
		}

		// Project.  If off-screen or behind camera, respawn and
		// re-project.  The original loops: respawn -> BRA loc_042760
		// (re-project) -> if fail -> respawn again -> etc
		if (!ProjectStar(star, sf.cameraZ))
		{
			constexpr int MAX_ATTEMPTS = 50;
			bool placed = false;
			for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++)
			{
				RespawnStar(star, sf.rngState);
				if (ProjectStar(star, sf.cameraZ))
				{
					placed = true;
					break;
				}
			}
			if (!placed)
			{
				continue;
			}
		}

		// Draw at new position
		uint8_t colour = DrawTypeToColour(star.drawType);
		indexBuf[star.screenY * FB_WIDTH + star.screenX] = colour;
	}
}

bool StarfieldAdvanceCamera(Starfield &sf)
{
	// sub_0424E0: multiply cameraZ by $1030 (~1.0117) each frame
	// Frequency counter decrements from $0700 toward $0400
	// Returns false when counter reaches $0400 (Phase 2 complete)
	if (sf.freqCounter <= 0x0400)
	{
		return false;
	}

	sf.cameraZ = LogMultiply(sf.cameraZ, CAMERA_ADVANCE_FACTOR);
	sf.freqCounter -= 4;
	sf.freqCounter2 -= 4;
	return true;
}

bool StarfieldCrashDescent(Starfield &sf)
{
	// sub_0425C0: divide cameraZ by $1030 each frame (reverses the
	// Phase 2 acceleration).  Frequency counter increments from
	// wherever Phase 2 left it back toward $0700
	// Returns true when counter reaches $0700 (crash descent done,
	// transition to gameplay)
	if (sf.freqCounter >= 0x0700)
	{
		return true;
	}

	sf.cameraZ = LogDivide(sf.cameraZ, CAMERA_ADVANCE_FACTOR);
	sf.freqCounter += 4;
	sf.freqCounter2 += 4;
	return false;
}
