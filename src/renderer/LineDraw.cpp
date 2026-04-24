// Line drawing -- sub_04745E (E1:10418) and E2 Bresenham routines

#include "renderer/LineDraw.h"
#include "data/GameData.h"
#include "renderer/FrameBuffer.h"
#include "renderer/LogMath.h"

#include <utility>

// Bresenham accumulator starts at 0.5 so the first minor-axis step is taken
// after half a major-axis unit of travel.  The carry-out (bit 16 after
// 16-bit addition) signals the minor-axis step
static constexpr uint16_t BRESENHAM_ACCUM_INIT = 0x8000;

// Pixel write matching the original's BSET/BCLR octant routines
// BSET (E2:$060BBE): BSET D1,(A1)           -- sets 1 bitplane bit
// BCLR (E2:$060CDC): BCLR D1,(A1); +2; BCLR -- clears 2 bitplane bits
static inline void WritePixel(uint8_t *indexBuf, int x, int y, LineMode mode)
{
	if (mode == LineMode::BCLR)
		indexBuf[y * FB_WIDTH + x] &= ~0x03;
	else
		indexBuf[y * FB_WIDTH + x] |= 1;
}

// E2 Bresenham octant routines ($060BBE-$060DFA)
// 8 BSET + 8 BCLR in the original; we fold BSET/BCLR via LineMode
// Loop: draw, check major endpoint, step major, accumulate slope,
// on carry step minor + check sentinel.  Each octant overrides D5
// or D6 with a screen-edge sentinel

// Octant 0: X-major, X+ Y+ -- sub_060BBE (BSET) / sub_060CCE (BCLR)
//   sub_060BBE:
//     MOVE.W #$D500,D6       ; sentinel override (Y bottom edge)
//   loc_060BC2:
//     [draw pixel]
//     CMP.W  D1,D5          ; X endpoint check (before step)
//     BEQ    done
//     ADDQ.W #1,D1          ; X += 1
//     ADD.W  D3,D4          ; accumulate slope
//     BCC    loc_060BC2     ; no carry -> next pixel
//     ADD.W  D0,D2          ; Y += stride
//     CMP.W  D2,D6          ; Y sentinel check (after step)
//     BNE    loc_060BC2
//   done: RTS
static void Octant0_XpYp_Xmaj(uint8_t *buf, int x, int y, uint16_t slope,
							  int endX, int /*endY*/, LineMode mode)
{
	int sentinelY = VIEWPORT_H; // MOVE.W #$D500,D6
	uint16_t accum = BRESENHAM_ACCUM_INIT;
	for (;;)
	{
		if (x >= 0 && x < FB_WIDTH && y >= 0 && y < VIEWPORT_H)
			WritePixel(buf, x, y, mode);
		if (x == endX)
			return;
		x += 1;
		uint32_t na = static_cast<uint32_t>(accum) + slope;
		accum = static_cast<uint16_t>(na);
		if (na > 0xFFFF)
		{
			y += 1;
			if (y == sentinelY)
				return;
		}
	}
}

// Octant 1: Y-major, X+ Y+ -- sub_060BE0 / sub_060CF4
//
//   MOVE.W #$3590,D5
//   loc_060BE4:
//     [draw pixel]
//     CMP.W  D2,D6          ; Y endpoint (major, before step)
//     BEQ    done
//     ADD.W  D0,D2          ; Y += stride
//     ADD.W  D3,D4          ; accumulate
//     BCC    loc_060BE4
//     ADDQ.W #1,D1          ; X += 1 (minor)
//     CMP.W  D1,D5          ; X sentinel (after step)
//     BNE    loc_060BE4
//   done: RTS
static void Octant1_XpYp_Ymaj(uint8_t *buf, int x, int y, uint16_t slope,
							  int /*endX*/, int endY, LineMode mode)
{
	int sentinelX = FB_WIDTH;
	uint16_t accum = BRESENHAM_ACCUM_INIT;
	for (;;)
	{
		if (x >= 0 && x < FB_WIDTH && y >= 0 && y < VIEWPORT_H)
			WritePixel(buf, x, y, mode);
		if (y == endY)
			return;
		y += 1;
		uint32_t na = static_cast<uint32_t>(accum) + slope;
		accum = static_cast<uint16_t>(na);
		if (na > 0xFFFF)
		{
			x += 1;
			if (x == sentinelX)
				return;
		}
	}
}

