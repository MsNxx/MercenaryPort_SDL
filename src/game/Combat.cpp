#include "game/Combat.h"

#include "game/Game.h"

#include "audio/Audio.h"
#include "data/GameData.h"
#include "game/Camera.h"
#include "game/ObjectSlots.h"
#include "game/Objects.h"
#include "game/ScriptVm.h"
#include "game/TileDetail.h"
#include "game/Workspace.h"
#include "renderer/Hud.h"
#include "renderer/LogMath.h"
#include "renderer/Palette.h"

// sub_041A74 (E1:1405-1421): relative position between two slots
// Computes (A0_slot - A4_slot) with SWAP/EXT.W compression on X/Z
// Returns delta in outDX/outDY/outDZ

static void SlotRelativePosition(const ObjectState &objs, int slotA, int slotB,
								 int32_t &outDX, int32_t &outDY, int32_t &outDZ)
{
	// E1:1406-1411: X delta with SWAP/EXT.W/SWAP compression
	int32_t dx = objs.posX[slotA] - objs.posX[slotB];
	int8_t hx = static_cast<int8_t>((dx >> 16) & 0xFF);
	outDX = (static_cast<int32_t>(hx) << 16) | (dx & 0xFFFF);

	// E1:1412-1414: Y delta (no compression)
	outDY = objs.posY[slotA] - objs.posY[slotB];

	// E1:1415-1420: Z delta with SWAP/EXT.W/SWAP compression
	int32_t dz = objs.posZ[slotA] - objs.posZ[slotB];
	int8_t hz = static_cast<int8_t>((dz >> 16) & 0xFF);
	outDZ = (static_cast<int32_t>(hz) << 16) | (dz & 0xFFFF);
}

// sub_04151E (E1:1093-1166): bullet hit check on Dominion Dart A
// Returns true if player bullet hit the NPC dart

static bool BulletHitDartA(Game &game, ObjectState &objs)
{
	// E1:1093: TST.B ($063134) -- is dart A in current room?
	if (objs.slotTable[OBJ_DOMINION_DART_A] != 0x00)
		return false;

	int32_t dx, dy, dz;
	SlotRelativePosition(objs, OBJ_PLAYER_BULLET, OBJ_DOMINION_DART_A, dx, dy,
						 dz);

	// E1:1099: distance threshold $0400
	if (!WithinDistance(dx, dy, dz, 0x0400))
		return false;

	// Hit! Envelope decay sound on channel B (E1:1101-1131)
	game.intro.mixerShadow &= ~0x01; // BCLR #0 -- enable tone A
	game.intro.mixerShadow |= 0x02;	 // BSET #1 -- disable tone B
	game.intro.mixerShadow &= ~0x10; // BCLR #4 -- enable noise B

	AudioWriteReg(game.audio, YM_TONE_B_FINE, 0x00);
	AudioWriteReg(game.audio, YM_TONE_B_COARSE, 0x00);
	AudioWriteReg(game.audio, YM_NOISE_PERIOD, 0x1F);
	AudioWriteReg(game.audio, YM_MIXER, game.intro.mixerShadow);
	AudioWriteReg(game.audio, YM_AMP_B, 0x10);
	AudioWriteReg(game.audio, YM_ENV_FINE, 0x00);
	AudioWriteReg(game.audio, YM_ENV_COARSE, 0x0F);
	AudioWriteReg(game.audio, YM_ENV_SHAPE, 0x00);

	// E1:1134-1144: deactivate dart A and bullet
	objs.motionTimer[OBJ_DOMINION_DART_A] = 0;
	objs.slotTable[OBJ_DOMINION_DART_A] = 0xFF;
	objs.slotTable[OBJ_PLAYER_BULLET] = 0xFF;
	objs.motionTimer[OBJ_PLAYER_BULLET] = 0;
	ObjectsBuildActiveList(objs);
	HudFactionLED();
	return true;
}

// sub_0415F2 (E1:1132-1175): bullet hit check on enemy ship
// Returns true if player bullet hit the enemy ship (NPC hexapod)

