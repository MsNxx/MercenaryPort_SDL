// Tile detail geometry -- load, spin, collapse, draw

#include "game/TileDetail.h"
#include "data/GameData.h"
#include "game/Workspace.h"
#include "renderer/LineDraw.h"
#include "renderer/LogMath.h"
#include "renderer/Projection.h"
#include "renderer/Roads.h"

#include <cstring>

// SpinningVertexUpdate (sub_04497E, E1:5996-6201)
// Per-vertex Y-axis rotation in log-space

void SpinningVertexUpdate(TileDetailState &td, const Camera &cam)
{
	(void)cam; // camera not needed -- all state is in td

	if (td.spinVertCount == 0)
		return;

	uint32_t sinA = td.spinSin; // $0623E0
	uint32_t cosA = td.spinCos; // $0623E4

	for (int i = 0; i < td.spinVertCount; i++)
	{
		uint32_t logX = td.spinLogDeltaX[i]; // $24F4 workspace
		uint32_t logZ = td.spinLogDeltaZ[i]; // $25F4 workspace

		// E1:6013-6027: X*cos and Z*sin
		uint32_t xCos = LogMultiply(logX, cosA);
		uint32_t zSin = LogMultiply(logZ, sinA);

		// E1:6028: NEG.W on Z*sin mantissa for subtraction
		uint16_t negMant =
			static_cast<uint16_t>(-static_cast<int16_t>(zSin & 0xFFFF));
		uint32_t zSinNeg = (zSin & 0xFFFF0000u) | negMant;

		// E1:6029-6083: LogFloatAdd(X*cos, -(Z*sin))
		uint32_t rotX = LogFloatAdd(xCos, zSinNeg);

		// E1:6084-6100: LogFloatToScreen + baseX
		// E1:6101: MOVE.W D0,$1410(A0) -- overwrites LOW WORD only
		// The high word (tile base) was set during precomputation
		int32_t wx = LogFloatToScreen(rotX);
		uint16_t wxLow = static_cast<uint16_t>(wx + td.spinBaseX);
		g_workspace.vertLongX[i] =
			(g_workspace.vertLongX[i] & static_cast<int32_t>(0xFFFF0000)) |
			wxLow;

		// E1:6102-6117: X*sin and Z*cos
		uint32_t xSin = LogMultiply(logX, sinA);
		uint32_t zCos = LogMultiply(logZ, cosA);

		// E1:6118-6170: LogFloatAdd(X*sin, Z*cos) -- NO negate
		uint32_t rotZ = LogFloatAdd(xSin, zCos);

		// E1:6171-6189: LogFloatToScreen + baseZ
		// E1:6190: MOVE.W D0,$1610(A0) -- overwrites LOW WORD only
		int32_t wz = LogFloatToScreen(rotZ);
		uint16_t wzLow = static_cast<uint16_t>(wz + td.spinBaseZ);
		g_workspace.vertLongZ[i] =
			(g_workspace.vertLongZ[i] & static_cast<int32_t>(0xFFFF0000)) |
			wzLow;
	}

	// E1:6194-6198: angle increment
	// TST.B ($062492); BMI skip -- only update if tileProperty >= 0
	if (td.tileProperty >= 0)
	{
		// $044B50: ASL.W #2,D0 -- speed * 4 (16-bit shift, not 32-bit multiply)
		// $044B52: ADD.W D0,($0623DE) -- angle wraps at 16 bits
		uint16_t increment = static_cast<uint16_t>(td.spinSpeed) << 2;
		td.spinAngle += increment;
	}
}

// sub_0448FC (E1:5927-5990): vertex gravity collapse
// Per-frame gravity + horizontal scatter, returns true if any vertex active