// Octant 2: Y-major, X- Y+ -- sub_060C02 / sub_060D1A
//
//   MOVE.W #$344F,D5
//   loc_060C06:
//     [draw pixel]
//     CMP.W  D2,D6          ; Y endpoint (major)
//     BEQ    done
//     ADD.W  D0,D2          ; Y += stride
//     ADD.W  D3,D4
//     BCC    loc_060C06
//     SUBQ.W #1,D1          ; X -= 1 (minor)
//     CMP.W  D1,D5          ; X sentinel
//     BNE    loc_060C06
//   done: RTS
static void Octant2_XmYp_Ymaj(uint8_t *buf, int x, int y, uint16_t slope,
							  int /*endX*/, int endY, LineMode mode)
{
	int sentinelX = -1;
	uint16_t accum = BRESENHAM_ACCUM_INIT;
	for (;;)
	{
		if (x >= 0 && x < FB_WIDTH && y >= 0 && y < VIEWPORT_H)
			WritePixel(buf, x, y, mode);
		if (y == endY)
			return;
		y += 1;
		uint32_t na = static_cast<uint32_t>(accum) + slope;
		accum = static_cast<uint16_t>(na);
		if (na > 0xFFFF)
		{
			x -= 1;
			if (x == sentinelX)
				return;
		}
	}
}

// Octant 3: X-major, X- Y+ -- sub_060C24 / sub_060D40
//
//   MOVE.W #$D500,D6
//   loc_060C28:
//     [draw pixel]
//     CMP.W  D1,D5          ; X endpoint (major)
//     BEQ    done
//     SUBQ.W #1,D1          ; X -= 1
//     ADD.W  D3,D4
//     BCC    loc_060C28
//     ADD.W  D0,D2          ; Y += stride (minor)
//     CMP.W  D6,D2          ; Y sentinel
//     BNE    loc_060C28
//   done: RTS
static void Octant3_XmYp_Xmaj(uint8_t *buf, int x, int y, uint16_t slope,
							  int endX, int /*endY*/, LineMode mode)
{
	int sentinelY = VIEWPORT_H;
	uint16_t accum = BRESENHAM_ACCUM_INIT;
	for (;;)
	{
		if (x >= 0 && x < FB_WIDTH && y >= 0 && y < VIEWPORT_H)
			WritePixel(buf, x, y, mode);
		if (x == endX)
			return;
		x -= 1;
		uint32_t na = static_cast<uint32_t>(accum) + slope;
		accum = static_cast<uint16_t>(na);
		if (na > 0xFFFF)
		{
			y += 1;
			if (y == sentinelY)
				return;
		}
	}
}

// Octant 4: X-major, X- Y- -- sub_060C46 / sub_060D66
//
//   MOVE.W #$7F60,D6
//   loc_060C4A:
//     [draw pixel]
//     CMP.W  D1,D5          ; X endpoint (major)
//     BEQ    done
//     SUBQ.W #1,D1          ; X -= 1
//     ADD.W  D3,D4
//     BCC    loc_060C4A
//     SUB.W  D0,D2          ; Y -= stride (minor)
//     CMP.W  D6,D2          ; Y sentinel
//     BNE    loc_060C4A
//   done: RTS
static void Octant4_XmYm_Xmaj(uint8_t *buf, int x, int y, uint16_t slope,
							  int endX, int /*endY*/, LineMode mode)
{
	int sentinelY = -1;
	uint16_t accum = BRESENHAM_ACCUM_INIT;
	for (;;)
	{
		if (x >= 0 && x < FB_WIDTH && y >= 0 && y < VIEWPORT_H)
			WritePixel(buf, x, y, mode);
		if (x == endX)
			return;
		x -= 1;
		uint32_t na = static_cast<uint32_t>(accum) + slope;
		accum = static_cast<uint16_t>(na);
		if (na > 0xFFFF)
		{
			y -= 1;
			if (y == sentinelY)
				return;
		}
	}
}