static bool BulletHitEnemyShip(Game &game, ObjectState &objs)
{
	// E1:1132: TST.B ($063143) -- is enemy ship in current room?
	if (objs.slotTable[OBJ_ENEMY_SHIP] != 0x00)
		return false;

	int32_t dx, dy, dz;
	SlotRelativePosition(objs, OBJ_PLAYER_BULLET, OBJ_ENEMY_SHIP, dx, dy, dz);

	if (!WithinDistance(dx, dy, dz, 0x0400))
		return false;

	// Hit! Noise + envelope decay (E1:1140-1170)
	game.intro.mixerShadow |= 0x02;	 // BSET #1 -- disable tone B
	game.intro.mixerShadow &= ~0x10; // BCLR #4 -- enable noise B

	AudioWriteReg(game.audio, YM_NOISE_PERIOD, 0x1F);
	AudioWriteReg(game.audio, YM_MIXER, game.intro.mixerShadow);
	AudioWriteReg(game.audio, YM_AMP_B, 0x10);
	AudioWriteReg(game.audio, YM_ENV_FINE, 0x00);
	AudioWriteReg(game.audio, YM_ENV_COARSE, 0x0F);
	AudioWriteReg(game.audio, YM_ENV_SHAPE, 0x00);

	// E1:1171-1175: clear attackFlag, restore palette 3, deactivate
	game.scriptVM.attackFlag = 0;
	// E1:1172: MOVE.W #$0110,($FF8248) -- restore palette 3 to default
	game.gameplayLut[3] = 0xFF000000u | (Expand3to8(1) << 16) |
						  (Expand3to8(1) << 8) | Expand3to8(0);
	objs.motionTimer[OBJ_ENEMY_SHIP] = 0;
	objs.slotTable[OBJ_ENEMY_SHIP] = 0xFF;
	objs.slotTable[OBJ_PLAYER_BULLET] = 0xFF;
	objs.motionTimer[OBJ_PLAYER_BULLET] = 0;
	ObjectsBuildActiveList(objs);
	HudFactionLED();
	return true;
}

// sub_041258 (E1:870-1002): object motion tick
// Iterates slots 15->8, applies velocity, handles timer expiry and
// bullet-building collision for slot 8

