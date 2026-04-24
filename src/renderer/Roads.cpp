// Coarse road grid -- sub_0442AA (E1:5214), sub_0444A2 (E1:5433)

#include "renderer/Roads.h"

#include "data/GameData.h"
#include "game/Workspace.h"
#include "renderer/LineDraw.h"
#include "renderer/LogMath.h"
#include "renderer/Projection.h"

#include <cstring>

// Coarse grid workspace ($0627F4)
// Unique line list (upward) + vertex byte list (downward) from common base

struct RoadWorkspace
{
	// Unique line list (A4 grows upward from index 0)
	// Each entry is a 16-bit line word encoding two vertex nibble-bytes
	// After the conversion pass, each word is replaced in-place with
	// a pair of vertex-offset bytes
	static constexpr int MAX_LINES = 64;
	uint16_t lineWords[MAX_LINES];
	int lineCount;

	// Vertex byte list (A3 grows downward)
	// Each byte encodes grid X (low nibble) and grid Z (high nibble)
	static constexpr int MAX_VERTS = 128;
	uint8_t vertBytes[MAX_VERTS];
	int vertCount;

	// Pre-seeded vertex bytes from the current tile ($0627E4-$0627F3)
	// ROR-unpacked from the spatial longword.  Indexed as:
	//   [0]=B, [4]=A, [8]=D, [12]=C of the tile longword [A][B][C][D]
	// Only bytes [0] and [8] are tested by the extra-lines code
	uint8_t preseeded[16];

	// Vertex count marker ($06241A) -- lowest allocated vertex offset
	uint8_t vertCountMarker;

	// Slot-to-vertex-byte mapping.  The original stores each vertex
	// byte at workspace address A0 + $26F4; we replicate that with
	// an explicit array indexed by offset/4
	// Slot 60=$F0, 61=$F4, 62=$F8, 63=$FC are pre-seeded
	// Projected vertices for these slots live in g_workspace.projVerts
	uint8_t slotVertByte[64];
};

static RoadWorkspace s_roadWs;

// sub_044490: deduplication (E1:5417-5430)
// Linear scan of the unique line list.  If the word is already present,
// skip.  Otherwise append

static void RoadsDedup(uint16_t word)
{
	if (word == 0)
		return;
	for (int i = 0; i < s_roadWs.lineCount; i++)
	{
		if (s_roadWs.lineWords[i] == word)
			return;
	}
	if (s_roadWs.lineCount < RoadWorkspace::MAX_LINES)
		s_roadWs.lineWords[s_roadWs.lineCount++] = word;
}

// sub_0442AA: build workspace (E1:5214-5430)
// Called from RoadsTerrainUpdate when D7 changes