bool VertexGravityCollapse(TileDetailState &td)
{
	bool anyActive = false;

	// E1:5928: A0 = ($06239E) -- vertex high-water pointer
	// Iterate backward from tileVertCount-1 to 0, matching the
	// original's SUBQ.W #4,A0 / BPL loop
	for (int i = td.tileVertCount - 1; i >= 0; i--)
	{
		// E1:5932-5933: MOVE.L $150E(A0),D0; BEQ skip
		int32_t y = g_workspace.vertLongY[i];
		if (y == 0)
			continue;

		// E1:5934: MOVEQ #-1,D7 -- flag that at least one vertex active
		anyActive = true;

		// E1:5935-5937: SUBI.L #$80,$150E(A0); BPL ok; CLR.L $150E(A0)
		y -= 0x80;
		if (y < 0)
			y = 0;
		g_workspace.vertLongY[i] = y;

		// X scatter (E1:5940-5960)

		// E1:5940: MOVE.L $170E(A0),D0 -- packed velocity longword
		int32_t vel = g_workspace.vertLongVel[i];

		// E1:5941: MOVE.W $1410(A0),D1 -- X position (low word of vertLongX)
		int16_t xPos = static_cast<int16_t>(g_workspace.vertLongX[i]);
		// E1:5942-5947: TST.W D0 tests LOW WORD of vel = X velocity
		int16_t xVel = static_cast<int16_t>(vel);
		if (xVel == 0)
		{
			xVel = 4;
			if (xPos < 0)
				xVel = -4;
		}
		// E1:5950-5957: decelerate + scatter
		if (xVel < 0)
		{
			xVel++;		  // ADDQ.W #1,D0
			xPos -= 0x80; // SUBI.W #$80,D1
		}
		else
		{
			xVel--;		  // SUBQ.W #1,D0
			xPos += 0x80; // ADDI.W #$80,D1
		}
		// E1:5960: MOVE.W D1,$1410(A0) -- write X position low word
		g_workspace.vertLongX[i] =
			(g_workspace.vertLongX[i] & static_cast<int32_t>(0xFFFF0000)) |
			static_cast<uint16_t>(xPos);

		// E1:5961: SWAP D0 -- Z velocity now in low word

		// Z scatter (E1:5962-5981)

		// E1:5962: MOVE.W $1610(A0),D1 -- Z position (low word of vertLongZ)
		int16_t zPos = static_cast<int16_t>(g_workspace.vertLongZ[i]);
		// E1:5963-5968: after SWAP, HIGH WORD of vel (Z velocity) is in low
		int16_t zVel = static_cast<int16_t>(vel >> 16);
		if (zVel == 0)
		{
			zVel = 4;
			if (zPos < 0)
				zVel = -4;
		}
		// E1:5971-5978: decelerate + scatter
		if (zVel < 0)
		{
			zVel++;
			zPos -= 0x80;
		}
		else
		{
			zVel--;
			zPos += 0x80;
		}
		// E1:5981: MOVE.W D1,$1610(A0) -- write Z position low word
		g_workspace.vertLongZ[i] =
			(g_workspace.vertLongZ[i] & static_cast<int32_t>(0xFFFF0000)) |
			static_cast<uint16_t>(zPos);

		// E1:5982-5983: SWAP D0; MOVE.L D0,$170E(A0) -- store velocity
		// After SWAP: restores original packing: low word = X, high word = Z
		g_workspace.vertLongVel[i] =
			(static_cast<int32_t>(static_cast<uint16_t>(zVel)) << 16) |
			static_cast<uint16_t>(xVel);
	}

	// E1:5989-5990: TST.W D7; RTS -- return NE if any vertex active
	return anyActive;
}

// loc_0446C2 (E1:5695-5713): shared tile change + geometry load