static void ObjectMotionTick(Game &game)
{
	ObjectState &objs = g_workspace.objs;
	Camera &cam = g_workspace.cam;

	// E1:870: A0 starts at $06003C (slot 15), iterates down to $060020 (slot
	// 8)
	for (int slot = OBJ_ENEMY_SHIP; slot >= OBJ_PLAYER_BULLET; slot--)
	{
		uint16_t timer = objs.motionTimer[slot];

		// E1:874: skip if timer == 0 (stationary)
		if (timer == 0)
			continue;
		// E1:876: skip if timer == $7FFF (permanent)
		if (timer == 0x7FFF)
			continue;

		// E1:878: decrement timer
		objs.motionTimer[slot] = --timer;

		// E1:879: if timer just hit zero, deactivate
		if (timer == 0)
		{
			// loc_0412A8 (E1:908-916): mark slot inactive
			objs.slotTable[slot] = 0xFF;
			ObjectsBuildActiveList(objs);
			HudFactionLED();
			continue;
		}

		// E1:880-888: apply velocity to position
		objs.posX[slot] += objs.velX[slot];
		objs.posY[slot] += objs.velY[slot];

		// E1:884-885: BPL loc_04128A -- if result >= 0 (non-negative), skip
		// clamp. Clamp only fires when posY goes negative (object wraps above
		// max altitude).  Sets to $FFFFFFFF (-1, just below zero)
		if (objs.posY[slot] < 0)
			objs.posY[slot] = static_cast<int32_t>(0xFFFFFFFF);

		objs.posZ[slot] += objs.velZ[slot];

		// E1:893-898: if this is the player bullet, do collision checks
		if (slot != OBJ_PLAYER_BULLET)
			continue;

		// ===== Player bullet collision (loc_0412C6, E1:913) =====

		int32_t bulletX = objs.posX[OBJ_PLAYER_BULLET];
		int32_t bulletY = objs.posY[OBJ_PLAYER_BULLET];
		int32_t bulletZ = objs.posZ[OBJ_PLAYER_BULLET];

		// E1:913-914: check if bullet is above ground (high word < 0)
		int16_t yHi =
			static_cast<int16_t>(static_cast<uint32_t>(bulletY) >> 16);
		if (yHi >= 0)
		{
			// E1:916: if high word != 0, bullet is underground -> miss
			if (yHi != 0)
				goto bullet_miss;
			// E1:918: check low word against $0320
			uint16_t yLo = static_cast<uint16_t>(bulletY & 0xFFFF);
			if (yLo >= 0x0320)
				goto bullet_miss;
		}

		// loc_0412DC (E1:927): building hit detection
		{
			// E1:928-936: validate X within map bounds
			if (static_cast<uint32_t>(bulletX) >= 0x00100000u)
				goto bullet_out_of_range;
			uint16_t xLo = static_cast<uint16_t>(bulletX & 0xFFFF);
			if (xLo < 0x7000 || xLo >= 0x9000)
				goto bullet_out_of_range;

			// E1:938-946: validate Z within map bounds
			if (static_cast<uint32_t>(bulletZ) >= 0x00100000u)
				goto bullet_out_of_range;
			uint16_t zLo = static_cast<uint16_t>(bulletZ & 0xFFFF);
			if (zLo < 0x7000 || zLo >= 0x9000)
				goto bullet_out_of_range;

			// E1:948-957: compute tile index
			// SWAP D3 (get high word of Z), LSL.W #4, OR with low nibble of X
			uint16_t zNibble = static_cast<uint16_t>(
				(static_cast<uint32_t>(bulletZ) >> 16) & 0xFF);
			uint16_t xNibble = static_cast<uint16_t>(
				(static_cast<uint32_t>(bulletX) >> 16) & 0x0F);
			uint16_t tileIdx = static_cast<uint16_t>((zNibble << 4) | xNibble);

			// === Building hit! (E1:943-979) ===
			// The original has no tile-byte guard here -- any tile within
			// bounds triggers destruction.  E1:947 saves the tile byte,
			// E1:948 sets bit 7 (destroyed flag)

			// E1:943-946: store tile index, compute tile pointer
			cam.collapseTileIndex = tileIdx;

			// E1:947: MOVE.B (A5),($0624A6) -- save tile byte
			cam.savedTileByte = objs.typeTable[tileIdx];

			// E1:965: set bit 7 of tile byte (mark destroyed)
			objs.typeTable[tileIdx] |= 0x80;

			// E1:967: set collapse countdown = 40 frames
			cam.collapseCountdown = 0x0028;

			// E1:968-970: deactivate bullet
			objs.slotTable[OBJ_PLAYER_BULLET] = 0xFF;
			objs.motionTimer[OBJ_PLAYER_BULLET] = 0;
			ObjectsBuildActiveList(objs);
			HudFactionLED();

			// E1:972-1002: building-hit explosion sound on channel B
			// Tone B = $0FFF (very low), noise, envelope decay
			game.intro.mixerShadow &= ~0x02; // BCLR #1 -- enable tone B
			AudioWriteReg(game.audio, YM_TONE_B_FINE, 0xFF);
			AudioWriteReg(game.audio, YM_TONE_B_COARSE, 0x0F);

			game.intro.mixerShadow &= ~0x10; // BCLR #4 -- enable noise B
			AudioWriteReg(game.audio, YM_NOISE_PERIOD, 0x1F);
			AudioWriteReg(game.audio, YM_MIXER, game.intro.mixerShadow);
			AudioWriteReg(game.audio, YM_AMP_B, 0x10);
			AudioWriteReg(game.audio, YM_ENV_FINE, 0xFF);
			AudioWriteReg(game.audio, YM_ENV_COARSE, 0x7F);
			AudioWriteReg(game.audio, YM_ENV_SHAPE, 0x00);

			// E1:979: BRA loc_041456 -- enters the collapse timer handler
			// mid-function.  First collapse animation frame runs immediately
			// via DestructionTimerTick later in the same GameTick call
			break; // bullet consumed -- exit the slot loop
		}

	bullet_out_of_range:
		// loc_041412 (E1:1003-1017): bullet is out of tile grid
		// If bullet is above ground (yHi < 0), deactivate it
		if (yHi < 0)
		{
			objs.slotTable[OBJ_PLAYER_BULLET] = 0xFF;
			objs.motionTimer[OBJ_PLAYER_BULLET] = 0;
			ObjectsBuildActiveList(objs);
			HudFactionLED();
			continue;
		}
		// Fall through to dynamic object check

	bullet_miss:
		// loc_041428 (E1:1018-1028): try hitting dynamic objects
		if (BulletHitEnemyShip(game, objs))
		{
			MessageDisplay(game, 49); // "ENEMY SHIP DESTROYED"
			break;
		}
		if (BulletHitDartA(game, objs))
		{
			// E1:997: MOVEQ #27,D1; BRA sub_04418C -- event 27 (New Ship)
			g_workspace.pendingEventSlot = 27;
			break;
		}
		// E1:1028: no hit -- continue loop (bullet stays alive)
	}
}

