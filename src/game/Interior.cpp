// Indoor scene management -- room loader and projection prep

#include "game/Interior.h"

#include "data/GameData.h"
#include "game/Game.h"
#include "game/ObjectSlots.h"
#include "game/Objects.h"
#include "game/Workspace.h"
#include "renderer/Hud.h"
#include "renderer/LineDraw.h"
#include "renderer/LogMath.h"
#include "renderer/Projection.h"

#include <cstring>

static uint8_t *GetRoomData(uint16_t room)
{
	if (room == 0 || room >= gen_e3::ROOM_COUNT)
		return nullptr;
	uint16_t virtOff = gen_e3::ROOM_OFFSETS[room];
	if (virtOff == 0 || virtOff < gen_e3::ROOM_DATA_BASE_OFFSET)
		return nullptr;
	int fileOff = virtOff - gen_e3::ROOM_DATA_BASE_OFFSET;
	if (fileOff < 0 || fileOff >= gen_e3::ROOM_DATA_SIZE)
		return nullptr;
	return &gen_e3::ROOM_DATA[fileOff];
}

// Template walk (E1:6374-6411)
// Expands a room template into vertex positions and pairwise edge connections

static void TemplateWalk(uint8_t cmdByte, uint8_t dataX, uint8_t dataZ,
						 bool isFloor)
{
	uint8_t tmplIdx = cmdByte & 0x0F;
	if (tmplIdx >= 16)
		return;
	uint8_t tmplOff = gen_e1::TEMPLATE_INDEX[tmplIdx];
	if (tmplOff >= gen_e1::INTERIOR_TEMPLATES_SIZE)
		return;
	const uint8_t *tpl = &gen_e1::INTERIOR_TEMPLATES[tmplOff];

	uint16_t a4 = g_workspace.roomVertHW;
	uint16_t a5 = g_workspace.roomEdgeHW;
	uint16_t d5 = a4;

	while (true)
	{
		uint8_t attr = *tpl++;
		if (attr == 0xFF)
			break;
		uint8_t yByte = *tpl++;

		a4 += 4;
		int idx = a4 >> 2;
		if (idx >= Workspace::ROOM_SLOT_MAX)
			break;

		g_workspace.vertLongX[idx] = (static_cast<int32_t>(dataX) << 8) |
									 (isFloor ? 0 : static_cast<int32_t>(attr));
		g_workspace.vertLongY[idx] = static_cast<int32_t>(yByte); // byte 3
		g_workspace.vertLongZ[idx] = (static_cast<int32_t>(dataZ) << 8) |
									 (isFloor ? static_cast<int32_t>(attr) : 0);
	}

	// E1:6400-6408: pairwise edge connections
	d5 += 4;
	while (d5 < a4)
	{
		a5 += 4;
		int eIdx = a5 >> 2;
		if (eIdx >= Workspace::ROOM_SLOT_MAX)
			break;
		g_workspace.edgeA[eIdx] = d5;
		d5 += 4;
		g_workspace.edgeB[eIdx] = d5;
	}

	g_workspace.roomVertHW = a4;
	g_workspace.roomEdgeHW = a5;
}

// sub_044B5A (E1:6204-6326) + loc_044D64 (E1:6328-6411)

