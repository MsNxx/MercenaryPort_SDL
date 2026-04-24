// Camera trig lookups and rotation matrix -- sub_044640, sub_04655E, sub_046684

#include "data/Gen_MData.h"
#include "game/Camera.h"
#include "game/TileDetail.h"
#include "renderer/LogMath.h"

// Altitude factor (sub_044640, E1:5638-5686)
// Log-float altitude from posY, used by CameraBuildMatrix for screen centres

void CameraComputeAltFactor(Camera &cam)
{
	// E1:5638: default value
	uint32_t d0 = 0x00041000;

	// E1:5639-5640: TST.W ($0623AA); BNE -> use default
	uint16_t altHi =
		static_cast<uint16_t>(static_cast<uint32_t>(cam.posY) >> 16);
	if (altHi == 0)
	{
		// E1:5641-5680: IntToLogFloat(posY) with sign preservation
		d0 = IntToLogFloat(cam.posY);

		// E1:5683: SUBI.L #$000C0000 -- subtract 12 from exponent
		d0 -= 0x000C0000;
	}

	cam.altFactor = d0;
}

// Trig table access
// Longword lookup into the log/trig table at A4 = $056000

static uint32_t TrigLookup(uint16_t angle)
{
	// Replicate: D1 = angle; D1 *= 4; D1 &= $0FFC;
	//            D1 = MOVE.L 0(A4,D1.W)
	uint16_t byteOff = (angle * 4) & 0x0FFC;
	int wordIdx = gen_m::LOG_A5_OFFSET + byteOff / 2;

	if (wordIdx < 0 || wordIdx + 1 >= gen_m::LOG_FULL_WORDS)
	{
		return LOG_FLOAT_ZERO;
	}

	uint16_t hi = gen_m::LOG_TABLE_FULL[wordIdx];
	uint16_t lo = gen_m::LOG_TABLE_FULL[wordIdx + 1];
	return (static_cast<uint32_t>(hi) << 16) | lo;
}

// CameraComputeTrig (sub_04655E)

void CameraComputeTrig(Camera &cam, TileDetailState &td)
{
	// Roll ($062348)
	// $046570: D1 = roll
	// $046576: D2 = roll (copy)
	// $046578: D1 += 256 (quadrature)
	// $04657C-$046594: lookup both, store
	cam.cosRoll = TrigLookup(cam.roll + 256); // $062352
	cam.sinRoll = TrigLookup(cam.roll);		  // $06234E

	// $0465A4-$0465B8: tanRoll = sinRoll / cosRoll
	cam.tanRoll = LogDivide(cam.sinRoll, cam.cosRoll); // $062356

	// $0465B8-$0465CC: cotRoll = cosRoll / sinRoll
	cam.cotRoll = LogDivide(cam.cosRoll, cam.sinRoll); // $06235A

	// Pitch ($06234A)
	// $0465D8-$046608: same pattern
	cam.cosPitch = TrigLookup(cam.pitch + 256); // $062362
	cam.sinPitch = TrigLookup(cam.pitch);		// $06235E

	// $046608-$04661C: tanPitch = sinPitch / cosPitch
	cam.tanPitch = LogDivide(cam.sinPitch, cam.cosPitch); // $062366

	// Heading ($06234C)
	// $046622-$04664C: same pattern but no tan/cot computed
	cam.cosHeading = TrigLookup(cam.heading + 256); // $06236E
	cam.sinHeading = TrigLookup(cam.heading);		// $06236A

	// Spinning angle ($0623DE)
	// $046652-$04667C (E1:8850-8862): sin/cos lookup for the global
	// spinning angle, used by sub_04497E for per-vertex rotation
	td.spinCos = TrigLookup(td.spinAngle + 256); // $0623E4
	td.spinSin = TrigLookup(td.spinAngle);		 // $0623E0
}

// CameraBuildMatrix (sub_046684)
// 3x3 log-float rotation matrix + screen centre offsets from altitude

static uint32_t XorSub(uint32_t val, uint16_t mask)
{
	// XOR_SUB: (val XOR mask) - mask on the mantissa word
	// When mask=0: identity.  When mask=$FFFF: negates mantissa
	uint16_t lo = static_cast<uint16_t>(val);
	lo ^= mask;
	lo -= mask;
	return (val & 0xFFFF0000u) | lo;
}