// sub_041442 (E1:1029-1072): building collapse timer

static void DestructionTimerTick(Game &game)
{
	Camera &cam = g_workspace.cam;
	TileDetailState &td = g_workspace.tileDetail;
	ObjectState &objs = g_workspace.objs;

top:
	// E1:1004-1006: TST.W ($062496); BEQ return
	if (cam.collapseCountdown == 0)
		return;

	// E1:1007: SUBQ.W #1
	cam.collapseCountdown--;

	// E1:1008: MOVEA.L ($06249A),A5 -- tile property pointer

	// E1:1011-1013: CMP.B ($062499),($062421) -- check tile match
	// Only animate when the player is on the same tile as the collapse
	if (static_cast<uint8_t>(cam.collapseTileIndex) !=
		static_cast<uint8_t>(td.currentTileIndex))
		return;

	// E1:1014: MOVE.B (A5),($062492) -- copy tile byte to tileProperty
	td.tileProperty =
		static_cast<int8_t>(objs.typeTable[cam.collapseTileIndex]);

	// E1:1015-1016: TST.B ($0631CC); BMI loc_0414EC -- anti-time-bomb check
	if (static_cast<int8_t>(objs.flagsTable[OBJ_ANTI_TIME_BOMB]) < 0)
	{
		// Repair path (loc_0414EC, E1:1074-1090)

		// E1:1075-1076: load savedTileByte, copy to D1
		uint8_t d0 = cam.savedTileByte;
		uint8_t d1 = d0;

		// E1:1077: ANDI.W #$007F -- clear bit 7 (destruction flag)
		d0 &= 0x7F;

		// E1:1078: MOVE.B D0,(A5) -- write restored byte to tile table
		objs.typeTable[cam.collapseTileIndex] = d0;

		// E1:1079-1080: TST.B D1; BPL loc_041478 -- if original byte
		// was not destroyed (bit 7 clear), jump to frame pacing
		if (static_cast<int8_t>(d1) >= 0)
			goto frame_pacing;

		// E1:1081-1082: MOVE.W ($062498),D3; BSR sub_044742 -- reload tile
		TileDetailReload(td, cam, cam.collapseTileIndex);

		// E1:1083-1084: MOVE.W ($062496),D5; BEQ return
		{
			uint16_t d5 = cam.collapseCountdown;
			if (d5 == 0)
				return;

			// E1:1087-1089: loop BSR sub_0448FC; SUBQ #1,D5; BNE loop
			while (d5 > 0)
			{
				VertexGravityCollapse(td);
				d5--;
			}
		}
		return;
	}

	// E1:1017: BSR sub_0448FC -- vertex gravity collapse
	// E1:1018: BNE loc_041484 -- if any vertex still active, return
	if (VertexGravityCollapse(td))
		return;

frame_pacing:
	// loc_041478 (E1:1020-1024): frame throttle
	// Load countdown, if zero -> collapse complete
	// LSR.W #1 -- if carry set (countdown was odd), loop back to top
	// If carry clear (countdown was even), return
	if (cam.collapseCountdown != 0)
	{
		if (cam.collapseCountdown & 1)
			goto top; // E1:1024: BCS sub_041442 -- loop back
		return;		  // E1:1025: fall through to RTS
	}

	// === Collapse complete (loc_041486, E1:1029-1072) ===

	// E1:1030-1032: XOR saved vs current tile byte
	uint8_t current = objs.typeTable[cam.collapseTileIndex];
	uint8_t changed = current ^ cam.savedTileByte;

	// E1:1033: BPL -- if bit 7 didn't change, return
	if ((changed & 0x80) == 0)
		return;
	// E1:1034: BTST #5 -- if bit 5 set, return
	if (current & 0x20)
		return;

	// E1:1036-1044: LSL.B #1,D0 -- shift left, carry = original bit 7
	// BPL -> Z-axis (original bit 6 was 0), else X-axis (bit 6 was 1)
	// BCS -> increment (original bit 7 was 1), else decrement
	bool useXAxis = (current & 0x40) != 0;	// original bit 6
	bool increment = (current & 0x80) != 0; // original bit 7 (carry after LSL)

	if (useXAxis)
	{
		if (increment)
			cam.patrolCounterB++;
		else
			cam.patrolCounterB--;
	}
	else
	{
		if (increment)
			cam.patrolCounterA++;
		else
			cam.patrolCounterA--;
	}

	// E1:1055-1060: check patrol imbalance
	int msgIdx = 10; // default
	int16_t diff = cam.patrolCounterB - cam.patrolCounterA;
	if (diff < 0)
	{
		diff = -diff;
		msgIdx = 26; // alternate direction
	}

	// E1:1063-1064: CMPI.W #$69,D0; BCC loc_0414E8 -- large imbalance
	// goes straight to event script dispatch with D1 = 10 or 26
	if (diff < 105)
	{
		// E1:1065-1069: small imbalance -- extract building type from tile
		// D1 = (tileByte & $1F) | $20; clear bits 0-4 of tile byte
		uint8_t bldType = objs.typeTable[cam.collapseTileIndex] & 0x1F;
		if (bldType == 0)
			return; // E1:1067: BEQ loc_041484
		msgIdx = bldType | 0x20;
		objs.typeTable[cam.collapseTileIndex] &= 0xE0;
	}

	// loc_0414E8 (E1:1072): BRA sub_04418C -- event script dispatch
	ScriptVMLoadEvent(game.scriptVM, msgIdx);
}

