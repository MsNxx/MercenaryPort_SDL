// World object rendering -- sub_043C14 (E1:4510) and related

#include "game/Objects.h"
#include "game/ObjectSlots.h"
#include "game/Workspace.h"
#include "renderer/FrameBuffer.h"
#include "renderer/Hud.h"
#include "renderer/LineDraw.h"
#include "renderer/LogMath.h"
#include "renderer/Projection.h"

#include "data/GameData.h"

#include <cstring>

// Geometry pointer table offset within E3_RAW
// $05E000 - $058200 = $5E00
static constexpr int GEOM_TABLE_OFFSET = 0x5E00;

// E3 load address for computing geometry file offsets
static constexpr int E3_LOAD_ADDR = 0x058200;
static constexpr int GEOM_TABLE_ADDR = 0x05E000;

// ObjectsInit -- copy E2 initial state into mutable ObjectState

void ObjectsInit(ObjectState &objs)
{
	std::memcpy(objs.posX, gen_e2::INIT_POS_X, sizeof(objs.posX));
	std::memcpy(objs.posY, gen_e2::INIT_POS_Y, sizeof(objs.posY));
	std::memcpy(objs.posZ, gen_e2::INIT_POS_Z, sizeof(objs.posZ));
	std::memset(objs.velX, 0, sizeof(objs.velX));
	std::memset(objs.velY, 0, sizeof(objs.velY));
	std::memset(objs.velZ, 0, sizeof(objs.velZ));
	std::memset(objs.motionTimer, 0, sizeof(objs.motionTimer));
	std::memcpy(objs.slotTable, gen_e2::SLOT_TABLE, sizeof(objs.slotTable));
	std::memcpy(objs.flagsTable, gen_e2::FLAGS_TABLE, sizeof(objs.flagsTable));
	std::memcpy(objs.typeTable, gen_e2::TYPE_TABLE, sizeof(objs.typeTable));
	std::memcpy(objs.rotSelector, gen_e2::ROTATION_SELECTOR,
				sizeof(objs.rotSelector));

	objs.activeCount = 0;
	objs.currentRoom = 0;
}

// ObjectsBuildActiveList -- sub_043C14 (E1:4510-4524)

void ObjectsBuildActiveList(ObjectState &objs)
{
	uint16_t count = 0;
	uint8_t room = objs.currentRoom;

	for (int i = 63; i >= 0; i--)
	{
		if (objs.slotTable[i] == room)
		{
			objs.activeList[count++] = static_cast<uint8_t>(i);
		}
	}
	objs.activeCount = count;
}

// CameraRelativeTransform -- sub_043F5C (E1:4943-4959)
// SWAP/EXT.W/SWAP compression on X and Z, full 32-bit Y

void CameraRelativeTransform(int32_t objX, int32_t objY, int32_t objZ,
							 const Camera &cam, int32_t &outDX, int32_t &outDY,
							 int32_t &outDZ)
{
	// X: SWAP/EXT.W/SWAP compression (E1:4944-4949)
	int32_t dx = objX - cam.posX;
	int8_t hx = static_cast<int8_t>((dx >> 16) & 0xFF);
	outDX = (static_cast<int32_t>(hx) << 16) | (dx & 0xFFFF);

	// Y: no compression (E1:4950-4952)
	outDY = objY - cam.posY;

	// Z: same compression as X (E1:4953-4958)
	int32_t dz = objZ - cam.posZ;
	int8_t hz = static_cast<int8_t>((dz >> 16) & 0xFF);
	outDZ = (static_cast<int32_t>(hz) << 16) | (dz & 0xFFFF);
}

// DistanceCull -- sub_043F8E (E1:4962-4979)
// Chebyshev distance: true if all components within threshold

bool WithinDistance(int32_t dx, int32_t dy, int32_t dz, uint32_t threshold)
{
	auto absVal = [](int32_t v) -> uint32_t
	{ return v < 0 ? static_cast<uint32_t>(-v) : static_cast<uint32_t>(v); };
	if (absVal(dx) > threshold)
		return false;
	if (absVal(dy) > threshold)
		return false;
	if (absVal(dz) > threshold)
		return false;
	return true;
}

// GetGeometry -- look up geometry pointer from E3 offset table at $05E000