void CameraBuildMatrix(Camera &cam)
{
	// The matrix formulas use sp/cp/sr/cr as shorthand for the
	// trig values at specific ADDRESSES, not semantic angle names
	// The addresses are fixed regardless of what we call the angles:
	//   A4 = $06234E, D5 = $062352, A0 = $06235E, A1 = $062362
	uint32_t sp = cam.sinRoll;	  // $06234E (value computed from $062348)
	uint32_t cp = cam.cosRoll;	  // $062352 (value computed from $062348)
	uint32_t sr = cam.sinPitch;	  // $06235E (value computed from $06234A)
	uint32_t cr = cam.cosPitch;	  // $062362 (value computed from $06234A)
	uint32_t sh = cam.sinHeading; // A2 = $06236A
	uint32_t ch = cam.cosHeading; // A3 = $06236E
	uint16_t mask = cam.horizonMirrorMask; // D4 = $062490

	// Row 1: camera X (screen right)

	// M10 ($06237E) = mirror(cp*ch + sp*sr*sh)
	// E1:8876-8957
	{
		uint32_t t1 = LogMultiply(sp, sr); // sp*sr
		t1 = LogMultiply(t1, sh);		   // sp*sr*sh
		uint32_t t2 = LogMultiply(cp, ch); // cp*ch
		t1 = LogFloatAdd(t1, t2);		   // sp*sr*sh + cp*ch
		cam.matrix[3] = XorSub(t1, mask);
	}

	// M11 ($062382) = mirror(-(sp*cr))
	// E1:8958-8969
	{
		uint32_t t1 = LogMultiply(sp, cr); // sp*cr
		// NEG.W: negate mantissa
		uint16_t lo = static_cast<uint16_t>(-static_cast<int16_t>(t1 & 0xFFFF));
		t1 = (t1 & 0xFFFF0000u) | lo;
		cam.matrix[4] = XorSub(t1, mask);
	}

	// Screen centre X ($0623C8)
	// E1:8970-8994: mul(M11, altFactor), linearise, add 160
	// Also computes channel coords $0623CC and $0623D0
	{
		uint32_t m11af = LogMultiply(cam.matrix[4], cam.altFactor);
		int32_t shiftX = LogFloatToScreen(m11af);
		cam.screenCenterX = static_cast<uint16_t>(shiftX + 160);

		// $0623CC = add(NEG(m11*altFactor), $0007EC00)
		uint16_t negMant =
			static_cast<uint16_t>(-static_cast<int16_t>(m11af & 0xFFFF));
		uint32_t negM11af = (m11af & 0xFFFF0000u) | negMant;
		cam.channelCoord[0] = LogFloatAdd(negM11af, 0x0007EC00);

		// $0623D0 = add($0623CC, $000813F0)
		cam.channelCoord[1] = LogFloatAdd(cam.channelCoord[0], 0x000813F0);
	}

	// M12 ($062386) = mirror(sp*sr*ch - cp*sh)
	// E1:9112-9194
	{
		uint32_t t1 = LogMultiply(sp, sr);
		t1 = LogMultiply(t1, ch);		   // sp*sr*ch
		uint32_t t2 = LogMultiply(cp, sh); // cp*sh
		// NEG.W t2: negate mantissa
		uint16_t lo = static_cast<uint16_t>(-static_cast<int16_t>(t2 & 0xFFFF));
		t2 = (t2 & 0xFFFF0000u) | lo;
		t1 = LogFloatAdd(t1, t2); // sp*sr*ch - cp*sh
		cam.matrix[5] = XorSub(t1, mask);
	}

	// Row 2: camera Y (screen down)

	// M20 ($06238A) = sp*ch - cp*sr*sh  (no mirror)
	// E1:9195-9275
	{
		uint32_t t1 = LogMultiply(cp, sr);
		t1 = LogMultiply(t1, sh); // cp*sr*sh
		// NEG.W t1
		uint16_t lo = static_cast<uint16_t>(-static_cast<int16_t>(t1 & 0xFFFF));
		t1 = (t1 & 0xFFFF0000u) | lo;
		uint32_t t2 = LogMultiply(sp, ch); // sp*ch
		t1 = LogFloatAdd(t2, t1);		   // sp*ch - cp*sr*sh
		cam.matrix[6] = t1;
	}

	// M21 ($06238E) = cp*cr
	// E1:9276-9284
	{
		cam.matrix[7] = LogMultiply(cp, cr);
	}

	// Screen centre Y ($0623CA)
	// E1:9285-9309: mul(M21, altFactor), linearise, add 68
	// Also computes channel coords $0623D4 and $0623D8
	{
		uint32_t m21af = LogMultiply(cam.matrix[7], cam.altFactor);
		int32_t shiftY = LogFloatToScreen(m21af);
		cam.screenCenterY = static_cast<uint16_t>(shiftY + 68);

		// $0623D4 = add(NEG(m21*altFactor), $0006EF00)
		uint16_t negMant =
			static_cast<uint16_t>(-static_cast<int16_t>(m21af & 0xFFFF));
		uint32_t negM21af = (m21af & 0xFFFF0000u) | negMant;
		cam.channelCoord[2] = LogFloatAdd(negM21af, 0x0006EF00);

		// $0623D8 = add($0623D4, $000710E0)
		cam.channelCoord[3] = LogFloatAdd(cam.channelCoord[2], 0x000710E0);
	}

	// M22 ($062392) = -(cp*sr*ch + sp*sh)
	// E1:9428-9507
	{
		uint32_t t1 = LogMultiply(cp, sr);
		t1 = LogMultiply(t1, ch);		   // cp*sr*ch
		uint32_t t2 = LogMultiply(sp, sh); // sp*sh
		t1 = LogFloatAdd(t1, t2);		   // cp*sr*ch + sp*sh
		// NEG.W
		uint16_t lo = static_cast<uint16_t>(-static_cast<int16_t>(t1 & 0xFFFF));
		t1 = (t1 & 0xFFFF0000u) | lo;
		cam.matrix[8] = t1;
	}

	// Row 0: camera Z (depth)

	// M00 ($062372) = cr*sh
	// E1:9508-9516
	cam.matrix[0] = LogMultiply(cr, sh);

	// M01 ($062376) = sr
	// E1:9517
	cam.matrix[1] = sr;

	// M02 ($06237A) = cr*ch
	// E1:9518-9526
	cam.matrix[2] = LogMultiply(cr, ch);
}