void RoomLoad(uint16_t room, uint8_t buildingIndex, ScriptVM &vm)
{
	Camera &cam = g_workspace.cam;

	room &= 0x00FF;
	if (room == 0)
	{
		// Surface-return path (E1:6207-6218)
		cam.renderMode = 0;
		g_workspace.objs.currentRoom = 0;
		cam.palBase89 = PAL_DEFAULT_89;
		cam.palBase1011 = PAL_DEFAULT_1011;
		cam.posX = 0x000867C0;
		cam.posY = 0x0040F480;
		cam.posZ = 0x00087000;
		g_workspace.tileDetail.cachedPosX =
			(g_workspace.tileDetail.cachedPosX & 0xFF00) | 0x00FF;
		ObjectsBuildActiveList(g_workspace.objs);
		HudFactionLED();
		return;
	}

	// Indoor room path (E1:6220-6326)
	cam.altFactor = (cam.altFactor & 0x0000FFFFu) | 0xFFF00000u;

	uint8_t *rd = GetRoomData(room);
	if (rd == nullptr)
		return;
	cam.doorTablePtr = static_cast<uint32_t>(rd - gen_e3::ROOM_DATA);

	uint8_t byte0 = rd[0];
	if (static_cast<int8_t>(byte0) < 0)
	{
		// E1:6230-6266: special room -- select teleport destination
		uint8_t d3, d4;
		bool apply = true;

		if (room == 0x0072)
		{
			// E1:6232-6244: room $72 reads RNG for random destination
			uint16_t d2 = RngWord(vm) & 0x000E;
			d3 = rd[12 + d2];
			d4 = rd[13 + d2];
		}
		else
		{
			// E1:6246-6254: default pair, skip if already in that room
			d3 = rd[12];
			d4 = rd[13];
			if (d3 == g_workspace.objs.currentRoom)
			{
				d3 = rd[14];
				d4 = rd[15];
				if (d3 == g_workspace.objs.currentRoom)
					apply = false; // BEQ loc_044C5E
			}
		}

		if (apply)
		{
			// loc_044C2E (E1:6257-6261): patch door entry in room data
			rd[7] = (rd[7] & 0x8F) | d4;
			rd[8] = d3;

			// E1:6262
			cam.transCooldown = 0x0028;

			// E1:6263-6266: mirror toggle for room $7F
			if (static_cast<uint8_t>(room) == 0x7F)
			{
				cam.mirrorMask ^= 0xFFFF;
				cam.transCooldown = 0x0050;
			}
		}
	}

	// E1:6268-6277 (loc_044C5E)
	uint8_t d1 = buildingIndex;
	g_workspace.objs.currentRoom = static_cast<uint8_t>(room);
	// E1:6271: MOVE.B 9(A0,D1), ($0623A8) -- byte 2 of posX
	cam.posX =
		(cam.posX & 0xFFFF00FFu) | (static_cast<uint32_t>(rd[9 + d1]) << 8);
	// E1:6272: MOVE.B 10(A0,D1), ($0623B0) -- byte 2 of posZ
	cam.posZ =
		(cam.posZ & 0xFFFF00FFu) | (static_cast<uint32_t>(rd[10 + d1]) << 8);
	// E1:6273-6277: sign byte at byte 3 of posX and posZ
	//   D2 = MOVE.B 7(A0,D1); ANDI.B #$80
	//   MOVE.B D2, ($0623B1)       -- posZ byte 3 = bit 7
	//   EORI.B #$80, D2
	//   MOVE.B D2, ($0623A9)       -- posX byte 3 = inverted bit 7
	{
		uint8_t signByte = rd[7 + d1] & 0x80;
		cam.posZ = (cam.posZ & 0xFFFFFF00u) | signByte;
		cam.posX = (cam.posX & 0xFFFFFF00u) | (signByte ^ 0x80);
	}

	// E1:6278: ObjectsBuildActiveList
	ObjectsBuildActiveList(g_workspace.objs);
	HudFactionLED();

	// E1:6279-6281: posX/Z high words = 0
	cam.posX &= 0x0000FFFFu;
	cam.posZ &= 0x0000FFFFu;

	// E1:6282-6290: zero vertex workspace (iterate from A1=$0600FC
	// down to $000000, clearing three longword arrays per slot)
	for (int i = 0; i < Workspace::ROOM_SLOT_MAX; i++)
	{
		g_workspace.vertLongX[i] = 0;
		g_workspace.vertLongY[i] = 0;
		g_workspace.vertLongZ[i] = 0;
	}
	std::memset(g_workspace.edgeA, 0, sizeof(g_workspace.edgeA));
	std::memset(g_workspace.edgeB, 0, sizeof(g_workspace.edgeB));

	// E1:6291-6299: copy edge template into first 12 slots
	{
		static const uint16_t EDGE_A_PRESEED[12] = {
			0x0000, 0x0004, 0x0008, 0x000C, 0x0000, 0x0004,
			0x0008, 0x000C, 0x0010, 0x0014, 0x0018, 0x001C,
		};
		static const uint16_t EDGE_B_PRESEED[12] = {
			0x0004, 0x0008, 0x000C, 0x0000, 0x0010, 0x0014,
			0x0018, 0x001C, 0x0014, 0x0018, 0x001C, 0x0010,
		};
		for (int i = 0; i < 12; i++)
		{
			int slot = 11 - i;
			g_workspace.edgeA[slot] = EDGE_A_PRESEED[i];
			g_workspace.edgeB[slot] = EDGE_B_PRESEED[i];
		}
	}

	// E1:6300-6319: room extents -> vertex slots
	cam.roomExtentX = static_cast<uint8_t>(rd[0] & 0x7F);
	cam.roomExtentZ = static_cast<uint8_t>(rd[2] & 0x7F);
	{
		uint8_t extX = rd[0] & 0x7F;
		uint8_t extY = rd[1] & 0x07;
		uint8_t extZ = rd[2] & 0x7F;

		int32_t xVal = static_cast<int32_t>(extX) << 8;
		int32_t yVal = static_cast<int32_t>(extY) << 8;
		int32_t zVal = static_cast<int32_t>(extZ) << 8;

		g_workspace.vertLongX[0] = xVal;
		g_workspace.vertLongX[1] = xVal;
		g_workspace.vertLongX[4] = xVal;
		g_workspace.vertLongX[5] = xVal;

		g_workspace.vertLongY[0] = yVal;
		g_workspace.vertLongY[1] = yVal;
		g_workspace.vertLongY[2] = yVal;
		g_workspace.vertLongY[3] = yVal;

		g_workspace.vertLongZ[0] = zVal;
		g_workspace.vertLongZ[3] = zVal;
		g_workspace.vertLongZ[4] = zVal;
		g_workspace.vertLongZ[7] = zVal;
	}

	// E1:6320-6321: initial high-water marks
	g_workspace.roomVertHW = 0x001C; // 7 * 4 = 28 (slots 0-6 reserved)
	g_workspace.roomEdgeHW = 0x002C; // 11 * 4 = 44 (slots 0-10 reserved)

	// E1:6322-6325: palette bytes -> palBase1011
	// A1 has consumed 3 bytes (extents), so reads rd[3..6]
	cam.palBase1011 = (static_cast<uint32_t>(rd[3]) << 24) |
					  (static_cast<uint32_t>(rd[4]) << 16) |
					  (static_cast<uint32_t>(rd[5]) << 8) |
					  static_cast<uint32_t>(rd[6]);

	// E1:6326: palBase89 high word = $0777
	cam.palBase89 = (cam.palBase89 & 0x0000FFFFu) | 0x07770000u;

	// E1:6328+ (loc_044D64): object-placement byte stream
	// A1 is at rd + 7 after 3 extent + 4 palette bytes
	// D7 tracks the pending room message index (E1:6279: MOVEQ #0,D7)
	const uint8_t *cmd = rd + 7;
	const uint8_t *end = &gen_e3::ROOM_DATA[gen_e3::ROOM_DATA_SIZE];
	uint16_t d7 = 0;

	while (cmd < end)
	{
		uint8_t c = *cmd++;
		if (c == 0x00)
		{
			// End of stream -> fall through to loc_044D94
			break;
		}
		if (static_cast<int8_t>(c) >= 0)
		{
			// Positive: wall command.  loc_044DDE
			if (cmd + 3 > end)
				break;
			cmd++; // E1:6381: ADDQ.W #1, A1 -- skip one byte
			uint8_t dataX = *cmd++;
			uint8_t dataZ = *cmd++;
			TemplateWalk(c, dataX, dataZ, false);
			continue;
		}
		if (c == 0x80)
		{
			// E1:6332-6341: dark room
			// TST.B ($0631C4) -- photon emitter (bit 7 = player has it)
			// BMI loc_044D94 -> skip blackout AND d7 stays 0 (no message)
			if ((g_workspace.objs.flagsTable[OBJ_PHOTON_EMITTER] &
				 OBJ_FLAG_TAKEN) == 0)
			{
				// E1:6336-6337: CMP.B ($063144),D0 -- room visited?
				// BEQ loc_044D94 skips both palette clear and MOVEQ
				if (static_cast<uint8_t>(room) !=
					g_workspace.objs.slotTable[OBJ_PHOTON_EMITTER])
				{
					// E1:6338-6340: MOVE.W D7(=0) to palette base
					cam.palBase89 &= 0x0000FFFFu;
					cam.palBase1011 = 0;
					// E1:6341: MOVEQ #1,D7 -- dark room message
					d7 = 1;
				}
			}
			// Fall through to loc_044D94 (same as end-of-stream)
			break;
		}
		// Negative non-$80: floor/ceiling command.  loc_044DE6
		if (cmd + 3 > end)
			break;
		cmd++;
		uint8_t dataX = *cmd++;
		uint8_t dataZ = *cmd++;
		TemplateWalk(c, dataX, dataZ, true);
	}

	// loc_044D94 (E1:6343-6365): end of stream / dark room fallthrough
	// Both the $00 end marker and $80 dark room marker reach here

	// E1:6344: MOVE.W #$0100, ($0623DC) -- drawModeSplit
	// Sets the BSET/BCLR mode-switch threshold to $0100 (256)
	// All indoor edges draw as BSET (the match never fires because
	// edge offsets are much smaller).  RoomDrawEdges already
	// unconditionally uses BSET, so no field needed

	// E1:6345-6353: event script dispatch or pending message
	{
		uint8_t d2 = static_cast<uint8_t>(rd[1]) >> 3;
		if (d2 != 0 && static_cast<int8_t>(rd[2]) < 0)
		{
			// E1:6348-6353: BSR sub_04418C (ScriptVMLoadEvent)
			// Then BRA sub_044F38 (returns without setting
			// pendingRoomMsg)
			g_workspace.pendingEventSlot = static_cast<int16_t>(d2);
		}
		else
		{
			// E1:6356: MOVE.W D7, ($06249E) -- pending msg
			cam.pendingRoomMsg = d7;
		}
	}
}