// Octant 5: Y-major, X- Y- -- sub_060C68 / sub_060D8C
//
//   MOVE.W #$344F,D5
//   loc_060C6C:
//     [draw pixel]
//     CMP.W  D6,D2          ; Y endpoint (major)
//     BEQ    done
//     SUB.W  D0,D2          ; Y -= stride
//     ADD.W  D3,D4
//     BCC    loc_060C6C
//     SUBQ.W #1,D1          ; X -= 1 (minor)
//     CMP.W  D1,D5          ; X sentinel
//     BNE    loc_060C6C
//   done: RTS
static void Octant5_XmYm_Ymaj(uint8_t *buf, int x, int y, uint16_t slope,
							  int /*endX*/, int endY, LineMode mode)
{
	int sentinelX = -1;
	uint16_t accum = BRESENHAM_ACCUM_INIT;
	for (;;)
	{
		if (x >= 0 && x < FB_WIDTH && y >= 0 && y < VIEWPORT_H)
			WritePixel(buf, x, y, mode);
		if (y == endY)
			return;
		y -= 1;
		uint32_t na = static_cast<uint32_t>(accum) + slope;
		accum = static_cast<uint16_t>(na);
		if (na > 0xFFFF)
		{
			x -= 1;
			if (x == sentinelX)
				return;
		}
	}
}

// Octant 6: Y-major, X+ Y- -- sub_060C8A / sub_060DB2
//
//   MOVE.W #$3590,D5
//   loc_060C8E:
//     [draw pixel]
//     CMP.W  D6,D2          ; Y endpoint (major)
//     BEQ    done
//     SUB.W  D0,D2          ; Y -= stride
//     ADD.W  D3,D4
//     BCC    loc_060C8E
//     ADDQ.W #1,D1          ; X += 1 (minor)
//     CMP.W  D1,D5          ; X sentinel
//     BNE    loc_060C8E
//   done: RTS
static void Octant6_XpYm_Ymaj(uint8_t *buf, int x, int y, uint16_t slope,
							  int /*endX*/, int endY, LineMode mode)
{
	int sentinelX = FB_WIDTH;
	uint16_t accum = BRESENHAM_ACCUM_INIT;
	for (;;)
	{
		if (x >= 0 && x < FB_WIDTH && y >= 0 && y < VIEWPORT_H)
			WritePixel(buf, x, y, mode);
		if (y == endY)
			return;
		y -= 1;
		uint32_t na = static_cast<uint32_t>(accum) + slope;
		accum = static_cast<uint16_t>(na);
		if (na > 0xFFFF)
		{
			x += 1;
			if (x == sentinelX)
				return;
		}
	}
}

// Octant 7: X-major, X+ Y- -- sub_060CAC / sub_060DD8
//
//   MOVE.W #$7F60,D6
//   loc_060CB0:
//     [draw pixel]
//     CMP.W  D1,D5          ; X endpoint (major)
//     BEQ    done
//     ADDQ.W #1,D1          ; X += 1
//     ADD.W  D3,D4
//     BCC    loc_060CB0
//     SUB.W  D0,D2          ; Y -= stride (minor)
//     CMP.W  D6,D2          ; Y sentinel
//     BNE    loc_060CB0
//   done: RTS
static void Octant7_XpYm_Xmaj(uint8_t *buf, int x, int y, uint16_t slope,
							  int endX, int /*endY*/, LineMode mode)
{
	int sentinelY = -1;
	uint16_t accum = BRESENHAM_ACCUM_INIT;
	for (;;)
	{
		if (x >= 0 && x < FB_WIDTH && y >= 0 && y < VIEWPORT_H)
			WritePixel(buf, x, y, mode);
		if (x == endX)
			return;
		x += 1;
		uint32_t na = static_cast<uint32_t>(accum) + slope;
		accum = static_cast<uint16_t>(na);
		if (na > 0xFFFF)
		{
			y -= 1;
			if (y == sentinelY)
				return;
		}
	}
}

// octIdx = (yNeg ? 1 : 0) | (xNeg ? 2 : 0) | (yMajor ? 4 : 0)

using OctantFn = void (*)(uint8_t *, int, int, uint16_t, int, int, LineMode);

static const OctantFn OCTANT_DISPATCH[8] = {
	Octant0_XpYp_Xmaj, // octIdx 0: yN=0 xN=0 yM=0 -> sub_060BBE
	Octant7_XpYm_Xmaj, // octIdx 1: yN=1 xN=0 yM=0 -> sub_060CAC
	Octant3_XmYp_Xmaj, // octIdx 2: yN=0 xN=1 yM=0 -> sub_060C24
	Octant4_XmYm_Xmaj, // octIdx 3: yN=1 xN=1 yM=0 -> sub_060C46
	Octant1_XpYp_Ymaj, // octIdx 4: yN=0 xN=0 yM=1 -> sub_060BE0
	Octant6_XpYm_Ymaj, // octIdx 5: yN=1 xN=0 yM=1 -> sub_060C8A
	Octant2_XmYp_Ymaj, // octIdx 6: yN=0 xN=1 yM=1 -> sub_060C02
	Octant5_XmYm_Ymaj, // octIdx 7: yN=1 xN=1 yM=1 -> sub_060C68
};