void RoadsBuildWorkspace(const Camera &cam, int d7)
{
	// Phase 1: compute tile window range (E1:5215-5272)
	// D6 = 2*D7+1 (window diameter)
	int d6 = 2 * d7 + 1;
	int startX = 0, countX = 0, startZ = 0, countZ = 0;

	// E1:5218: D1 = signed high word of posX
	int d1x = static_cast<int16_t>(cam.posX >> 16);
	d1x -= d7; // E1:5219: SUB.W D7,D1
	if (d1x < 0)
	{
		// E1:5221: ADD.W D6,D1 -> remaining count
		d1x += d6;
		if (d1x <= 0)
		{
			d7 = 0;
			goto ranges_done;
		}
		if (d1x > 16)
		{
			d1x = 16;
		}
		startX = 0;
		countX = d1x;
	}
	else
	{
		// loc_0442CA
		if (d1x >= 16)
		{
			d7 = 0;
			goto ranges_done;
		}
		// D0 = 16 - D1; EXG D0,D1 -> D0=start, D1=count
		countX = 16 - d1x;
		startX = d1x;
		if (countX > d6)
		{
			countX = d6;
		}
	}

	{
		// E1:5244: D1 = signed high word of posZ
		int d1z = static_cast<int16_t>(cam.posZ >> 16);
		d1z -= d7;
		if (d1z < 0)
		{
			d1z += d6;
			if (d1z <= 0)
			{
				d7 = 0;
				goto ranges_done;
			}
			if (d1z > 16)
			{
				d1z = 16;
			}
			startZ = 0;
			countZ = d1z;
		}
		else
		{
			if (d1z >= 16)
			{
				d7 = 0;
				goto ranges_done;
			}
			countZ = 16 - d1z;
			startZ = d1z;
			if (countZ > d6)
			{
				countZ = d6;
			}
		}
	}

ranges_done:

	// Init workspace
	s_roadWs.lineCount = 0;
	s_roadWs.vertCount = 0;
	std::memset(s_roadWs.preseeded, 0, 16);
	std::memset(s_roadWs.slotVertByte, 0, 64);

	// Phase 2: pre-seed current tile (E1:5297-5302)
	// ANDI.W #$F0 on the high word of posX/posZ
	uint16_t posXHi = static_cast<uint16_t>(cam.posX >> 16);
	uint16_t posZHi = static_cast<uint16_t>(cam.posZ >> 16);
	bool insideGrid = ((posXHi & 0xF0) == 0) && ((posZHi & 0xF0) == 0);

	if (insideGrid && g_workspace.tileDetail.currentTileIndex != 0)
	{
		// Read the current tile's spatial longword from E3
		int spatOff = g_workspace.tileDetail.currentTileIndex * 4;
		uint32_t longword =
			(static_cast<uint32_t>(gen_e3::E3_RAW[spatOff]) << 24) |
			(static_cast<uint32_t>(gen_e3::E3_RAW[spatOff + 1]) << 16) |
			(static_cast<uint32_t>(gen_e3::E3_RAW[spatOff + 2]) << 8) |
			gen_e3::E3_RAW[spatOff + 3];

		// MOVE.L D6,(A4)+ -- store two 16-bit line words
		// On 68000 big-endian: first word at (A4) = high word of longword
		// In our array: lineWords[0] = high word, lineWords[1] = low word
		s_roadWs.lineWords[0] = static_cast<uint16_t>(longword >> 16);
		s_roadWs.lineWords[1] = static_cast<uint16_t>(longword);
		s_roadWs.lineCount = 2;

		// ROR unpack (E1:5321-5330)
		// Input longword bytes: [A][B][C][D] (big-endian)
		uint8_t A = (longword >> 24) & 0xFF;
		uint8_t B = (longword >> 16) & 0xFF;
		uint8_t C = (longword >> 8) & 0xFF;
		uint8_t D = longword & 0xFF;

		// After ROR sequence, first byte at each 4-byte write:
		//   $0627F0 = C,  $0627EC = D,  $0627E8 = A,  $0627E4 = B
		// We store with the same indexing: [0]=B, [4]=A, [8]=D, [12]=C
		s_roadWs.preseeded[0] = B;	// $0627E4
		s_roadWs.preseeded[4] = A;	// $0627E8
		s_roadWs.preseeded[8] = D;	// $0627EC
		s_roadWs.preseeded[12] = C; // $0627F0

		// In the original, the projection loop in sub_0444A2 reads vertex
		// bytes directly from $0627E4-$0627F0 via 9972(A0) indexing for
		// slots $F0-$FC.  Mirror that: slots 60-63 always reflect the
		// pre-seeded bytes regardless of whether the conversion pass runs
		s_roadWs.slotVertByte[60] = B; // slot $F0 reads from $0627E4
		s_roadWs.slotVertByte[61] = A; // slot $F4 reads from $0627E8
		s_roadWs.slotVertByte[62] = D; // slot $F8 reads from $0627EC
		s_roadWs.slotVertByte[63] = C; // slot $FC reads from $0627F0
	}
	else
	{
		// Outside grid: zero everything, lineCount stays 0
		// But we still need 2 zero entries for the pre-seed slots
		s_roadWs.lineWords[0] = 0;
		s_roadWs.lineWords[1] = 0;
		s_roadWs.lineCount = 2;
	}

	// Phase 3: D7=0 early exit (E1:5333-5336)
	if (d7 == 0)
	{
		s_roadWs.vertCountMarker = 0xF0;
		// Draw boundary: lineCount stays at 2 (pre-seeded only)
		// Phase 2 of sub_0444A2 will skip because there are no
		// entries beyond the pre-seeded pair
		return;
	}

	// Phase 4: tile iteration (E1:5340-5367)
	for (int tz = startZ; tz < startZ + countZ; tz++)
	{
		for (int tx = startX; tx < startX + countX; tx++)
		{
			int tileOff = (tz * 16 + tx) * 4;
			uint32_t tw =
				(static_cast<uint32_t>(gen_e3::E3_RAW[tileOff]) << 24) |
				(static_cast<uint32_t>(gen_e3::E3_RAW[tileOff + 1]) << 16) |
				(static_cast<uint32_t>(gen_e3::E3_RAW[tileOff + 2]) << 8) |
				gen_e3::E3_RAW[tileOff + 3];

			// Each tile longword has two 16-bit line words
			// E1:5348-5351: dedup low word, SWAP, dedup high word
			RoadsDedup(static_cast<uint16_t>(tw));
			RoadsDedup(static_cast<uint16_t>(tw >> 16));
		}
	}

	// Phase 5: clear lookup table (E1:5357-5367)
	uint8_t lut[256] = {};

	// Phase 6: pre-assign vertex offsets (E1:5370-5378)
	// Bytes from preseeded[12]/[8]/[4]/[0] get offsets $FC/$F8/$F4/$F0
	if (s_roadWs.preseeded[12] != 0)
	{
		lut[s_roadWs.preseeded[12]] = 0xFC;
		s_roadWs.slotVertByte[0xFC / 4] = s_roadWs.preseeded[12];
	}
	if (s_roadWs.preseeded[8] != 0)
	{
		lut[s_roadWs.preseeded[8]] = 0xF8;
		s_roadWs.slotVertByte[0xF8 / 4] = s_roadWs.preseeded[8];
	}
	if (s_roadWs.preseeded[4] != 0)
	{
		lut[s_roadWs.preseeded[4]] = 0xF4;
		s_roadWs.slotVertByte[0xF4 / 4] = s_roadWs.preseeded[4];
	}
	if (s_roadWs.preseeded[0] != 0)
	{
		lut[s_roadWs.preseeded[0]] = 0xF0;
		s_roadWs.slotVertByte[0xF0 / 4] = s_roadWs.preseeded[0];
	}

	// Phase 7: conversion pass (E1:5380-5413)
	// D4 starts at $F0 (E1:5379).  First new allocation gets $EC
	uint8_t d4 = 0xF0;
	s_roadWs.vertCount = 0;

	for (int i = 0; i < s_roadWs.lineCount; i++)
	{
		uint16_t word = s_roadWs.lineWords[i];
		if (word == 0)
		{
			s_roadWs.lineWords[i] = 0;
			continue;
		}

		uint8_t vtxA = static_cast<uint8_t>(word >> 8);
		uint8_t vtxB = static_cast<uint8_t>(word);

		uint8_t offA = lut[vtxA];
		if (offA == 0)
		{
			d4 -= 4;
			offA = d4;
			lut[vtxA] = offA;
			s_roadWs.slotVertByte[offA / 4] = vtxA;
			if (s_roadWs.vertCount < RoadWorkspace::MAX_VERTS)
				s_roadWs.vertBytes[s_roadWs.vertCount++] = vtxA;
		}

		uint8_t offB = lut[vtxB];
		if (offB == 0)
		{
			d4 -= 4;
			offB = d4;
			lut[vtxB] = offB;
			s_roadWs.slotVertByte[offB / 4] = vtxB;
			if (s_roadWs.vertCount < RoadWorkspace::MAX_VERTS)
				s_roadWs.vertBytes[s_roadWs.vertCount++] = vtxB;
		}

		// Replace line word in-place with offset byte pair
		s_roadWs.lineWords[i] = static_cast<uint16_t>((offA << 8) | offB);
	}

	s_roadWs.vertCountMarker = d4;
}

