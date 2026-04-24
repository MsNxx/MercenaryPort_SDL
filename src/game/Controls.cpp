#include "game/Controls.h"

#include "game/Game.h"

#include "audio/Audio.h"
#include "data/GameData.h"
#include "game/Interior.h"
#include "game/KeyBinds.h"
#include "game/ObjectSlots.h"
#include "game/Objects.h"
#include "game/Workspace.h"
#include "renderer/Hud.h"
#include "renderer/LogMath.h"

// Thrust byte lookup
// Values extracted from the original THRUST_TABLE (dat_04546A) at the
// known scancode positions.  Indexed by (action - Action::THRUST_1)
//
// Index  0-9  = forward thrust 1-0  (ST scancodes $02-$0B)
// Index 10    = halt                 (ST scancode $39)
// Index 11-20 = reverse thrust 1-0  (ST scancodes $3B-$44)

static constexpr uint8_t THRUST_BYTES[] = {
	0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, // fwd 1-0
	0x80,														// halt
	0x09, 0x0B, 0x0D, 0x0F, 0x11, 0x13, 0x15, 0x17, 0x19, 0x1B, // rev 1-0
};

// Returns the thrust byte for a thrust/halt action, or 0 if not a
// thrust action.  The caller uses this byte with the original
// thrust-byte-to-log-float conversion (E1:6914-6933)
static constexpr uint8_t ThrustByte(Action a)
{
	int idx = static_cast<int>(a) - static_cast<int>(Action::THRUST_1);
	if (idx >= 0 && idx < 21)
		return THRUST_BYTES[idx];
	return 0;
}

// sub_0441E8 (E1:5176): interaction ping sound
// Channel B tone with envelope decay.  Called after speed changes,
// vehicle boarding, and other key actions
void InteractionPing(Game &game)
{
	game.intro.mixerShadow &= ~0x02;				 // BCLR #1 -- enable tone B
	AudioWriteReg(game.audio, YM_TONE_B_FINE, 0x60); // E1:5180
	AudioWriteReg(game.audio, YM_TONE_B_COARSE, 0x00); // E1:5183
	game.intro.mixerShadow |= 0x10; // BSET #4 -- disable noise B
	AudioWriteReg(game.audio, YM_MIXER, game.intro.mixerShadow); // E1:5187
	AudioWriteReg(game.audio, YM_AMP_B, 0x10);					 // E1:5190
	AudioWriteReg(game.audio, YM_ENV_FINE, 0x00);				 // E1:5193
	AudioWriteReg(game.audio, YM_ENV_COARSE, 0x1F);				 // E1:5196
	AudioWriteReg(game.audio, YM_ENV_SHAPE, 0x00);				 // E1:5199
	// E1:5200-5204: silence channel C unless soundLock ($0624F2)
	if (g_workspace.cam.soundLock == 0)
		AudioWriteReg(game.audio, YM_AMP_C, 0x00); // E1:5204
}

// sub_0462DC (E1:8509-8548): interior stack unwind
// Drops all carried items at random surface locations