// Slope computation -- E1:10572-10618

struct SlopeResult
{
	uint16_t slopeStep;		// 16-bit fixed-point minor-axis step
	bool yMajor;			// true if Y is the major axis
	bool overflow;			// true if slope overflows
	uint32_t slopeLogFloat; // D0 after E1:10578 -- |dy/dx| as log-float
							// Used by channel dispatch for edge intersection
};

static SlopeResult ComputeSlope(uint32_t dy, uint32_t dx)
{
	SlopeResult r;
	r.slopeStep = 0;
	r.yMajor = false;
	r.overflow = false;
	r.slopeLogFloat = 0;

	// E1:10573-10578: slope = |dy/dx| via log divide
	uint32_t slope = LogDivide(dy, dx);
	r.slopeLogFloat = slope; // save for channel dispatch

	// E1:10580: BMI -- test bit 31 (exponent sign)
	// If negative exponent, slope < 1.0 -> X-major
	// If non-negative, slope >= 1.0 -> Y-major
	if (static_cast<int32_t>(slope) >= 0)
	{
		// Y-major: compute reciprocal 1/slope = |dx/dy|
		r.yMajor = true;
		slope = LogDivide(0x00001000, slope); // 1.0 / slope

		// E1:10590: BMI -- if reciprocal exponent still negative, proceed
		// If non-negative (BPL), overflow -> sentinel
		if (static_cast<int32_t>(slope) >= 0)
		{
			r.slopeStep = 0xFFFF;
			r.overflow = true;
			return r;
		}
	}

	// E1:10597-10610: convert slope log-float to fixed-point
	// ADDI.L #$000F0000 adjusts exponent by +15
	uint32_t slopeAdj = slope + 0x000F0000;

	if (static_cast<int32_t>(slopeAdj) < 0)
	{
		// E1:10600: CLR.W D5 -- slope too small
		r.slopeStep = 0;
	}
	else
	{
		// E1:10604-10610: SWAP, NEG.W, ADDI.W #$E, EXT.L, ASL.L #2, ASR.L
		int16_t sExp = static_cast<int16_t>(slopeAdj >> 16);
		int16_t shift = static_cast<int16_t>(-sExp + 14);
		if (shift < 0)
		{
			// E1:10607: overflow -> sentinel
			r.slopeStep = 0xFFFF;
			r.overflow = true;
			return r;
		}
		int32_t sv = static_cast<int16_t>(slopeAdj & 0xFFFF);
		sv <<= 2;
		sv >>= shift;
		r.slopeStep = static_cast<uint16_t>(sv);
	}

	// E1:10613-10615: ADD.W D5,D5; BCC; NOT.W D5
	uint32_t doubled =
		static_cast<uint32_t>(r.slopeStep) + static_cast<uint32_t>(r.slopeStep);
	if (doubled > 0xFFFF)
		r.slopeStep = ~static_cast<uint16_t>(doubled);
	else
		r.slopeStep = static_cast<uint16_t>(doubled);

	return r;
}

// Channel edge intersection -- E1:10809-11429
// Computes where the line crosses one screen edge
// Returns screen coordinate or -1 if out of bounds