// Indoor projection prep (sub_046CAE, E1:9530-9672)

void RoomProjectVertices(const Camera &cam)
{
	// E1:9531-9533: A1 = posX, A2 = posY, A3 = posZ
	int32_t camX = cam.posX;
	int32_t camY = cam.posY;
	int32_t camZ = cam.posZ;

	// E1:9534: A0 = vertex high-water, iterate down to 0
	int hwByteOff = static_cast<int>(g_workspace.roomVertHW);

	for (int off = hwByteOff; off >= 0; off -= 4)
	{
		int idx = off >> 2;
		if (idx < 0 || idx >= Workspace::ROOM_SLOT_MAX)
			continue;

		// E1:9539: D3 = MOVE.L 5134(A0) -- X longword
		// E1:9582: D4 = MOVE.L 5390(A0) -- Y longword
		// E1:9625: D5 = MOVE.L 5646(A0) -- Z longword
		int32_t dx = g_workspace.vertLongX[idx] - camX;
		int32_t dy = g_workspace.vertLongY[idx] - camY;
		int32_t dz = g_workspace.vertLongZ[idx] - camZ;

		// E1:9539-9579: IntToLogFloat with exponent 23
		uint32_t lfX = IntToLogFloat(dx);
		uint32_t lfY = IntToLogFloat(dy);
		uint32_t lfZ = IntToLogFloat(dz);

		// E1:9668: BSR sub_046DA6 -- projection (dispatches to
		// loc_0471FE for indoor via renderMode check)
		g_workspace.projVerts[idx] = ProjectVertexLogFloat(lfX, lfY, lfZ, cam);
	}
}