// RoadsTerrainUpdate: sub_044640 D7/tile change logic (E1:5688-5739)
// Called every frame.  Detects tile changes (invalidates D7 cache) and
// D7 changes (triggers workspace rebuild)

void RoadsTerrainUpdate(Camera &cam)
{
	// E1:5688-5693: tile change detection
	// Compare camera tile bytes against cached position words
	uint8_t tileX = (cam.posX >> 16) & 0xFF;
	uint8_t tileZ = (cam.posZ >> 16) & 0xFF;

	// Recompute current tile index (sub_044640 E1:5697-5711)
	uint8_t d3 = tileX | tileZ;
	uint16_t tileIdx;
	if (d3 & 0xF0)
		tileIdx = 0; // off-grid
	else
		tileIdx = ((tileZ & 0xF) << 4) | (tileX & 0xF);

	if (tileIdx != g_workspace.tileDetail.currentTileIndex)
	{
		// Tile changed: invalidate D7 cache to force rebuild
		g_workspace.tileDetail.currentTileIndex = tileIdx;
		g_workspace.tileDetail.cachedD7 = 0xFF; // E1:5696
	}

	// E1:5715-5731: D7 lookup
	int altIndex = static_cast<int>(static_cast<uint32_t>(cam.posY) >> 11);
	if (altIndex < 0)
		altIndex = 0;
	if (altIndex >= gen_e3::ALT_TABLE_SIZE)
		altIndex = gen_e3::ALT_TABLE_SIZE - 1;
	uint8_t d7 = gen_e3::ALT_TO_D7[altIndex];

	if (d7 != g_workspace.tileDetail.cachedD7)
	{
		g_workspace.tileDetail.cachedD7 = d7;
		RoadsBuildWorkspace(cam, d7);
	}
}