// sub_0416D6 (E1:1176-1275): player projectile launch

static void PlayerProjectileLaunch(Game &game)
{
	Camera &cam = g_workspace.cam;
	ObjectState &objs = g_workspace.objs;

	// E1:1177: BTST #7,$0623B4 -- weapon active?
	if ((cam.inputFlags & 0x80) == 0)
		return;

	// E1:1180: TST.W $0623FE; BMI -- outdoors/on foot?
	if (static_cast<int16_t>(cam.flightState) < 0)
		return;

	// E1:1182: TST.W $06244C -- elevator transition engine running?
	if (cam.elevatorActive != 0)
		return;

	// E1:1184: TST.W $062496 -- collapse in progress?
	if (cam.collapseCountdown != 0)
		return;

	// E1:1186: player bullet already in flight?
	if (objs.motionTimer[OBJ_PLAYER_BULLET] != 0)
		return;

	// === All preconditions met ===

	// E1:1189-1207: fire sound -- noise + envelope on channel B
	game.intro.mixerShadow |= 0x02;	 // BSET #1 -- disable tone B
	game.intro.mixerShadow &= ~0x10; // BCLR #4 -- enable noise B

	AudioWriteReg(game.audio, YM_NOISE_PERIOD, 0x1F);
	AudioWriteReg(game.audio, YM_MIXER, game.intro.mixerShadow);
	AudioWriteReg(game.audio, YM_AMP_B, 0x10);
	AudioWriteReg(game.audio, YM_ENV_FINE, 0x00);
	AudioWriteReg(game.audio, YM_ENV_COARSE, 0x0F);
	AudioWriteReg(game.audio, YM_ENV_SHAPE, 0x00);

	// E1:1208: set bullet timer = $0020 (32 frames)
	objs.motionTimer[OBJ_PLAYER_BULLET] = 0x0020;

	// E1:1209: assign bullet to current room
	objs.slotTable[OBJ_PLAYER_BULLET] = objs.currentRoom;

	// E1:1210-1265: compute velocity from camera rotation matrix
	// Each component: LogFloatToInt(matrix_element + $000A0000)
	// $000A0000 is a log-domain scale factor (multiply by 2^10)

	// E1:1211: velocity X from M00 ($062372 = matrix[0])
	objs.velX[OBJ_PLAYER_BULLET] = LogFloatToInt(cam.matrix[0] + 0x000A0000);

	// E1:1232: velocity Y from sinPitch ($06235E)
	objs.velY[OBJ_PLAYER_BULLET] = LogFloatToInt(cam.sinPitch + 0x000A0000);

	// E1:1253: velocity Z from M02 ($06237A = matrix[2])
	objs.velZ[OBJ_PLAYER_BULLET] = LogFloatToInt(cam.matrix[2] + 0x000A0000);

	// E1:1266-1270: copy player position to bullet
	objs.posX[OBJ_PLAYER_BULLET] = cam.posX;
	objs.posY[OBJ_PLAYER_BULLET] = cam.posY;
	objs.posZ[OBJ_PLAYER_BULLET] = cam.posZ;

	// E1:1275: re-scan to make bullet visible
	ObjectsBuildActiveList(objs);
	HudFactionLED();
}