// Indoor edge draw (sub_04741E, E1:10396-10415)

void RoomDrawEdges(uint8_t *indexBuf, const Camera &cam)
{
	int hwByteOff = static_cast<int>(g_workspace.roomEdgeHW);

	for (int off = hwByteOff; off >= 0; off -= 4)
	{
		int eIdx = off >> 2;
		if (eIdx < 0 || eIdx >= Workspace::ROOM_SLOT_MAX)
			continue;

		int slotA = g_workspace.edgeA[eIdx] >> 2;
		int slotB = g_workspace.edgeB[eIdx] >> 2;
		if (slotA >= Workspace::ROOM_SLOT_MAX ||
			slotB >= Workspace::ROOM_SLOT_MAX)
			continue;

		// All indoor edges are BSET (drawModeSplit = $0100,
		// never matches any edge offset)
		DrawLineProjected(indexBuf, g_workspace.projVerts[slotA],
						  g_workspace.projVerts[slotB], cam);
	}
}

// sub_044F38 (E1:6534-6582)
// Clamp posX/posZ to room bounds, return true when at a boundary

bool RoomBoundaryClamp(Camera &cam)
{
	// E1:6534-6536: skip if elevator active
	if (cam.elevatorActive != 0)
		return false;

	// E1:6537-6540: D1 = posX low word, D2 = posZ low word
	// The original reads WORD at $0623A8 (byte 2-3 of posX longword)
	// and $0623B0 (byte 2-3 of posZ longword).  For indoor positions
	// the high word is zero, so the low word IS the full position
	int16_t d1 = static_cast<int16_t>(cam.posX & 0xFFFF);
	int16_t d2 = static_cast<int16_t>(cam.posZ & 0xFFFF);
	bool clamped = false;

	// E1:6541-6544: clamp X low bound
	if (d1 <= 0x0020)
	{
		d1 = 0x0020;
		clamped = true;
	}
	// E1:6547-6550: clamp Z low bound
	if (d2 <= 0x0020)
	{
		d2 = 0x0020;
		clamped = true;
	}

	// E1:6553-6558: clamp X high bound
	// D3 = MOVE.W $061410 -> word-level extent, then SUBI.W #$20
	int16_t extXWord = static_cast<int16_t>(g_workspace.vertLongX[0] & 0xFFFF);
	int16_t xHigh = static_cast<int16_t>(extXWord - 0x0020);
	if (d1 > xHigh)
	{
		d1 = xHigh;
		clamped = true;
	}

	// E1:6561-6566: clamp Z high bound
	int16_t extZWord = static_cast<int16_t>(g_workspace.vertLongZ[0] & 0xFFFF);
	int16_t zHigh = static_cast<int16_t>(extZWord - 0x0020);
	if (d2 > zHigh)
	{
		d2 = zHigh;
		clamped = true;
	}

	// E1:6569-6570: write back as longwords (high word = 0)
	cam.posX = static_cast<uint32_t>(static_cast<uint16_t>(d1));
	cam.posZ = static_cast<uint32_t>(static_cast<uint16_t>(d2));

	// E1:6571-6576: edge detection for door-wipe trigger
	// Compare previous clamp state against current; on rising edge
	// (was 0, now 1) set roomChangedFlag
	uint8_t prevClamp = cam.wallContactState & 0x01;
	if (clamped)
		cam.wallContactState |= 0x01;
	else
		cam.wallContactState &= ~0x01;

	if (clamped && prevClamp == 0)
		cam.roomChangedFlag = 1;

	return clamped;
}