static const uint8_t *GetGeometry(uint8_t slot)
{
	int tablePos = GEOM_TABLE_OFFSET + slot * 2;
	if (tablePos + 2 > gen_e3::E3_SIZE)
		return nullptr;

	// Big-endian word
	int16_t offset = static_cast<int16_t>((gen_e3::E3_RAW[tablePos] << 8) |
										  gen_e3::E3_RAW[tablePos + 1]);

	// Geometry address = $05E000 + signed offset
	int geomAddr = GEOM_TABLE_ADDR + offset;
	int filePos = geomAddr - E3_LOAD_ADDR;

	if (filePos < 0 || filePos >= gen_e3::E3_SIZE)
		return nullptr;

	return &gen_e3::E3_RAW[filePos];
}

// ProcessVertices -- vertex loop (E1:4730-4905)

static int ProcessVertices(const uint8_t *geom, int slot, int32_t camRelX,
						   int32_t camRelY, int32_t camRelZ, const Camera &cam,
						   const ObjectState &objs, ProjectedVertex *projVerts)
{
	// E1:4730-4733: vertex count
	int vtxCount = geom[0] + 1; // ADDQ.B #1
	const uint8_t *ptr = geom + 1;

	for (int i = 0; i < vtxCount && i < OBJ_MAX_VERTS; i++)
	{
		// E1:4737-4739: read 3 signed byte offsets
		int8_t bx = static_cast<int8_t>(ptr[0]);
		int8_t by = static_cast<int8_t>(ptr[1]);
		int8_t bz = static_cast<int8_t>(ptr[2]);
		ptr += 3;

		// E1:4741-4746: EXT.W + ASL.W #4 (sign-extend, scale by 16)
		int16_t dx = static_cast<int16_t>(bx) * 16;
		int16_t dy = static_cast<int16_t>(by) * 16;
		int16_t dz = static_cast<int16_t>(bz) * 16;

		// E1:4747-4768: yaw rotation for slots < 16
		if (slot < 16)
		{
			uint8_t rotByte = objs.rotSelector[slot];
			int16_t m0 = gen_e2::ROT_TABLE[rotByte];	  // cos, (A4)
			int16_t m1 = gen_e2::ROT_TABLE[rotByte + 16]; // sin, 32(A4)

			// E1:4756-4762: newZ = Z*sin - X*cos
			int32_t t1 =
				static_cast<int32_t>(dz) * m1 - static_cast<int32_t>(dx) * m0;
			int16_t newZ = static_cast<int16_t>((t1 >> 16) * 2);

			// E1:4764-4768: newX = Z*cos + X*sin
			int32_t t2 =
				static_cast<int32_t>(dz) * m0 + static_cast<int32_t>(dx) * m1;
			int16_t newX = static_cast<int16_t>((t2 >> 16) * 2);

			dx = newX;
			dz = newZ;
			// dy unchanged -- yaw rotation is around Y axis
		}

		// E1:4771-4772: EXT.L + ADD.L (add camera-relative offset)
		int32_t worldRelX = static_cast<int32_t>(dx) + camRelX;
		int32_t worldRelY = static_cast<int32_t>(dy) + camRelY;
		int32_t worldRelZ = static_cast<int32_t>(dz) + camRelZ;

		// E1:4773-4897: IntToLogFloat with exponent 23 for ALL axes
		uint32_t lfX = IntToLogFloat(worldRelX);
		uint32_t lfY = IntToLogFloat(worldRelY);
		uint32_t lfZ = IntToLogFloat(worldRelZ);

		// E1:4900: project via sub_046DA6
		projVerts[i] = ProjectVertexLogFloat(lfX, lfY, lfZ, cam);
	}

	return vtxCount;
}

// ProcessLineConnectivity -- line connectivity (E1:4906-4940)
// Format A (bit 7 set): byte pairs; Format B (bit 7 clear): nibble pairs