// sub_041832 (E1:1276-1372): enemy bullet tracking
// Consumes attackFlag: launches enemy bullets from enemy ship toward the
// player, tracks enemy bullet, and handles player hit

static void MissileTrackingTick(Game &game)
{
	ScriptVM &vm = game.scriptVM;
	Camera &cam = g_workspace.cam;
	ObjectState &objs = g_workspace.objs;

	// E1:1277: TST.W ($070104)
	if (vm.attackFlag == 0)
		return;

	// E1:1279: TST.W ($06244C) -- elevator active?
	if (cam.elevatorActive != 0)
		return;

	// E1:1282: check enemy ship motion timer -- negative = special state
	int16_t timer15 = static_cast<int16_t>(objs.motionTimer[OBJ_ENEMY_SHIP]);
	if (timer15 < 0)
		return;

	// E1:1284: camera-relative transform on enemy ship
	int32_t dx, dy, dz;
	CameraRelativeTransform(objs.posX[OBJ_ENEMY_SHIP],
							objs.posY[OBJ_ENEMY_SHIP],
							objs.posZ[OBJ_ENEMY_SHIP], cam, dx, dy, dz);

	// E1:1286: distance check ($3000 = tracking range)
	if (!WithinDistance(dx, dy, dz, 0x3000))
	{
		// === Out of range (loc_041A1A, E1:1365-1372) ===
		// Move enemy ship toward player: shift 6, timer $805A

		// sub_041A5A: scale and negate relative vector (shift 6)
		int32_t vx = -(dx >> 6);
		int32_t vy = -(dy >> 6);
		int32_t vz = -(dz >> 6);

		objs.velX[OBJ_ENEMY_SHIP] = vx;
		objs.velY[OBJ_ENEMY_SHIP] = vy;
		objs.velZ[OBJ_ENEMY_SHIP] = vz;

		// E1:1369: timer $805A (bit 15 = long-range mode, 90 frames)
		objs.motionTimer[OBJ_ENEMY_SHIP] = 0x805A;

		// E1:1371: keep enemy ship active
		objs.slotTable[OBJ_ENEMY_SHIP] = 0x00;

		ObjectsBuildActiveList(objs);
		HudFactionLED();
		return;
	}

	// === Close range (within $3000) ===

	// E1:1289: check if enemy bullet is already in flight
	if (objs.motionTimer[OBJ_ENEMY_BULLET] != 0)
	{
		// Enemy bullet in flight: check if it hit the player

		int32_t dx14, dy14, dz14;
		CameraRelativeTransform(
			objs.posX[OBJ_ENEMY_BULLET], objs.posY[OBJ_ENEMY_BULLET],
			objs.posZ[OBJ_ENEMY_BULLET], cam, dx14, dy14, dz14);

		// E1:1322: hit distance $0400
		if (!WithinDistance(dx14, dy14, dz14, 0x0400))
			return; // still in flight, no hit

		// === Enemy bullet hit the player! (E1:1325-1364) ===

		// E1:1325-1350: explosion sound -- tone B + noise + envelope
		game.intro.mixerShadow &= ~0x02; // BCLR #1 -- enable tone B
		AudioWriteReg(game.audio, YM_TONE_B_FINE, 0x0C);
		AudioWriteReg(game.audio, YM_TONE_B_COARSE, 0x00);

		game.intro.mixerShadow &= ~0x10; // BCLR #4 -- enable noise B
		AudioWriteReg(game.audio, YM_NOISE_PERIOD, 0x05);
		AudioWriteReg(game.audio, YM_MIXER, game.intro.mixerShadow);
		AudioWriteReg(game.audio, YM_AMP_B, 0x10);
		AudioWriteReg(game.audio, YM_ENV_FINE, 0x00);
		AudioWriteReg(game.audio, YM_ENV_COARSE, 0x40);
		AudioWriteReg(game.audio, YM_ENV_SHAPE, 0x00);

		// E1:1351: BSR sub_041AA0 -- trigger damage palette flash
		game.damageFlashTimer =
			43; // ~0.87s at 50Hz ($FFFF iterations x ~106 cycles / 8MHz)

		// E1:1354: if in vehicle/building, display hit message
		if (static_cast<int16_t>(cam.flightState) >= 0)
			MessageDisplay(game, 48); // "SHIP DESTROYED"

		// E1:1358: clear attackFlag, restore palette 3
		vm.attackFlag = 0;
		game.gameplayLut[3] = 0xFF000000u | (Expand3to8(1) << 16) |
							  (Expand3to8(1) << 8) | Expand3to8(0);

		// E1:1360: set flightState = 9
		cam.flightState = 9;

		// E1:1361-1362: deactivate enemy bullet and enemy ship
		objs.slotTable[OBJ_ENEMY_SHIP] = 0xFF;
		objs.slotTable[OBJ_ENEMY_BULLET] = 0xFF;
		objs.motionTimer[OBJ_ENEMY_BULLET] = 0;
		objs.motionTimer[OBJ_ENEMY_SHIP] = 0;

		ObjectsBuildActiveList(objs);
		HudFactionLED();
		return;
	}

	// Launch new enemy bullet from enemy ship toward player (E1:1291-1317)

	// E1:1291: set enemy bullet timer = $20 (32 frames)
	objs.motionTimer[OBJ_ENEMY_BULLET] = 0x0020;

	// E1:1292: activate in current room
	objs.slotTable[OBJ_ENEMY_BULLET] = 0x00;

	// E1:1293-1296: sub_041A5A (shift 4) + sub_041A38 + sub_041A46
	// Scale and negate relative vector (shift 4 = close range speed)
	objs.velX[OBJ_ENEMY_BULLET] = -(dx >> 4);
	objs.velY[OBJ_ENEMY_BULLET] = -(dy >> 4);
	objs.velZ[OBJ_ENEMY_BULLET] = -(dz >> 4);

	// Enemy bullet starts at enemy ship position
	objs.posX[OBJ_ENEMY_BULLET] = objs.posX[OBJ_ENEMY_SHIP];
	objs.posY[OBJ_ENEMY_BULLET] = objs.posY[OBJ_ENEMY_SHIP];
	objs.posZ[OBJ_ENEMY_BULLET] = objs.posZ[OBJ_ENEMY_SHIP];

	// E1:1297-1316: enemy bullet launch sound -- noise + envelope on channel B
	game.intro.mixerShadow |= 0x02;	 // BSET #1 -- disable tone B
	game.intro.mixerShadow &= ~0x10; // BCLR #4 -- enable noise B

	AudioWriteReg(game.audio, YM_NOISE_PERIOD, 0x1F);
	AudioWriteReg(game.audio, YM_MIXER, game.intro.mixerShadow);
	AudioWriteReg(game.audio, YM_AMP_B, 0x10);
	AudioWriteReg(game.audio, YM_ENV_FINE, 0x00);
	AudioWriteReg(game.audio, YM_ENV_COARSE, 0x08);
	AudioWriteReg(game.audio, YM_ENV_SHAPE, 0x00);

	ObjectsBuildActiveList(objs);
	HudFactionLED();
}

// CombatOutdoorTick: wraps the four combat steps in original order

void CombatOutdoorTick(Game &game)
{
	// E1:12776: sub_0416D6 -- player projectile launch
	PlayerProjectileLaunch(game);

	// E1:12777: sub_041442 -- building destruction timer
	DestructionTimerTick(game);

	// E1:12778: sub_041258 -- object motion + bullet lifecycle
	ObjectMotionTick(game);

	// E1:12779: sub_041832 -- enemy bullet tracking
	MissileTrackingTick(game);
}