// sub_044FC2 (E1:6589-6679)
// Scan door table for a proximity + heading match

bool RoomDoorScan(Camera &cam, Game &game, uint16_t &outRoom,
				  uint8_t &outBuildingIndex)
{
	// E1:6590-6591: skip if transCooldown != 0
	if (cam.transCooldown != 0)
		return false;

	// E1:6592: A0 = doorTablePtr (room data base)
	uint32_t tableOff = cam.doorTablePtr;

	if (tableOff >= static_cast<uint32_t>(gen_e3::ROOM_DATA_SIZE))
		return false;
	const uint8_t *base = &gen_e3::ROOM_DATA[tableOff];
	const uint8_t *end = &gen_e3::ROOM_DATA[gen_e3::ROOM_DATA_SIZE];

	// E1:6595-6599: D2 = posX word, D3 = posZ word,
	// D4 = compressed heading = (heading + $0280) >> 8
	uint16_t d2 = static_cast<uint16_t>(cam.posX & 0xFFFF);
	uint16_t d3 = static_cast<uint16_t>(cam.posZ & 0xFFFF);
	uint16_t d4 = static_cast<uint16_t>((cam.heading + 0x0280) >> 8);

	// E1:6600: ADDQ.W #6,A0 then falls through to loc_044FEE
	// (E1:6603: ADDQ.W #1,A0) before the first read -- total skip = 7
	const uint8_t *a0 = base + 7;

	// loc_044FEE / loc_044FF0: scan loop
	while (a0 + 4 <= end)
	{
		// loc_044FF0 entry: read descriptor and secondary
		uint8_t desc = *a0++; // E1:6606
		uint8_t d7mask = desc & 0x7F;
		if (d7mask == 0) // E1:6609: end sentinel
			return false;
		uint8_t secondary = *a0++; // E1:6610

		// Test 1 (E1:6611-6616): X proximity
		// D7 = (desc & $80) + posX_word, then >> 8, compare to (A0)+
		uint16_t t1 = static_cast<uint16_t>(desc & 0x80) + d2;
		uint8_t t1hi = static_cast<uint8_t>(t1 >> 8);
		uint8_t xCheck = *a0++; // E1:6615
		if (t1hi != xCheck)
		{
			// E1:6616: BNE loc_044FEE -> skip 1 byte, retry
			a0++; // skip z_check
			continue;
		}

		// Test 2 (E1:6617-6620): Z sign-side
		// D7 = posZ - $40; EOR.B desc, D7; BPL -> fail (need BMI)
		{
			uint16_t t2 = static_cast<uint16_t>(d3 - 0x0040);
			uint8_t t2lo = static_cast<uint8_t>(t2) ^ desc;
			if ((t2lo & 0x80) == 0) // BPL = not negative -> fail
			{
				a0++; // skip z_check (loc_044FEE path)
				continue;
			}
		}

		// Test 3 (E1:6621-6627): Z proximity
		// D7 = (desc & $80) ^ $80 + posZ_word, then >> 8, compare (A0)+
		{
			uint16_t t3bit = static_cast<uint16_t>((desc & 0x80) ^ 0x80);
			uint16_t t3 = t3bit + d3;
			uint8_t t3hi = static_cast<uint8_t>(t3 >> 8);
			uint8_t zCheck = *a0++; // E1:6626
			if (t3hi != zCheck)
			{
				continue;
			}
		}

		// Test 4 (E1:6628-6631): X sign-side
		// D7 = posX - $28; EOR.B desc, D7; BMI -> fail
		{
			uint16_t t4 = static_cast<uint16_t>(d2 - 0x0028);
			uint8_t t4lo = static_cast<uint8_t>(t4) ^ desc;
			if (t4lo & 0x80) // BMI = negative -> fail
				continue;
		}

		// Test 5 (E1:6632-6635): heading parity
		// D7 = D4 (compressed heading); ROR.B #1,D7; EOR.B desc,D7; BMI -> fail
		{
			uint8_t t5 = static_cast<uint8_t>(d4);
			t5 = static_cast<uint8_t>((t5 >> 1) | (t5 << 7)); // ROR.B #1
			t5 ^= desc;
			if (t5 & 0x80) // BMI -> fail
				continue;
		}

		// All 5 tests passed!

		// E1:6636-6644: locked door check
		// TST.B ($0631E4); BMI -> skip lock check (cobweb = lockpick)
		if ((g_workspace.objs.flagsTable[OBJ_COBWEB] & OBJ_FLAG_TAKEN) == 0)
		{
			// Check bit 3 of descriptor low nibble
			uint8_t lockIdx = desc & 0x0F;
			if (lockIdx & 0x08)
			{
				// $0631BC[lockIdx]: if positive -> locked
				// $0631BC = flagsTable base ($0631B4) + 8 = slot 8
				if (static_cast<int8_t>(
						g_workspace.objs.flagsTable[8 + lockIdx]) >= 0)
				{
					// E1:6676-6679: locked! Show message 0 and return no-match
					MessageDisplay(game, 0);
					return false;
				}
			}
		}

		// E1:6647-6668: compute new heading and $062448 palette mode
		// D7 = (desc < 0) ? posX : posZ
		uint16_t d7val;
		if (desc & 0x80)
			d7val = d2; // posX
		else
			d7val = d3; // posZ

		// E1:6656-6663: if D7 >= $100 -> D7 = 0; else D7 ^= $80
		uint8_t d7byte;
		if (d7val >= 0x0100)
			d7byte = 0x00;
		else
		{
			d7byte = static_cast<uint8_t>(d7val);
			d7byte ^= 0x80;
		}

		// E1:6665-6668: ROR.B #2,D4; EOR.B D4,D7; ORI.B #$7F,D7;
		// MOVE.B D7,($062448)
		{
			uint8_t d4b = static_cast<uint8_t>(d4);
			d4b = static_cast<uint8_t>((d4b >> 2) | (d4b << 6)); // ROR.B #2
			d7byte ^= d4b;
			d7byte |= 0x7F;
			cam.doorWipeDir = static_cast<int8_t>(d7byte);
		}

		// E1:6669-6672: compute room and buildingIndex outputs
		// D0 = desc; LSR.B #2; ANDI.B #$1C -> buildingIndex
		// EXG D0,D1 -> D0 = secondary (room), D1 = buildingIndex
		uint8_t buildIdx = (desc >> 2) & 0x1C;
		outRoom = static_cast<uint16_t>(secondary);
		outBuildingIndex = buildIdx;

		return true;
	}

	return false;
}

// Door wipe effect -- see DoorWipeHide/DoorWipeReveal in VblHandler.cpp