static void TileChangeAndLoad(TileDetailState &td, const Camera &cam,
							  uint8_t d0, uint8_t d1)
{
	// E1:5696: MOVE.B #$FF,($062426) -- force D7 recalc
	td.cachedD7 = 0xFF;

	// E1:5697-5700: D3 = D0 | D1; if upper nibble set, D3 = 0
	int tileIdx;
	if ((d0 | d1) & 0xF0)
	{
		tileIdx = 0;
	}
	else
	{
		// E1:5705-5707: D3 = (D1 << 4) | D0
		tileIdx = (d1 << 4) | d0;
	}

	// E1:5710-5711: MOVE.W D3,($062420); BEQ loc_0446EE
	td.currentTileIndex = static_cast<uint16_t>(tileIdx);
	if (tileIdx == 0)
		return;

	// E1:5713: BSR sub_044742 -- load tile geometry

	// sub_044742 (E1:5743-5745): tile property from mutable table at $0631F4
	// The original reads from the workspace copy (initialised from E2),
	// not from E3 static data.  Destruction sets bit 7 in this table
	td.tileProperty = static_cast<int8_t>(g_workspace.objs.typeTable[tileIdx]);

	// E1:5746-5749: read tile geometry header
	int offTbl = 0x0400 + tileIdx * 2;
	uint16_t geoOffset =
		(gen_e3::E3_RAW[offTbl] << 8) | gen_e3::E3_RAW[offTbl + 1];
	if (geoOffset == 0 || geoOffset >= gen_e3::E3_SIZE)
	{
		td.spinVertCount = 0;
		td.tileVertCount = 0;
		return;
	}

	const uint8_t *geo = gen_e3::E3_RAW + geoOffset;
	int vtxRaw = geo[0];
	int connRaw = geo[1];
	int bldFlag = geo[3];

	// E1:5753-5754: store drawModeSplit to workspace
	// MOVE.B (A0)+,($0623DD) -- geo[2] to low byte
	// MOVE.B #$00,($0623DC) -- high byte = 0
	td.drawModeSplit = static_cast<uint16_t>(geo[2]);

	int vtxCount = (vtxRaw >> 2) + 1;

	// E1:5755-5759: spinning vertex setup
	int spinVertCount = 0;
	int headerSize = 4;
	if (bldFlag != 0)
	{
		spinVertCount = (bldFlag >> 2) + 1;
		td.spinSpeed = geo[4];
		td.spinBaseX = static_cast<int16_t>(static_cast<uint16_t>(geo[5]) << 8);
		td.spinBaseZ = static_cast<int16_t>(static_cast<uint16_t>(geo[6]) << 8);
		headerSize = 7; // 4 base + 3 spinning params
	}
	td.spinVertCount = static_cast<uint8_t>(spinVertCount);

	int connCount = (connRaw >> 2) + 1;
	const uint8_t *connData = geo + headerSize;
	const uint8_t *vtxData = connData + connCount * 2;

	// E1:5752: MOVE.B (A0)+,($0623A1) -- store connRaw as edge high-water
	g_workspace.roomEdgeHW = static_cast<uint16_t>(connRaw);

	// E1:5761-5772: copy connectivity pairs (byte 1 only, preserving byte 0)
	for (int i = 0; i < connCount && i < Workspace::ROOM_SLOT_MAX; i++)
	{
		g_workspace.edgeA[i] =
			(g_workspace.edgeA[i] & 0xFF00) | connData[i * 2];
		g_workspace.edgeB[i] =
			(g_workspace.edgeB[i] & 0xFF00) | connData[i * 2 + 1];
	}

	int32_t tileBaseX = cam.posX & static_cast<int32_t>(0xFFFF0000);
	int32_t tileBaseZ = cam.posZ & static_cast<int32_t>(0xFFFF0000);

	int totalVerts = (vtxCount < TileDetailState::MAX_SPIN_VERTS)
						 ? vtxCount
						 : TileDetailState::MAX_SPIN_VERTS;

	// Spinning vertices (E1:5791-5891, loc_044808)
	// Precompute log-space deltas and store world coords in workspace
	for (int i = 0; i < spinVertCount && i < totalVerts; i++)
	{
		int8_t dx = static_cast<int8_t>(vtxData[i * 3]);
		uint8_t hy = vtxData[i * 3 + 1];
		int8_t dz = static_cast<int8_t>(vtxData[i * 3 + 2]);

		td.spinLogDeltaX[i] = IntToLogFloat(static_cast<int32_t>(dx) * 64);
		td.spinLogDeltaZ[i] = IntToLogFloat(static_cast<int32_t>(dz) * 64);

		// E1:5792,5793: MOVE.L D0/D2,(A1/A3)+ -- tile base with low word 0
		g_workspace.vertLongX[i] = tileBaseX;
		// E1:5843-5844: ASL.W #8 -> byte * 256
		g_workspace.vertLongY[i] = static_cast<int32_t>(hy) * 256;
		g_workspace.vertLongZ[i] = tileBaseZ;
		// E1:5794: MOVE.L #0,(A6)+ -- zero velocity
		g_workspace.vertLongVel[i] = 0;
	}

	// Non-spinning vertices (E1:5894-5908)
	int nonSpinCount =
		(spinVertCount > 0) ? (vtxCount - spinVertCount) : vtxCount;
	for (int i = spinVertCount; i < spinVertCount + nonSpinCount &&
								i < TileDetailState::MAX_SPIN_VERTS;
		 i++)
	{
		uint8_t bx = vtxData[i * 3];
		uint8_t by = vtxData[i * 3 + 1];
		uint8_t bz = vtxData[i * 3 + 2];

		// E1:5899,5902: ASL.W #8 -> byte << 8, full longword write
		// High word preserved from D0/D2 = tile base (E1:5777/5779)
		g_workspace.vertLongX[i] = tileBaseX | (static_cast<int32_t>(bx) << 8);
		// E1:5900,5903: ASL.W #5 -> byte << 5
		g_workspace.vertLongY[i] = static_cast<int32_t>(by) << 5;
		// E1:5901,5904: ASL.W #8 -> byte << 8
		g_workspace.vertLongZ[i] = tileBaseZ | (static_cast<int32_t>(bz) << 8);
		// E1:5905: MOVE.L #0,(A6)+ -- zero velocity
		g_workspace.vertLongVel[i] = 0;
	}

	// Track how many vertices were actually stored (for sub_0448FC)
	td.tileVertCount = static_cast<uint8_t>(spinVertCount + nonSpinCount);
	if (td.tileVertCount > TileDetailState::MAX_SPIN_VERTS)
		td.tileVertCount = TileDetailState::MAX_SPIN_VERTS;

	// E1:5751: MOVE.B (A0)+,($0623A1) -- store vtxRaw (DBRA format)
	// vtxRaw = geo[0] = (vertexCount - 1) * 4
	g_workspace.roomVertHW = static_cast<uint16_t>(vtxRaw);

	// loc_0448C8 (E1:5909-5924): if tileProperty bit 7 is set (destroyed),
	// fast-collapse all vertices to ground immediately
	// TST.B ($062492); BMI loc_0448F4 -- if negative, loop sub_0448FC
	if (td.tileProperty < 0)
	{
		// loc_0448F4 (E1:5921-5924): BSR sub_0448FC; BNE loc_0448F4
		while (VertexGravityCollapse(td))
			;
	}
}