static void InteriorStackUnwind(Game &game)
{
	while (g_workspace.inventoryStackDepth > 0)
	{
		// E1:8514: MOVE.B 11508(A0),D0 -- read slot index
		uint8_t slot =
			g_workspace.inventoryStack[g_workspace.inventoryStackDepth - 1];

		// E1:8520: MOVE.B #$00,12596(A1) -- clear slotTable
		g_workspace.objs.slotTable[slot] = 0x00;

		// E1:8521: BCLR #$07,12724(A1) -- clear "taken" flag
		g_workspace.objs.flagsTable[slot] &= ~OBJ_FLAG_TAKEN;

		// E1:8523-8533: random X position
		// ROR.W (A0); MOVE.W (A0)+,D0; wrap pointer;
		// ASL.L #4,D0; ANDI.L #$000FFFFF,D0 -> posX
		{
			uint16_t idx = (game.damageFlashPtr >> 1) & 0x07FF;
			game.randomBuf[idx] = static_cast<uint16_t>(
				(game.randomBuf[idx] >> 1) |
				(game.randomBuf[idx] << 15));  // E1:8524 ROR.W
			uint32_t rX = game.randomBuf[idx]; // E1:8525
			game.damageFlashPtr =
				static_cast<uint16_t>(((game.damageFlashPtr + 2) & 0x0FFF) |
									  0x7000); // E1:8526-8529 wrap
			rX = (rX << 4) & 0x000FFFFF;	   // E1:8531-8532
			g_workspace.objs.posX[slot] = static_cast<int32_t>(rX); // E1:8533
		}

		// E1:8535-8545: random Z position (same pattern)
		{
			uint16_t idx = (game.damageFlashPtr >> 1) & 0x07FF;
			game.randomBuf[idx] = static_cast<uint16_t>(
				(game.randomBuf[idx] >> 1) |
				(game.randomBuf[idx] << 15));  // E1:8536 ROR.W
			uint32_t rZ = game.randomBuf[idx]; // E1:8537
			game.damageFlashPtr =
				static_cast<uint16_t>(((game.damageFlashPtr + 2) & 0x0FFF) |
									  0x7000); // E1:8538-8541 wrap
			rZ = (rZ << 4) & 0x000FFFFF;	   // E1:8543-8544
			g_workspace.objs.posZ[slot] = static_cast<int32_t>(rZ); // E1:8545
		}

		// E1:8546: MOVE.L #$00000000,12084(A2) -- posY = 0
		g_workspace.objs.posY[slot] = 0;

		// E1:8547: SUBQ.W #1,($062484)
		g_workspace.inventoryStackDepth--;
	}
}

// loc_04556C (E1:7001-7031): bail-out / eject
// Triggered by Help key (Atari scancode $62), mapped to Ctrl+Q
// Ejects the player from any vehicle, resets to ground level outdoors

static void BailOut(Game &game)
{
	Camera &cam = g_workspace.cam;

	// E1:7002-7003: CMPI.W #$000A,($0623FE); BEQ -- can't bail out
	// during Prestinium descent (flightState == 10)
	if (cam.flightState == 0x000A)
		return;

	// E1:7004-7005: clamp posX/posZ to 20-bit world grid
	cam.posX &= 0x000FFFFF;
	cam.posZ &= 0x000FFFFF;

	// E1:7006: posY = $0000003F (ground level)
	cam.posY = 0x0000003F;

	// E1:7007: pitch = $0200
	cam.pitch = 0x0200;

	// E1:7008: clear room
	g_workspace.objs.currentRoom = 0;

	// E1:7009-7010: clear script state
	game.scriptVM.endgameFlag = 0;
	game.scriptVM.scriptRunning = 0;

	// E1:7011: MOVE.L #$00000001,($0623FC) -- renderMode=0, flightState=1
	cam.renderMode = 0;
	cam.flightState = 0x0001;

	// E1:7012: BSR sub_0462DC -- interior stack unwind
	// Pops all entries, restores each object to a random surface
	// position with flags cleared
	InteriorStackUnwind(game);

	// E1:7013: clear endgameFlag again (already cleared above)

	// E1:7014: hide player's dart on bail-out
	g_workspace.objs.slotTable[OBJ_DOMINION_DART_B] = 0xFF;

	// E1:7015: BSR sub_040CFC -- reload gameplay palette
	game.activeLut = game.gameplayLut;

	// E1:7016: BSR sub_048042 -- reset palette overrides
	game.palOverride89 = PAL_DEFAULT_89;
	game.palOverride1011 = PAL_DEFAULT_1011;

	// E1:7017: BSR sub_043C14 -- rebuild active list + faction LED
	ObjectsBuildActiveList(g_workspace.objs);
	HudFactionLED();

	// E1:7018: BSR sub_044176 -- reset thrust/velocity
	cam.thrustAccum = LOG_FLOAT_ZERO;
	cam.vertVelocity = LOG_FLOAT_ZERO;

	// E1:7019-7030: bail-out sound setup -- silence B+C, reset mixer/noise
	// Reg 9: AMP_B = 0 (silence channel B)
	AudioWriteReg(game.audio, YM_AMP_B, 0x00);
	// Reg $0A: AMP_C = 0 (silence channel C)
	AudioWriteReg(game.audio, YM_AMP_C, 0x00);
	// Reg 7: restore mixer from shadow ($0624EA)
	AudioWriteReg(game.audio, YM_MIXER, game.intro.mixerShadow);
	// Reg 6: noise period = $1F
	AudioWriteReg(game.audio, YM_NOISE_PERIOD, 0x1F);

	// E1:7031: BRA sub_0441E8 -- interaction ping (tone B + envelope)
	InteractionPing(game);
}