static void ProcessLineConnectivity(const uint8_t *lineData, int vtxCount,
									uint8_t *indexBuf, const Camera &cam,
									const ProjectedVertex *projVerts)
{
	uint8_t control = *lineData++;

	if (control & 0x80)
	{
		// Format A: two-byte vertex pairs (E1:4910-4923)
		// SUBQ.B #1,D1; BMI -- loop while bit 7 set
		uint8_t counter = control;
		do
		{
			// E1:4912-4917: read raw byte indices, LSL.B #2
			uint8_t rawA = *lineData++;
			uint8_t rawB = *lineData++;

			// LSL.B #2 then MOVEA.L from $060000 base means
			// the effective vertex index is the raw byte value
			// The *4 is the workspace stride, not the vertex index
			if (rawA < vtxCount && rawB < vtxCount)
			{
				DrawLineProjectedMode(indexBuf, projVerts[rawA],
									  projVerts[rawB], cam, LineMode::BCLR);
			}

			counter--;
		} while (counter & 0x80); // BMI -- bit 7 set
	}
	else
	{
		// Format B: packed nibble pairs (E1:4925-4940)
		// Counter starts non-negative, loops while BPL
		int8_t counter = static_cast<int8_t>(control);
		do
		{
			uint8_t packed = *lineData++;

			// E1:4929-4934: low nibble = vtxA, high nibble = vtxB
			uint8_t vtxA = packed & 0x0F;
			uint8_t vtxB = (packed >> 4) & 0x0F;

			if (vtxA < vtxCount && vtxB < vtxCount)
			{
				DrawLineProjectedMode(indexBuf, projVerts[vtxA],
									  projVerts[vtxB], cam, LineMode::BCLR);
			}

			counter--;
		} while (counter >= 0); // BPL
	}
}

// RenderSingleObject -- sub_043C78 (E1:4550-4940)