// sub_044640 (E1:5636-5739): outdoor tile detail load

void TileDetailLoad(TileDetailState &td, const Camera &cam)
{
	// E1:5688-5693: load camera X/Z bytes, compare with cached values
	uint8_t d0 = static_cast<uint8_t>((cam.posX >> 16) & 0xFF);
	uint8_t d1 = static_cast<uint8_t>((cam.posZ >> 16) & 0xFF);

	// E1:5690-5693: CMP.B ($0623B7),D0; BNE loc_0446C2;
	//               CMP.B ($0623BB),D1; BEQ loc_0446EE
	// $0623B7 = low byte of cachedPosX, $0623BB = low byte of cachedPosZ
	if (d0 != static_cast<uint8_t>(td.cachedPosX) ||
		d1 != static_cast<uint8_t>(td.cachedPosZ))
	{
		// E1:5694: tile changed -- fall through to loc_0446C2
		TileChangeAndLoad(td, cam, d0, d1);
	}

	// loc_0446EE (E1:5715-5739): D7 lookup handled by RoadsTerrainUpdate
	// E1:5734-5736: MOVE.W posX/posY/posZ high words to cache every frame
	td.cachedPosX = static_cast<uint16_t>(cam.posX >> 16);
	td.cachedPosY = static_cast<uint16_t>(cam.posY >> 16);
	td.cachedPosZ = static_cast<uint16_t>(cam.posZ >> 16);
}

// TileDetailReload: reload tile geometry for repair path (E1:1082)

void TileDetailReload(TileDetailState &td, const Camera &cam,
					  uint16_t tileIndex)
{
	(void)tileIndex; // tile index verified via camera position match

	// The original repair path at E1:1082 calls sub_044742 directly,
	// NOT via loc_0446C2.  So cachedD7 is NOT invalidated
	// TileChangeAndLoad begins with loc_0446C2 which sets cachedD7 = $FF
	// Preserve and restore cachedD7 to avoid the unwanted side effect
	uint8_t savedD7 = td.cachedD7;

	uint8_t d0 = static_cast<uint8_t>((cam.posX >> 16) & 0xFF);
	uint8_t d1 = static_cast<uint8_t>((cam.posZ >> 16) & 0xFF);
	TileChangeAndLoad(td, cam, d0, d1);

	td.cachedD7 = savedD7;
}

// sub_044630 (E1:5630-5633): tile change handler (elevator ascent path)