// loc_047EC6 (E1:11622-11687): elevator handler
// Indoor -> descend ($0001/$0042), outdoor on building -> ascend ($8000/$0D84)

static void ElevatorHandler(Game &game)
{
	Camera &cam = g_workspace.cam;

	// E1:11622: TST.W ($06244C) -- elevator already active?
	if (cam.elevatorActive != 0)
		return;

	// E1:11624: TST.W ($0623FC) -- indoor?
	if (cam.renderMode != 0)
	{
		// Indoor path (E1:11626-11641): descend to next room
		// Scan room transition table for current room
		const uint8_t *ft = gen_e1::ROOM_TRANSITIONS;
		uint16_t curRoom = g_workspace.objs.currentRoom;

		for (int i = 0; i < 8; i++)
		{
			uint16_t tableRoom =
				static_cast<uint16_t>((ft[i * 6] << 8) | ft[i * 6 + 1]);
			uint32_t destCoords = static_cast<uint32_t>(
				(ft[i * 6 + 2] << 24) | (ft[i * 6 + 3] << 16) |
				(ft[i * 6 + 4] << 8) | ft[i * 6 + 5]);

			if (tableRoom == 0)
				return; // end sentinel
			if (static_cast<uint8_t>(tableRoom) !=
				static_cast<uint8_t>(curRoom))
				continue;

			// E1:11635: store destination coords at $0624E6
			cam.elevatorDest = destCoords;

			// E1:11636-11639: fill base palette ($062438-$06243E) with
			// value at $042FE4 = ELEVATOR_PARAMS entry 7 offset 0 = $0000
			{
				uint16_t w = static_cast<uint16_t>(
					(gen_e1::ELEVATOR_PARAMS[7 * 8 + 0] << 8) |
					gen_e1::ELEVATOR_PARAMS[7 * 8 + 1]);
				uint32_t packed = (static_cast<uint32_t>(w) << 16) | w;
				cam.palBase89 = packed;
				cam.palBase1011 = packed;
			}

			// E1:11640: elevatorPhase = $0042
			cam.elevatorPhase = 0x0042;
			// E1:11641: elevatorActive = $0001 (ascending/exiting)
			cam.elevatorActive = 0x0001;
			return;
		}
		return;
	}

	// Outdoor path (E1:11644-11684): ascend into building

	// E1:11645: TST.W ($0623B2) -- must be grounded
	if (cam.grounded == 0)
		return;

	// E1:11647-11654: check player is on a building tile
	// $0623A8 = posX byte [15:8] (third byte of big-endian longword)
	// $0623B0 = posZ byte [15:8]
	// Both must satisfy (byte >> 2) == $1C, i.e. byte in $70-$73
	uint8_t xMid = static_cast<uint8_t>((cam.posX >> 8) & 0xFF);
	uint8_t zMid = static_cast<uint8_t>((cam.posZ >> 8) & 0xFF);
	if ((xMid >> 2) != 0x1C)
		return;
	if ((zMid >> 2) != 0x1C)
		return;

	// E1:11655-11658: build packed tile coord for table lookup
	// MOVE.W ($0623A6),D1 -- high word of posX (bits 31-16)
	// SWAP D1 -- to high long
	// MOVE.W ($0623AE),D1 -- high word of posZ into low word
	// ANDI.L #$00FF00FF -- keep low byte of each word
	uint8_t xTile = static_cast<uint8_t>((cam.posX >> 16) & 0xFF);
	uint8_t zTile = static_cast<uint8_t>((cam.posZ >> 16) & 0xFF);
	uint32_t tileCoord = (static_cast<uint32_t>(xTile) << 16) | zTile;

	// E1:11659-11665: scan room transition table
	const uint8_t *ft = gen_e1::ROOM_TRANSITIONS;
	for (int i = 0; i < 8; i++)
	{
		uint16_t tableRoom =
			static_cast<uint16_t>((ft[i * 6] << 8) | ft[i * 6 + 1]);
		uint32_t tableCoords = static_cast<uint32_t>(
			(ft[i * 6 + 2] << 24) | (ft[i * 6 + 3] << 16) |
			(ft[i * 6 + 4] << 8) | ft[i * 6 + 5]);

		if (tableRoom == 0)
			return; // end sentinel
		if (tableCoords != tileCoord)
			continue;

		// E1:11666-11671: room 5 requires pass
		//   TST.B ($000631E2).L
		//   BMI loc_047F92        ; bit 7 set -> has pass, proceed
		//   MOVEQ #57,D1
		//   BRA sub_0441C0        ; else show message 57 ("PASS HOLDERS ONLY")
		// $0631E2 sits in FLAGS_TABLE at slot 46 ($0631B4 + 46 = $0631E2)
		// The script VM flips bit 7 of this slot when the player picks up
		// the room-5 pass
		if (tableRoom == 0x0005)
		{
			if ((g_workspace.objs.flagsTable[OBJ_PASS] & OBJ_FLAG_TAKEN) == 0)
			{
				MessageDisplay(game, 57); // "PASS HOLDERS ONLY"
				return;
			}
		}

		// E1:11674: set new room
		g_workspace.objs.currentRoom = static_cast<uint8_t>(tableRoom);

		// E1:11675: elevatorActive = $8000 (ascending/entering)
		cam.elevatorActive = 0x8000;

		// E1:11676-11679: fill bottom palette ($062440-$062446)
		// with $04300C = $0111 (dark grey)
		// $04300C is ELEVATOR_PARAMS byte offset $60 (entry 12, offset 0)
		uint16_t botPal =
			static_cast<uint16_t>((gen_e1::ELEVATOR_PARAMS[12 * 8] << 8) |
								  gen_e1::ELEVATOR_PARAMS[12 * 8 + 1]);
		cam.groundPal89 = (static_cast<uint32_t>(botPal) << 16) | botPal;
		cam.groundPal1011 = (static_cast<uint32_t>(botPal) << 16) | botPal;

		// E1:11680-11683: fill base palette ($062438-$06243E)
		// with $043004 = ELEVATOR_PARAMS entry 11 offset 0 = $0320
		{
			uint16_t basePal =
				static_cast<uint16_t>((gen_e1::ELEVATOR_PARAMS[11 * 8] << 8) |
									  gen_e1::ELEVATOR_PARAMS[11 * 8 + 1]);
			uint32_t packed = (static_cast<uint32_t>(basePal) << 16) | basePal;
			cam.palBase89 = packed;
			cam.palBase1011 = packed;
		}

		// E1:11684: elevatorPhase = $0D84
		cam.elevatorPhase = 0x0D84;
		return;
	}
}