static int ChannelIntersect(
	uint32_t channelCoord, // one of cam.channelCoord[0-3]
	uint32_t fovBase,	   // $00070000
	uint32_t vtxRatioA,	   // the ratio to subtract (xzRatio or yzRatio)
	uint32_t slopeD0,	   // D0: slope log-float from ComputeSlope
	uint32_t vtxRatioB,	   // the other ratio to add (yzRatio or xzRatio)
	uint16_t screenCenter, // screenCenterX or screenCenterY
	uint16_t maxCoord,	   // $013F for X, $0087 for Y
	bool divide) // true for channels 1/3 (SUB.L), false for 0/2 (ADD.L)
{
	// E1:10813-10814: SUB.L D4, D2 (full 32-bit subtract, NOT LogFloatAdd)
	uint32_t d2 = channelCoord - fovBase;

	// E1:10815-10816: NEG.W vtxRatioA mantissa, then LogFloatAdd
	uint16_t negMant =
		static_cast<uint16_t>(-static_cast<int16_t>(vtxRatioA & 0xFFFF));
	uint32_t negRatio = (vtxRatioA & 0xFFFF0000u) | negMant;
	d2 = LogFloatAdd(d2, negRatio);

	// E1:10872: ANDI.W #$FFFE -- clear bit 0 before log table access
	d2 &= 0xFFFFFFFEu;

	// Channels 0/2 (vertical edges): ADD.L -> LogMultiply (E1:10876, 11177)
	// Channels 1/3 (horizontal edges): SUB.L -> LogDivide (E1:11027, 11328)
	if (divide)
		d2 = LogDivide(d2, slopeD0);
	else
		d2 = LogMultiply(d2, slopeD0);

	// E1:10880-10936: LogFloatAdd with vtxRatioB
	d2 = LogFloatAdd(d2, vtxRatioB);

	// E1:10936: ANDI.W #$FFFE -- clear bit 0 before ADD.L
	d2 &= 0xFFFFFFFEu;

	// E1:10937: ADD.L D4, D2 (full 32-bit add, NOT LogFloatAdd)
	d2 = d2 + fovBase;

	// E1:10938-10950: LogFloatToScreen
	int32_t coord = LogFloatToScreen(d2);

	// E1:10953: add screen centre
	coord += screenCenter;

	// E1:10954-10955: CMPI.W / BCC bounds check
	if (static_cast<uint16_t>(coord) > maxCoord)
		return -1;

	return static_cast<int>(coord);
}

// sub_04745E (E1:10418-11474)
// mode selects the pixel operation:
//   BSET ($0624C8=0): roads -- pixel |= 1
//   BCLR ($0624C8=$20): building outlines -- pixel &= ~3