static void RenderSingleObject(uint8_t *indexBuf, const Camera &cam,
							   const ObjectState &objs, uint8_t slot)
{
	// E1:4554-4556: geometry pointer from $05E000 table
	const uint8_t *geom = GetGeometry(slot);
	if (!geom)
		return;

	// E1:4561: camera-relative transform (sub_043F5C)
	int32_t dx, dy, dz;
	CameraRelativeTransform(objs.posX[slot], objs.posY[slot], objs.posZ[slot],
							cam, dx, dy, dz);

	// E1:4562: visibility/type check (sub_043FAC, E1:4988-5057)
	// Checks keyCommand for TAKE_ITEM or BOARD_VEHICLE
	// Returns false if the object was consumed (entered/boarded)
	{
		Action keyCmd = g_workspace.keyCommand;

		if (keyCmd == Action::BOARD_VEHICLE)
		{
			// B key: vehicle boarding (E1:5033-5057)
			// Must be on foot (flightState < 0), slot < 8,
			// within $0200 (512) units
			if (static_cast<int16_t>(cam.flightState) < 0 && slot < 8 &&
				WithinDistance(dx, dy, dz, 0x0200))
			{
				// E1:5045-5046: set camera to object position
				g_workspace.cam.posX = objs.posX[slot];
				g_workspace.cam.posZ = objs.posZ[slot];

				// E1:5047-5050: heading from rotation byte x 16
				// MOVEQ #0,D1; MOVE.B 11812(A5),D1; ASL.W #4,D1
				// MOVE.W D1,($06234C).L -- unconditional
				uint8_t rot = objs.rotSelector[slot];
				g_workspace.cam.heading = static_cast<uint16_t>(rot) << 4;

				// E1:5043-5044: dispatch based on flagsTable bits 2-3
				// (sub_04408E, E1:5060-5074)
				{
					uint8_t dispatchFlags = objs.flagsTable[slot] & 0x0C;
					if (dispatchFlags != 0)
					{
						if ((dispatchFlags >> 3) == 0)
						{
							// E1:5069-5071: bit 2 set, bit 3 clear ->
							// sub_04418C (event script load)
							g_workspace.pendingEventSlot =
								static_cast<int16_t>(slot);
						}
						else
						{
							// E1:5065-5067: bit 3 set ->
							// sub_0441C0 (message display) with D1=slot
							g_workspace.cam.pendingMsg = slot;
							g_workspace.cam.pendingMsgFlag = 0x0001;
						}
					}
				}

				// E1:5045-5046: set camera position (already done above)

				// E1:5051: flightState = slot index
				g_workspace.cam.flightState = slot;

				// E1:5053-5056: rebuild matrix, reset projection
				CameraBuildMatrix(g_workspace.cam);
				g_workspace.cam.thrustAccum = LOG_FLOAT_ZERO;
				g_workspace.cam.vertVelocity = LOG_FLOAT_ZERO;

				// E1:5057 -> loc_04400E: consume action, hide object
				g_workspace.keyCommand = Action::NONE;
				g_workspace.objs.slotTable[slot] = 0xFF;
				ObjectsBuildActiveList(g_workspace.objs);
				HudFactionLED();
				g_workspace.pendingPing = true;
				return; // object consumed, skip rendering
			}
		}

		if (keyCmd == Action::TAKE_ITEM)
		{
			// T key: take item (E1:4990-5022)
			// Must be within 256 units
			if (!WithinDistance(dx, dy, dz, 0x0100))
				goto skip_take;

			// E1:4995-4996: skip checks if player has kitchen sink
			if (static_cast<int8_t>(
					g_workspace.objs.flagsTable[OBJ_KITCHEN_SINK]) >= 0)
			{
				// E1:4997-4999: BTST #4 -- not collectible, can't take
				uint8_t flags = objs.flagsTable[slot];
				if (flags & OBJ_FLAG_NOT_TAKEABLE)
					goto skip_take;

				// E1:5000-5001: skip heavy check if player has antigrav
				if (static_cast<int8_t>(
						g_workspace.objs.flagsTable[OBJ_ANTIGRAV]) >= 0)
				{
					// E1:5002-5003: BTST #5 -- heavy -> "TOO HEAVY"
					if (flags & OBJ_FLAG_HEAVY)
					{
						// loc_044024 (E1:5024-5026): message index 2
						g_workspace.cam.pendingMsg = 2;
						g_workspace.cam.pendingMsgFlag = 1;
						goto skip_take;
					}
				}
			}

			// loc_043FE4 (E1:5005-5015): push onto interior stack

			// E1:5006-5009: stack depth check (max 11)
			if (g_workspace.inventoryStackDepth >=
				Workspace::INVENTORY_STACK_MAX)
				goto skip_take;

			// E1:5010-5012: push slot onto stack
			g_workspace.inventoryStack[g_workspace.inventoryStackDepth] = slot;
			g_workspace.inventoryStackDepth++;

			// E1:5013: BSET #7 -- set "currently taken" flag
			g_workspace.objs.flagsTable[slot] |= OBJ_FLAG_TAKEN;

			// E1:5014-5015: sub_04408E -- dispatch based on bits 2-3
			{
				uint8_t d1 = objs.flagsTable[slot] & 0x0C;
				if (d1 != 0)
				{
					if ((d1 >> 3) == 0)
					{
						// Bit 2 set, bit 3 clear -> event script (sub_04418C)
						g_workspace.pendingEventSlot =
							static_cast<int16_t>(slot);
					}
					else
					{
						// Bit 3 set -> message display (sub_0441C0)
						g_workspace.cam.pendingMsg =
							static_cast<uint16_t>(slot);
						g_workspace.cam.pendingMsgFlag = 1;
					}
				}
			}

			// loc_04400E (E1:5017-5022): consume action, hide object, rebuild
			g_workspace.keyCommand = Action::NONE;
			g_workspace.objs.slotTable[slot] = 0xFF;
			ObjectsBuildActiveList(g_workspace.objs);
			HudFactionLED();
			g_workspace.pendingPing = true;
			return; // object consumed, skip rendering

		skip_take:;
		}
	}

	// E1:4571-4573: distance cull with threshold $10000
	if (WithinDistance(dx, dy, dz, 0x10000))
	{
		// Nearby: full vertex loop + line connectivity
		ProjectedVertex projVerts[OBJ_MAX_VERTS];
		int vtxCount =
			ProcessVertices(geom, slot, dx, dy, dz, cam, objs, projVerts);

		// Line connectivity data follows vertex data:
		// skip past vertex count byte + vtxCount * 3 bytes
		const uint8_t *lineData = geom + 1 + vtxCount * 3;
		ProcessLineConnectivity(lineData, vtxCount, indexBuf, cam, projVerts);
		return;
	}

	// E1:4574-4580: far away -- secondary altitude-based cull
	// BPL at E1:4575: if camera-relative Y >= 0 (object at same
	// level or below), skip the altitude check and always render
	// as a dot.  If Y < 0 (object above camera), apply the
	// altitude-based distance threshold
	if (dy < 0)
	{
		// E1:4576-4580: altitude-based LOD threshold
		// MOVEQ #0,D1; MOVE.B ($062426),D1; SWAP D1 -> threshold
		// $062426 = cachedD7 (altitude-to-tile-window byte)
		uint32_t altThreshold =
			static_cast<uint32_t>(g_workspace.tileDetail.cachedD7) << 16;
		if (!WithinDistance(dx, dy, dz, altThreshold))
			return; // too far above, cull entirely
	}

	// E1:4583-4707: center-point projection + dot
	// D3=A1 (relX), D4=A2 (relY), D5=A3 (relZ) -- no negation
	// X, Z use exponent 23 (IntToLogFloat)
	// Y uses exponent 31 (IntToLogFloat31)
	uint32_t lfX = IntToLogFloat(dx);
	uint32_t lfY = IntToLogFloat31(dy);
	uint32_t lfZ = IntToLogFloat(dz);

	// Project center point
	ProjectedVertex center = ProjectVertexLogFloat(lfX, lfY, lfZ, cam);

	// E1:4712-4713: if behind camera, skip
	if (center.visFlags != 0)
		return;

	// E1:4714-4724: render as dot
	// The original uses ORI.L #$01AD0002 via two lookup tables
	// For our indexed framebuffer, write a BCLR pixel at the
	// projected screen coordinates (palette index 8 = white)
	int sx = center.screenX;
	int sy = center.screenY;
	if (sx >= 0 && sx < FB_WIDTH && sy >= 0 && sy < VIEWPORT_H)
	{
		indexBuf[sy * FB_WIDTH + sx] &= ~3; // BCLR: pixel &= ~3
	}
}