// sub_045306 (E1:6875-6944): keyboard command dispatch

void KeyCommandDispatch(Game &game)
{
	Action key = g_workspace.keyCommand;

	// E1:6876: clear action buffer
	// Only clear if this dispatch CONSUMES the key.  BOARD_VEHICLE
	// and TAKE_ITEM are consumed later by ObjectsDraw during the
	// rendering pass, so we must NOT clear them here.  They are
	// cleared after ObjectsDraw if not consumed (see GameplayTick)
	if (key != Action::BOARD_VEHICLE && key != Action::TAKE_ITEM)
		g_workspace.keyCommand = Action::NONE;

	if (key == Action::NONE)
		return; // no key pressed

	// E1:6895-6896: bail-out (Ctrl+Q on PC, Help key on ST)
	// Ejects from vehicle and resets to ground level outdoors
	if (key == Action::BAIL_OUT)
	{
		BailOut(game);
		return;
	}

	// E1:6897-6900: weapon sight / walk-run keys
	// These are dispatched BEFORE the state gate -- they work in
	// all states (indoor, outdoor, on foot, in vehicle)
	if (key == Action::WALK)
	{
		// W key -> loc_040FA6 (E1:641-653): walk mode (type 3)
		// JSR sub_0441E8 -- interaction ping
		InteractionPing(game);
		// MOVE.L #$00031800,($062400) -- walk speed
		g_workspace.cam.movementSpeed = 0x00031800;
		// MOVE.W #$0006,($062404) -- walk turn rate
		g_workspace.cam.turnRate = 0x0006;
		// loc_04100A: draw indicator into both buffers
		HudWeaponSightIndicator();
		return;
	}
	if (key == Action::RUN)
	{
		// R key -> loc_040FDA (E1:655-666): run mode (type 4)
		// JSR sub_0441E8 -- interaction ping
		InteractionPing(game);
		// MOVE.L #$00041800,($062400) -- run speed
		g_workspace.cam.movementSpeed = 0x00041800;
		// MOVE.W #$0008,($062404) -- run turn rate
		g_workspace.cam.turnRate = 0x0008;
		// loc_04100A: draw indicator into both buffers
		HudWeaponSightIndicator();
		return;
	}

	// E1:6901-6904: state gate
	// If renderMode != 0 (indoors) OR flightState < 0 (on foot),
	// skip speed keys and go to exit key dispatch
	// Speed keys only work when outdoors AND in a vehicle
	if (g_workspace.cam.renderMode != 0 ||
		static_cast<int16_t>(g_workspace.cam.flightState) < 0)
	{
		goto exit_dispatch;
	}

	// E1:6905-6908: speed adjust keys (only when in vehicle outdoors)
	if (key == Action::DECELERATE)
	{
		// loc_0453EA (E1:6946-6961): decrease speed
		// LogMultiply thrustAccum by $FFFF1F80 (multiply by ~0.98)
		// If result goes negative, reset to LOG_FLOAT_ZERO via sub_044176
		// and DO NOT ping (BMI branches past the BSR sub_0441E8)
		uint32_t cur = g_workspace.cam.thrustAccum;
		uint32_t result = LogMultiply(cur, 0xFFFF1F80);
		if (static_cast<int32_t>(result) < 0)
		{
			g_workspace.cam.thrustAccum = LOG_FLOAT_ZERO;
			g_workspace.cam.vertVelocity = LOG_FLOAT_ZERO;
		}
		else
		{
			// E1:6959-6960: store result and ping
			g_workspace.cam.thrustAccum = result;
			InteractionPing(game);
		}
		return;
	}
	if (key == Action::ACCELERATE)
	{
		// loc_045428 (E1:6963-6980): increase speed
		// LogMultiply thrustAccum by $00001080
		// If result > $000E0000, BGT to RTS -- no update, no ping
		uint32_t cur = g_workspace.cam.thrustAccum;
		uint32_t result = LogMultiply(cur, 0x00001080);
		if (result <= 0x000E0000)
		{
			// E1:6976-6977: store result and ping
			g_workspace.cam.thrustAccum = result;
			InteractionPing(game);
		}
		return;
	}

	// E1:6909-6933: thrust table lookup
	{
		uint8_t thrustByte = ThrustByte(key);
		if (thrustByte == 0)
			goto exit_dispatch; // not a thrust action

		// E1:6914-6933: convert thrust byte to log-float
		// Traced conversion for each value:
		//   $08(key1)->$00041000, $0A(key2)->$00051000, ...
		//   $1A(key0)->$000D1000.  Reverse: $FF001000
		uint32_t d1;
		if (thrustByte & 0x80)
		{
			// Negative: reverse thrust (E1:6915)
			d1 = 0xFF001000;
		}
		else
		{
			// Positive: forward/reverse thrust (E1:6919-6928)
			// EXT.W D1; SWAP D1; ASR.L #1,D1; TST.W D1
			// Even thrust bytes (number keys) -> low word $0000 -> BPL -> $1000
			// Odd thrust bytes (F-keys) -> low word $8000 -> BMI -> $F000
			int16_t ext = static_cast<int16_t>(static_cast<int8_t>(thrustByte));
			int32_t val = static_cast<int32_t>(ext) << 16;
			val >>= 1; // ASR.L #1
			uint16_t loWord = static_cast<uint16_t>(val & 0xFFFF);
			if (static_cast<int16_t>(loWord) < 0)
				val = (val & 0xFFFF0000) | 0xF000; // E1:6924
			else
				val = (val & 0xFFFF0000) | 0x1000; // E1:6928
			d1 = static_cast<uint32_t>(val);
		}

		// E1:6931-6934: compare, update, play sound
		if (d1 != g_workspace.cam.thrustAccum)
		{
			g_workspace.cam.thrustAccum = d1;
			InteractionPing(game);
		}
		return;
	}

exit_dispatch:
	// loc_0453CA (E1:6937-6944): exit key dispatch
	// These work regardless of flight/ground state
	if (key == Action::LEAVE_VEHICLE)
	{
		// L key: vehicle exit (loc_0440AC)
		ObjectVehicleExit(g_workspace.cam, g_workspace.objs);
		// ping only when not in elevator -- the original calls sub_0441E8
		// unconditionally, but the elevator VBL overwrites channels B+C
		// within one tick, making it inaudible on real hardware
		if (g_workspace.cam.elevatorActive == 0)
		{
			InteractionPing(game);
		}
	}
	if (key == Action::DROP_ITEM)
	{
		// D key: drop item (loc_044104, E1:5101-5130)
		// Guards at E1:5102-5112: elevator, control, stack non-empty
		if (g_workspace.cam.elevatorActive == 0 &&
			(static_cast<int8_t>(
				 g_workspace.objs.flagsTable[OBJ_MOBILE_PYRAMID]) < 0 ||
			 g_workspace.cam.grounded != 0) &&
			g_workspace.inventoryStackDepth > 0)
		{
			// E1:5113: decrement stack depth
			g_workspace.inventoryStackDepth--;

			// E1:5114: read stored slot index from stack
			uint8_t slot =
				g_workspace.inventoryStack[g_workspace.inventoryStackDepth];

			// E1:5117: BCLR #7 -- clear "currently entered" flag
			g_workspace.objs.flagsTable[slot] &= ~OBJ_FLAG_TAKEN;

			// loc_044140 (E1:5119-5130): place object at camera position

			// E1:5120: BSR sub_0441E8 -- interaction ping
			InteractionPing(game);

			// E1:5121: set room byte = current room
			g_workspace.objs.slotTable[slot] = g_workspace.objs.currentRoom;

			// E1:5125-5129: restore object world position from camera
			g_workspace.objs.posX[slot] = g_workspace.cam.posX;
			// E1:5126-5127: posY with low byte cleared
			g_workspace.objs.posY[slot] = g_workspace.cam.posY & ~0xFFu;
			g_workspace.objs.posZ[slot] = g_workspace.cam.posZ;

			// E1:5130: BRA sub_043C14 -- rebuild active list
			ObjectsBuildActiveList(g_workspace.objs);
			HudFactionLED();
		}
	}
	if (key == Action::ELEVATOR)
	{
		// E key: elevator (loc_047EC6, E1:11622)
		ElevatorHandler(game);
	}
}