void TileDetailLoadDirect(TileDetailState &td, const Camera &cam)
{
	// E1:5631-5632: MOVE.B ($0623A7),D0; MOVE.B ($0623AF),D1
	uint8_t d0 = static_cast<uint8_t>((cam.posX >> 16) & 0xFF);
	uint8_t d1 = static_cast<uint8_t>((cam.posZ >> 16) & 0xFF);

	// E1:5633: BRA loc_0446C2
	TileChangeAndLoad(td, cam, d0, d1);

	// loc_0446EE fall-through (E1:5715-5731): D7 lookup
	// In the elevator call site (E1:11773), cachedD7 is immediately
	// overwritten to 0x01, so the D7 computation here has no lasting
	// effect -- but the original runs it, so we do too
	int altIndex = static_cast<int>(static_cast<uint32_t>(cam.posY) >> 11);
	if (altIndex < 0)
		altIndex = 0;
	if (altIndex >= gen_e3::ALT_TABLE_SIZE)
		altIndex = gen_e3::ALT_TABLE_SIZE - 1;
	uint8_t d7 = gen_e3::ALT_TO_D7[altIndex];
	if (d7 != td.cachedD7)
	{
		td.cachedD7 = d7;
		RoadsBuildWorkspace(cam, d7);
	}

	// E1:5734-5736: MOVE.W posX/posY/posZ high words to cache
	td.cachedPosX = static_cast<uint16_t>(cam.posX >> 16);
	td.cachedPosY = static_cast<uint16_t>(cam.posY >> 16);
	td.cachedPosZ = static_cast<uint16_t>(cam.posZ >> 16);
}

// TileDetailDraw

void TileDetailDraw(uint8_t *indexBuf, const TileDetailState &td,
					const Camera &cam)
{
	// E1:12762-12765: elevatorActive bypasses the altitude check
	//   TST.W $06244C        ; elevatorActive
	//   BNE loc_048AD6       ; if active, draw tile detail
	//   TST.W $0623AA        ; posY high word
	//   BNE loc_048AE6       ; if non-zero, skip tile detail (too high)
	if (cam.elevatorActive == 0)
	{
		uint16_t altWord =
			static_cast<uint16_t>(static_cast<uint32_t>(cam.posY) >> 16);
		if (altWord != 0)
		{
			return; // altitude >= 65536 -> no tile detail
		}
	}

	// E1:12768: antigrav ($0624A4) also skips tile detail
	if (cam.landingProx != 0)
	{
		return;
	}

	int connCount = (g_workspace.roomEdgeHW >> 2) + 1;
	int vtxCount = (g_workspace.roomVertHW >> 2) + 1;
	int drawModeSplit = td.drawModeSplit;

	bool vertProjected[64] = {};

	auto projectTileVertex = [&](int v)
	{
		if (vertProjected[v])
			return;
		if (v >= vtxCount)
			return;

		int32_t wx = g_workspace.vertLongX[v];
		int32_t wy = g_workspace.vertLongY[v];
		int32_t wz = g_workspace.vertLongZ[v];

		g_workspace.projVerts[v] = ProjectVertex(wx, wy, wz, cam);
		vertProjected[v] = true;
	};

	// Draw connectivity pairs in two passes: BSET (roads) then BCLR (buildings)
	// Mode switches at the exact edge index matching drawModeSplit

	int switchIdx = -1;
	if ((drawModeSplit & 3) == 0)
	{
		int candidate = drawModeSplit / 4;
		if (candidate < connCount)
			switchIdx = candidate;
	}

	auto drawConn = [&](int c, LineMode mode)
	{
		int slotA = g_workspace.edgeA[c] >> 2;
		int slotB = g_workspace.edgeB[c] >> 2;
		if (slotA < vtxCount)
			projectTileVertex(slotA);
		if (slotB < vtxCount)
			projectTileVertex(slotB);
		DrawLineProjectedMode(indexBuf, g_workspace.projVerts[slotA],
							  g_workspace.projVerts[slotB], cam, mode);
	};

	// Pass 1: BSET (roads) -- pairs above the mode-switch index.  When
	// switchIdx is -1 (no exact match possible) every pair draws BSET
	for (int c = 0; c < connCount; c++)
	{
		if (c <= switchIdx)
			continue;
		drawConn(c, LineMode::BSET);
	}

	// Pass 2: BCLR (buildings) -- pairs at or below the mode-switch
	// index.  Nothing draws when switchIdx stays at -1
	for (int c = 0; c < connCount; c++)
	{
		if (c > switchIdx)
			continue;
		drawConn(c, LineMode::BCLR);
	}
}