// ObjectsDraw -- sub_043C48 (E1:4529-4547)

void ObjectsDraw(uint8_t *indexBuf, const Camera &cam, const ObjectState &objs)
{
	// E1:4530: MOVE.W #$0020,($0624C8) -- BCLR mode
	// (Handled implicitly: all lines drawn with LineMode::BCLR)

	int count = objs.activeCount;
	if (count == 0)
		return;

	// E1:4534-4544: iterate backward through the active list
	for (int i = count - 1; i >= 0; i--)
	{
		uint8_t slot = objs.activeList[i];
		RenderSingleObject(indexBuf, cam, objs, slot);
	}
}

// ObjectVehicleExit -- loc_0440AC (E1:5076-5096)
// Places vehicle at camera position; slot 9 (Dart) stays hidden

void ObjectVehicleExit(Camera &cam, ObjectState &objs)
{
	// E1:5077-5082: guard checks
	if (cam.elevatorActive != 0)
		return;
	int16_t vehicleSlot = static_cast<int16_t>(cam.flightState);
	if (vehicleSlot < 0)
		return; // already on foot ($8000)
	if (cam.grounded == 0)
		return; // no control

	// E1:5083: sub_044176 -- reset thrust/velocity to LOG_FLOAT_ZERO
	cam.thrustAccum = LOG_FLOAT_ZERO;
	cam.vertVelocity = LOG_FLOAT_ZERO;

	// E1:5084-5086: clear roll, set pitch, set on-foot
	cam.roll = 0x0000;
	cam.pitch = 0x0200;
	cam.flightState = 0x8000; // on foot

	// E1:5089-5091: slot 9 (Dart) is never placed
	uint8_t slot = static_cast<uint8_t>(vehicleSlot);
	if (slot == 9)
		return;

	// E1:5092-5096: slots < 9 save rotation from heading
	if (slot < 9)
	{
		uint8_t rot = static_cast<uint8_t>((cam.heading >> 4) & 0x3F);
		objs.rotSelector[slot] = rot;
	}

	// loc_044140 (E1:5119-5130): place object at camera position

	// E1:5121: set room byte = current room ($062453)
	// $062453 is the low byte of $062452 (current room word)
	objs.slotTable[slot] = objs.currentRoom;

	// E1:5122-5124: compute workspace address = $060000 + slot*4
	// (We index by slot directly.)

	// E1:5125: world X = camera X
	objs.posX[slot] = cam.posX;

	// E1:5126-5127: world Y = camera Y with low byte cleared
	objs.posY[slot] = cam.posY & ~0xFF;

	// E1:5129: world Z = camera Z
	objs.posZ[slot] = cam.posZ;

	// E1:5130: BRA sub_043C14 -- rebuild active list
	ObjectsBuildActiveList(objs);
	HudFactionLED();
}