void DrawLineProjectedMode(uint8_t *indexBuf, const ProjectedVertex &vA,
						   const ProjectedVertex &vB, const Camera &cam,
						   LineMode mode)
{
	// E1:10421-10422: load visibility flags
	uint16_t vis1 = vA.visFlags;
	uint16_t vis2 = vB.visFlags;

	// E1:10423-10437: visibility classification and swap
	const ProjectedVertex *pV1 = &vA;
	const ProjectedVertex *pV2 = &vB;

	if (vis1 == 0)
	{
		// vtx1 visible -> main dispatch
	}
	else if (vis1 & 0x8000)
	{
		// vtx1 behind -> swap
		std::swap(pV1, pV2);
		std::swap(vis1, vis2);
		if (vis1 & 0x8000)
			return; // both behind -> Path 3
	}
	else
	{
		// vtx1 off-screen positive
		if (vis2 == 0)
		{
			std::swap(pV1, pV2);
			std::swap(vis1, vis2);
		}
		else if (vis2 & 0x8000)
		{
			// vtx2 behind -> proceed without swap
		}
		else
		{
			// Both off-screen positive -> Z tiebreak
			if (static_cast<int32_t>(pV1->camZ) <=
				static_cast<int32_t>(pV2->camZ))
			{
				std::swap(pV1, pV2);
				std::swap(vis1, vis2);
			}
		}
	}

	// E1:10440-10441: test vtx2 vis
	// If vtx2 behind -> Path 2
	bool path2 = (vis2 & 0x8000) != 0;

	// Slope computation
	// Path 1: dy/dx from xzRatio/yzRatio differences (E1:10443-10618)
	// Path 2: 3D slope from camera-space coords (E1:10620-10769)

	uint32_t dy, dx;
	bool yNeg = false, xNeg = false;

	if (path2)
	{
		// Path 2: compute t = -(Zbeh/Zvis), scaledX/Y
		uint32_t Zvis = pV1->camZ;
		uint32_t Zbeh = pV2->camZ;

		uint32_t ratio = LogDivide(Zbeh, Zvis);
		uint16_t tMant =
			static_cast<uint16_t>(-static_cast<int16_t>(ratio & 0xFFFF));
		uint32_t t = (ratio & 0xFFFF0000u) | tMant;

		uint32_t YvisT = LogMultiply(pV1->camY, t);
		uint32_t scaledY = LogFloatAdd(YvisT, pV2->camY);

		uint32_t XvisT = LogMultiply(pV1->camX, t);
		uint32_t scaledX = LogFloatAdd(XvisT, pV2->camX);

		// E1:10697-10700: Y sign from EOR scaledY with Zvis
		yNeg = ((scaledY ^ Zvis) & 0x8000) != 0;
		// E1:10766-10769: X sign from EOR scaledX with Zvis
		xNeg = ((scaledX ^ Zvis) & 0x8000) != 0;

		dy = scaledY;
		dx = scaledX;
	}
	else
	{
		// Path 1: dy/dx from ratio differences
		// E1:10443-10448: load ratios, negate vtx1's
		uint32_t yzR2 = pV2->yzRatio;
		uint32_t yzR1 = pV1->yzRatio;
		uint32_t xzR2 = pV2->xzRatio;
		uint32_t xzR1 = pV1->xzRatio;

		// NEG.W vtx1 ratios
		uint16_t negYZ =
			static_cast<uint16_t>(-static_cast<int16_t>(yzR1 & 0xFFFF));
		yzR1 = (yzR1 & 0xFFFF0000u) | negYZ;

		uint16_t negXZ =
			static_cast<uint16_t>(-static_cast<int16_t>(xzR1 & 0xFFFF));
		xzR1 = (xzR1 & 0xFFFF0000u) | negXZ;

		// E1:10449-10504: dy = LogFloatAdd(yzR2, -yzR1)
		dy = LogFloatAdd(yzR2, yzR1);

		// E1:10504-10507: ANDI #$FFFE; test sign -> yNeg
		dy = (dy & 0xFFFFFFFEu); // ANDI.W #$FFFE
		yNeg = (static_cast<int16_t>(dy & 0xFFFF) < 0);

		// E1:10509-10565: dx = LogFloatAdd(xzR2, -xzR1)
		dx = LogFloatAdd(xzR2, xzR1);

		// E1:10565-10570: ANDI #$FFFE; test sign -> xNeg
		dx = (dx & 0xFFFFFFFEu);
		xNeg = (static_cast<int16_t>(dx & 0xFFFF) < 0);
	}

	// E1:10572-10618: compute slope
	SlopeResult slope = ComputeSlope(dy, dx);
	int octIdx = (yNeg ? 1 : 0) | (xNeg ? 2 : 0) | (slope.yMajor ? 4 : 0);

	// E1:10774-10777: re-test vis flags for draw path selection
	// TST.W D2; BEQ -> both-visible draw (loc_047CF6, E1:11431)
	// TST.W D1; BEQ -> draw from vtx1 toward edge (loc_047D4C, E1:11456)
	if (vis2 == 0)
	{
		// Path 1: both visible (loc_047CF6, E1:11431-11454)
		// D5 = vtx2.X, D6 = vtx2.Y
		// All 8 octant functions override the minor-axis sentinel
		// internally, so only the major-axis value (vtx2's coordinate)
		// is actually used for termination
		OCTANT_DISPATCH[octIdx](indexBuf, pV1->screenX, pV1->screenY,
								slope.slopeStep, pV2->screenX, pV2->screenY,
								mode);
		return;
	}

	if (vis1 == 0)
	{
		// Path 2: one visible, draw to edge.  D5/D6 from $0E4E
		// (screen edges in the direction the octant walks toward)
		int endX = (octIdx & 2) ? 0 : (FB_WIDTH - 1);
		int endY = (octIdx & 1) ? 0 : (VIEWPORT_H - 1);
		OCTANT_DISPATCH[octIdx](indexBuf, pV1->screenX, pV1->screenY,
								slope.slopeStep, endX, endY, mode);
		return;
	}

	// Both have nonzero vis flags (off-screen but in front)
	// E1:10771-10807: octant table lookup, then rendering attribute key
	//
	// A3 accumulates: +2 for yNeg, +4 for xNeg, +8 for yMajor
	// E1:10772: MOVEA.W $0DFE(A3), A3 -- look up octant table
	// The octant table at E2+$0DFE maps the 8 octant offsets to
	// specific key base values

	// Octant table (extracted from E2+$0DFE, 8 words):
	static const uint16_t OCTANT_TABLE[8] = {
		0x0000, 0x001C, 0x000C, 0x0010, 0x0004, 0x0018, 0x0008, 0x0014,
	};

	uint16_t octKey = OCTANT_TABLE[octIdx];

	// E1:10778-10786: build combined key
	// D6 = A3.W | D1 (octant key | vis1)
	uint16_t d6 = octKey | vis1;

	// E1:10782-10785: shift sign bits in via ASL/ROXL
	// ASL.W #1, D3 shifts yzSign bit 15 into carry
	// ROXL.W #1, D6 rotates carry into D6 bit 0 (shifting D6 left)
	// Then same for xzSign
	uint16_t yzS = pV1->yzSign;
	uint16_t xzS = pV1->xzSign;
	d6 = static_cast<uint16_t>((d6 << 1) | ((yzS >> 15) & 1));
	d6 = static_cast<uint16_t>((d6 << 1) | ((xzS >> 15) & 1));

	// Bounds-check the key
	if (d6 >= gen_e2::RENDER_ATTR_SIZE)
		return;

	uint8_t attr = gen_e2::RENDER_ATTR_TABLE[d6];

	if (attr == 0)
		return; // line culled -- doesn't cross viewport

	// E1:10793-10807: check vtx2's attribute
	// E1:10793-10794: TST.W D2; BMI -> skip if vtx2 behind camera
	if (!(vis2 & 0x8000))
	{
		uint16_t d6b = octKey | vis2;
		uint16_t yzS2 = pV2->yzSign;
		uint16_t xzS2 = pV2->xzSign;
		d6b = static_cast<uint16_t>((d6b << 1) | ((yzS2 >> 15) & 1));
		d6b = static_cast<uint16_t>((d6b << 1) | ((xzS2 >> 15) & 1));
		if (d6b < gen_e2::RENDER_ATTR_SIZE)
		{
			if (gen_e2::RENDER_ATTR_TABLE2[d6b] == 0)
				return;
		}
	}

	// E1:10809-11429: 4-channel dispatch
	// Each channel computes a screen-edge intersection
	// The multiply uses D0 = slope log-float (|dy/dx|) for ALL channels
	//
	// Channel edge sentinels for the octant dispatch (equivalent to
	// the $0E4E data in the original).  The octant functions that
	// override a sentinel will replace one of these internally
	int edgeEndX = (octIdx & 2) ? 0 : (FB_WIDTH - 1);
	int edgeEndY = (octIdx & 1) ? 0 : (VIEWPORT_H - 1);

	uint32_t slopeD0 = slope.slopeLogFloat;

	if (attr & 0x01)
	{
		int cy = ChannelIntersect(cam.channelCoord[1], cam.FOV_BASE,
								  pV1->xzRatio, slopeD0, pV1->yzRatio,
								  cam.screenCenterY, 0x0087, false);
		if (cy >= 0)
		{
			OCTANT_DISPATCH[octIdx](indexBuf, FB_WIDTH - 1, cy, slope.slopeStep,
									edgeEndX, edgeEndY, mode);
		}
	}

	if (attr & 0x02)
	{
		int cx = ChannelIntersect(cam.channelCoord[3], cam.FOV_BASE,
								  pV1->yzRatio, slopeD0, pV1->xzRatio,
								  cam.screenCenterX, 0x013F, true);
		if (cx >= 0)
		{
			OCTANT_DISPATCH[octIdx](indexBuf, cx, VIEWPORT_H - 1,
									slope.slopeStep, edgeEndX, edgeEndY, mode);
		}
	}

	if (attr & 0x04)
	{
		int cy = ChannelIntersect(cam.channelCoord[0], cam.FOV_BASE,
								  pV1->xzRatio, slopeD0, pV1->yzRatio,
								  cam.screenCenterY, 0x0087, false);
		if (cy >= 0)
		{
			OCTANT_DISPATCH[octIdx](indexBuf, 0, cy, slope.slopeStep, edgeEndX,
									edgeEndY, mode);
		}
	}

	if (attr & 0x08)
	{
		int cx = ChannelIntersect(cam.channelCoord[2], cam.FOV_BASE,
								  pV1->yzRatio, slopeD0, pV1->xzRatio,
								  cam.screenCenterX, 0x013F, true);
		if (cx >= 0)
		{
			OCTANT_DISPATCH[octIdx](indexBuf, cx, 0, slope.slopeStep, edgeEndX,
									edgeEndY, mode);
		}
	}
}

// BSET wrapper used by RoadsDraw

void DrawLineProjected(uint8_t *indexBuf, const ProjectedVertex &vA,
					   const ProjectedVertex &vB, const Camera &cam)
{
	DrawLineProjectedMode(indexBuf, vA, vB, cam, LineMode::BSET);
}