// sub_0444A2 (E1:5433-5636): draw the coarse road grid

void RoadsDraw(uint8_t *indexBuf, const Camera &cam)
{
	// E1:5435-5437: skip if road drawing is disabled ($06248C)
	if (cam.roadDrawDisable != 0)
		return;

	// Phase 1: project vertices (E1:5487-5590)
	// Iterate all vertex slots, project those with non-zero vertex bytes
	uint32_t lfY = IntToLogFloat31(-cam.posY);

	int firstSlot = s_roadWs.vertCountMarker / 4;
	for (int slot = firstSlot; slot < 64; slot++)
	{
		uint8_t vtxByte = s_roadWs.slotVertByte[slot];
		if (vtxByte == 0)
			continue;

		int gridX = vtxByte & 0x0F;
		int gridZ = (vtxByte >> 4) & 0x0F;

		// The original initialises D3 = $80000000 before MOVE.B vtxByte,
		// so after SWAP the low word is $8000, adding half a grid cell
		// offset: worldCoord = (gridNibble << 16) | 0x8000
		int32_t worldX = (gridX << 16) | 0x8000;
		int32_t worldZ = (gridZ << 16) | 0x8000;

		uint32_t lfX = IntToLogFloat(worldX - cam.posX);
		uint32_t lfZ = IntToLogFloat(worldZ - cam.posZ);

		g_workspace.projVerts[slot] = ProjectVertexLogFloat(lfX, lfY, lfZ, cam);
	}

	// Phase 2: draw connectivity pairs (E1:5591-5604)
	// Backward from lineWords[lineCount-1] to lineWords[2]
	// Indices 0-1 are pre-seeded (current tile) -- skipped
	for (int i = s_roadWs.lineCount - 1; i >= 2; i--)
	{
		uint16_t word = s_roadWs.lineWords[i];
		if (word == 0)
			continue;

		int slotA = (word >> 8) / 4;
		int slotB = (word & 0xFF) / 4;

		DrawLineProjected(indexBuf, g_workspace.projVerts[slotA],
						  g_workspace.projVerts[slotB], cam);
	}

	// Phase 3: extra lines at high altitude (E1:5606-5627)
	if (cam.renderMode != 0)
		return;
	uint16_t altHiWord =
		static_cast<uint16_t>(static_cast<uint32_t>(cam.posY) >> 16);
	if (altHiWord == 0)
		return;

	if (s_roadWs.preseeded[0] != 0)
		DrawLineProjected(indexBuf, g_workspace.projVerts[60],
						  g_workspace.projVerts[61], cam);

	if (s_roadWs.preseeded[8] != 0)
		DrawLineProjected(indexBuf, g_workspace.projVerts[62],
						  g_workspace.projVerts[63], cam);
}

// Tile detail functions (TileChangeAndLoad, TileDetailLoad,
// TileDetailLoadDirect, TileDetailReload, TileDetailDraw) have
// been moved to TileDetail.cpp
