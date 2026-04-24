// Camera flight physics -- sub_0482D2, sub_0482B6, sub_048264, sub_047DC2

#include "DevUtils.h"
#include "game/Camera.h"
#include "game/ObjectSlots.h"
#include "game/Workspace.h"
#include "renderer/LogMath.h"

// Flight state parameter blocks from jump table at $04637E
//   THRUST=$0482D2  WALK=$0482B6  GRAVITY=$048264  DESCENT=$047DC2
//   SPECIAL=$042240

enum FlightHandler
{
	HANDLER_THRUST,
	HANDLER_WALK,
	HANDLER_GRAVITY,
	HANDLER_DESCENT,
	HANDLER_INTERSTELLAR,
};

struct FlightParams
{
	FlightHandler handler;

	// Longword at A0+4: gravity / thrust multiplier
	uint32_t param4;

	// Words at A0+8, A0+10, A0+12, A0+14:
	//   For HANDLER_GRAVITY: roll delta ($062348), pitch delta ($06234A),
	//   heading delta, (unused) For HANDLER_THRUST/WALK: acceleration param
	//   (longword at A0+8),
	//     pitch/heading rate (A0+12 word), pitch rate add (A0+14 word)
	uint16_t param8w;
	uint16_t paramAw;
	uint16_t paramCw;
	uint16_t paramEw;

	// Longword at A0+$14: Z movement multiplier
	uint32_t param14;

	// Word at A0+$1A: ground clearance
	uint16_t param1A;

	// Longword at A0+$1C: vertical velocity multiplier
	uint32_t param1C;
};

// 11 flight states, extracted from E1 binary at $04637E
static const FlightParams FLIGHT_TABLE[] = {
	// State 0: standard flying (hovering).  $04637E -> $0482D2
	{HANDLER_THRUST, 0xFFFF1E00, 0x0000, 0x1000, 0x0008, 0x0002, 0xFFFF1000,
	 0x005A, 0xFFFF1900},

	// State 1: standard flying (cruising).  $04639E -> $0482D2
	{HANDLER_THRUST, 0xFFFF1E00, 0x0000, 0x1000, 0x0008, 0x0002, 0xFFFF1000,
	 0x000A, 0xFFFF1900},

	// State 2: walking (grounded).  $0463BE -> $0482B6
	{HANDLER_WALK, 0xFFF01000, 0x0002, 0x1000, 0x0000, 0xFFE0, 0xFFFF1000,
	 0x005A, 0xFFFF10D0},

	// State 3: flying (variant).  $0463DE -> $0482D2
	{HANDLER_THRUST, 0xFFFF1C80, 0x0001, 0x1000, 0x000C, 0x0002, 0xFFFF1800,
#if SECOND_CITY
	 0x0049, 0xFFFE1900},
#else
	 0x005A, 0xFFFE1900},
#endif

	// State 4: flying (variant).  $0463FE -> $0482D2
	{HANDLER_THRUST, 0xFFFF1E00, 0x0001, 0x1000, 0x0008, 0xFFE0, 0xFFFE1000,
	 0x0001, 0xFFFF1900},

	// State 5: walking (variant).  $04641E -> $0482B6
	{HANDLER_WALK, 0xFFFF1E00, 0x0000, 0x1000, 0x0000, 0xFFE0, 0xFFFF1000,
	 0x005A, 0x00011338},

	// State 6: flying (variant).  $04643E -> $0482D2
	{HANDLER_THRUST, 0xFFFF1E00, 0x0000, 0x1000, 0x0008, 0x0002, 0xFFFF1000,
	 0x005A, 0x00001900},

	// State 7: interstellar craft launch pose.  $04645E -> $042240
	// Runs briefly after boarding until Game.cpp fires the outro
	{HANDLER_INTERSTELLAR, 0xFFFF1E00, 0x0000, 0x1000, 0x0008, 0x0002,
	 0xFFFF1000, 0x005A, 0xFFFF1900},

	// State 8: flying (boat/vehicle).  $04647E -> $0482D2
	{HANDLER_THRUST, 0xFFF01000, 0x0004, 0x1000, 0x0001, 0x0002, 0xFFFE1000,
	 0x005A, 0x00011000},

	// State 9: shot down (gravity + spin).  $04649E -> $048264
	{HANDLER_GRAVITY, 0x00001700, 0x0007, 0x0009, 0x000D, 0x0000, 0x00000000,
	 0x0000, 0x00000000},

	// State 10: pre-crash descent (exponential gravity).  $0464BE -> $047DC2
	{HANDLER_DESCENT, 0x00000000, 0x0008, 0x0000, 0x0000, 0x0000, 0x00000000,
	 0x0000, 0x00000000},
};

static constexpr int FLIGHT_TABLE_SIZE = 11;

// Crash landing ($0481B8)
// Ground camera, exit vehicle, signal crashEvent for sound + blackout

static void CrashLand(Camera &cam)
{
	// $0481B8
	cam.posY = 0x3F;
	cam.grounded = 0x3F;

	// $0481CA: BSR loc_0440AC
	ObjectVehicleExit(cam, g_workspace.objs);

	// Signal Game.cpp to handle crash sound + blackout
	cam.crashEvent = true;
}

// LogFloatToInt is now in LogMath.cpp (shared with combat code)

// State 10: pre-crash exponential descent ($047DC2)

static void HandleDescent(Camera &cam, const FlightParams &fp)
{
	// $047DC2-$047DDE: exponential gravity
	int32_t y = cam.posY;
	y = y - (y >> 5) - 0x1000;

	if (y < 0)
	{
		CrashLand(cam);
		return;
	}
	cam.posY = y;

	// $04827E-$0482B4: add angle deltas from parameter block
	// A0+8 = roll delta ($062348), A0+10 = pitch delta ($06234A), A0+12 =
	// heading delta
	cam.roll = (cam.roll + fp.param8w) & 0x3FF;
	cam.pitch = (cam.pitch + fp.paramAw) & 0x3FF;
	cam.heading = (cam.heading + fp.paramCw) & 0x3FF;
}

// State 9: gravity-only ($048264)

static void HandleGravity(Camera &cam, const FlightParams &fp)
{
	// $048264: CLR.W $624A4 -- clear landing proximity
	cam.landingProx = 0;

	// $04826A-$04827A: Y -= param (longword at A0+4)
	int32_t y = cam.posY - static_cast<int32_t>(fp.param4);
	if (y < 0)
	{
		CrashLand(cam);
		return;
	}
	cam.posY = y;

	// $04827E-$0482B4: angle deltas
	cam.roll = (cam.roll + fp.param8w) & 0x3FF;
	cam.pitch = (cam.pitch + fp.paramAw) & 0x3FF;
	cam.heading = (cam.heading + fp.paramCw) & 0x3FF;
}

// Landing pad proximity check (sub_048724)

static bool NearLandingPad(const Camera &cam)
{
	// $048724-$04875E
	uint32_t dy = static_cast<uint32_t>(cam.posY - 0x0040FB00);
	if (dy >= 0x0440)
	{
		return false;
	}

	uint32_t dx = static_cast<uint32_t>(cam.posX - 0x00086800);
	if (dx >= 0x1000)
	{
		return false;
	}

	uint32_t dz = static_cast<uint32_t>(cam.posZ - 0x00086800);
	if (dz >= 0x1000)
	{
		return false;
	}

	return true;
}

// Main thrust + movement handler ($0482D2)

static void HandleThrust(Camera &cam, const FlightParams &fp)
{
	uint8_t input = cam.inputFlags;

	// $0482D8-$0482F2: multiply current thrust by param to get speed
	uint32_t speed = LogMultiply(cam.thrust, fp.param4);

	// $0482F6: acceleration param
	uint32_t accel = (static_cast<uint32_t>(fp.param8w) << 16) | fp.paramAw;

	// $0482FA-$0483EA: forward/backward thrust
	// Forward (bit 3): thrust = LogFloatAdd(accel, speed)
	// Backward (bit 2): thrust = LogFloatAdd(-accel, speed)
	if (input & 0x08)
	{
		cam.thrust = LogFloatAdd(accel, speed);
	}
	else if (input & 0x04)
	{
		// Negate accel mantissa for reverse
		uint32_t negAccel =
			(accel & 0xFFFF0000u) |
			static_cast<uint16_t>(-static_cast<int16_t>(accel & 0xFFFF));
		cam.thrust = LogFloatAdd(negAccel, speed);
	}
	else
	{
		cam.thrust = speed;
	}

	// $0483F0-$04841A: pitch from input ($06234A)
	// BTST + BNE = skip if bit SET.  Active-LOW: operation runs
	// when bit is CLEAR.  When no keys pressed, both ADD and SUB
	// fire (net zero).  Pressing one key suppresses one operation
	uint16_t pitchRate = fp.paramCw;
	if (!(input & 0x02))
	{
		cam.pitch = (cam.pitch + pitchRate) & 0x3FF;
	}
	if (!(input & 0x01))
	{
		cam.pitch = (cam.pitch - pitchRate) & 0x3FF;
	}

	// $04841C-$0484D2: vertical velocity computation
	// Multiply thrustAccum by vert velocity param, add to prev vert velocity
	uint32_t vertParam = fp.param1C;
	uint32_t vertInput = cam.thrustAccum;

	// $048422-$048438: power amp boost in cruising mode (state 1)
	// Adds 1 to thrust accumulator exponent, doubling vertical thrust
	if (static_cast<int8_t>(g_workspace.objs.flagsTable[OBJ_POWER_AMP]) < 0 &&
		cam.flightState == 1)
	{
		vertInput += 0x00010000;
	}

	// $04843E-$048446: reduce thrust accumulator exponent when grounded
	if (cam.grounded != 0)
	{
		vertInput -= 0x00020000;
	}

	uint32_t vertSpeed = LogMultiply(vertInput, vertParam);
	cam.vertVelocity = LogFloatAdd(vertSpeed, cam.vertVelocity);

	// $0484D6-$0484EE: multiply accumulated velocity by param14 (damping)
	// limits the velocity to an equilibrium determined by thrustAccum
	uint32_t dampedVel = LogMultiply(cam.vertVelocity, fp.param14);
	cam.vertVelocity = dampedVel;
	uint32_t vel = cam.vertVelocity;

	// $0484F4-$048560: Z movement = vel * cosPitch * cosHeading
	{
		uint32_t cpch = LogMultiply(cam.cosPitch, cam.cosHeading);
		uint32_t zVel = LogMultiply(vel, cpch);
		int32_t zInt = LogFloatToInt(zVel);
		cam.posZ += zInt;
		// $048558-$048560: sign-extend high word
		cam.posZ = static_cast<int32_t>(static_cast<int16_t>(cam.posZ >> 16))
					   << 16 |
				   (cam.posZ & 0xFFFF);
	}

	// $048566-$0485D2: X movement = vel * cosPitch * sinHeading
	{
		uint32_t cpsh = LogMultiply(cam.cosPitch, cam.sinHeading);
		uint32_t xVel = LogMultiply(vel, cpsh);
		int32_t xInt = LogFloatToInt(xVel);
		cam.posX += xInt;
		cam.posX = static_cast<int32_t>(static_cast<int16_t>(cam.posX >> 16))
					   << 16 |
				   (cam.posX & 0xFFFF);
	}

	// $0485D8-$04864A: Y movement = vel * sinPitch + ground correction
	{
		uint32_t yVel = LogMultiply(vel, cam.sinPitch);
		int16_t yMant = static_cast<int16_t>(yVel & 0xFFFF);

		// $0485F6-$04861E: ground proximity correction
		// Original: SUB.W $623AA,D4 -- subtracts the HIGH WORD of posY
		if (yMant >= 0)
		{
			int16_t clearance = static_cast<int16_t>(fp.param1A);

			// $0485FC-$04860E: power amp overrides clearance in state 1
			// without the amp, clearance is $000A (soft ceiling)
			// with the amp, clearance becomes $005A (same as hovering)
			if (static_cast<int8_t>(
					g_workspace.objs.flagsTable[OBJ_POWER_AMP]) < 0 &&
				cam.flightState == 1)
			{
				clearance = 0x005A;
			}

			int16_t altWord = static_cast<int16_t>(
				(static_cast<uint32_t>(cam.posY) >> 16) & 0xFFFF);
			int16_t diff = clearance - altWord;
			if (diff < 0)
			{
				// Below ground clearance -- boost Y
				int32_t boost = static_cast<int32_t>(diff) << 16;
				yVel =
					static_cast<uint32_t>(static_cast<int32_t>(yVel) + boost);
			}
		}

		int32_t yInt = LogFloatToInt(yVel);
		cam.posY += yInt;
	}

	// $048650-$04869C: ground detection
	bool landed = false;
	if (cam.posY >= 0x40)
	{
		// Still airborne.  Check landing pad
		if (NearLandingPad(cam))
		{
			// $048664-$048676: near pad -- snap to pad altitude
			cam.landingProx = 0x3F;
			cam.posY = 0x0040FF3F;
			landed = true; // BRA $04869C
		}
	}
	else
	{
		// $048678-$048692: low altitude checks (all posY < $40 cases)
		int32_t yVelInt = LogFloatToInt(LogMultiply(vel, cam.sinPitch));

		if (yVelInt < -100)
		{
			// Too fast -- crash
			CrashLand(cam);
			return;
		}

		uint16_t pitchCheck = cam.pitch >> 2;
		if ((pitchCheck & 0xFF) >= 0xA0)
		{
			// Pitch too extreme -- crash
			CrashLand(cam);
			return;
		}

		cam.posY = 0x3F;
		landed = true; // falls through to $04869C
	}

	// $04869C: grounded state -- entered from landing pad or ground level
	if (landed)
	{
		cam.grounded = 0x3F;
		cam.roll = 0;

		// $0486AC-$0486C6: snap pitch to level ($0200)
		if (cam.pitch > 0x200)
		{
			cam.pitch = 0x200;
		}
		else if (static_cast<int32_t>(vel) <= static_cast<int32_t>(0x0004FFFF))
		{
			cam.pitch = 0x200;
		}
	}
	else
	{
		// $0486C8: airborne
		cam.grounded = 0;
		cam.landingProx = 0;
	}

	// $0486D4-$0486EE: heading turn from thrust
	{
		int32_t headingDelta = LogFloatToInt(cam.thrust);
		// $0486EE: ADD.W D4,($06234C).L -- no 10-bit mask here
		// The rendering/compass code handles the wrap
		cam.heading += static_cast<uint16_t>(headingDelta);
	}

	// $0486F4-$048722: roll update when airborne
	// Original stores to $062348 (cam.roll), NOT $06234A (cam.pitch)
	// D2 = thrust, SWAP + add paramEw + SWAP -> log-to-int -> store
	if (cam.grounded == 0)
	{
		uint32_t rollVal = cam.thrust;
		uint16_t rollHi = static_cast<uint16_t>(rollVal >> 16);
		rollHi += fp.paramEw;
		rollVal = (static_cast<uint32_t>(rollHi) << 16) | (rollVal & 0xFFFF);
		int32_t rollDelta = LogFloatToInt(rollVal);
		cam.roll = static_cast<uint16_t>(rollDelta);
	}
}

// Walking handler ($0482B6)

static void HandleWalk(Camera &cam, const FlightParams &fp)
{
	HandleThrust(cam, fp);

	if (cam.grounded == 0)
	{
		// $0482C2: walking gravity = $4000 per frame
		cam.posY -= 0x4000;
		if (cam.posY < 0)
		{
			CrashLand(cam);
		}
	}
}

// Interstellar craft handler ($042240) -- camera locked upward while
// the elevator drives ascent; Game.cpp detects the endgame transition

static void HandleSpecial(Camera &cam)
{
	// $042252: pitch = $0100 (looking straight up)
	cam.pitch = 0x0100;
}

// Grounded handler (loc_048760, sub_0487CC/sub_0487CE)
// XZ movement from input, heading changes, landing pad check

static void HandleGrounded(Camera &cam)
{
	// $048768: MOVE.B ($0623B4),D0
	uint8_t input = cam.inputFlags;

	// $04876E: MOVE.L ($062400),D1 -- thrust config
	// $062400 is set by sub_040F9A during init and updated by
	// the W/R keys (walk=$00031800, run=$00041800)
	uint32_t thrustConfig = cam.movementSpeed;

	// $048774: TST.W ($0623FE); BPL $048790
	// If flightState >= 0 (flying), skip thrust -> heading only
	// We're already grounded (flightState < 0), so run thrust

	// $04877C-$04878C: forward/backward from bits 1 and 0
	// In the original, both are independent BSR calls -- both
	// can fire on the same frame (net zero if both pressed)
	// Bit 1 SET = forward (BEQ skip when clear)
	if (input & 0x02)
	{
		// sub_0487CE: forward thrust
		uint32_t d2 = LogMultiply(thrustConfig, cam.sinHeading);
		uint32_t d3 = LogMultiply(thrustConfig, cam.cosHeading);

		// $048806-$048836: LogFloatToInt + EXT.L truncation
		// The original does EXT.L on all paths, clamping to int16_t
		int32_t dx = static_cast<int16_t>(LogFloatToScreen(d2));
		int32_t dz = static_cast<int16_t>(LogFloatToScreen(d3));

		cam.posX += dx;
		cam.posZ += dz;
	}

	// Bit 0 SET = backward (BEQ skip when clear)
	if (input & 0x01)
	{
		// sub_0487CC: NEG.W D1, fall through to sub_0487CE
		uint32_t d1neg = thrustConfig;
		uint16_t lo =
			static_cast<uint16_t>(-static_cast<int16_t>(d1neg & 0xFFFF));
		d1neg = (d1neg & 0xFFFF0000u) | lo;

		uint32_t d2 = LogMultiply(d1neg, cam.sinHeading);
		uint32_t d3 = LogMultiply(d1neg, cam.cosHeading);

		int32_t dx = static_cast<int16_t>(LogFloatToScreen(d2));
		int32_t dz = static_cast<int16_t>(LogFloatToScreen(d3));

		cam.posX += dx;
		cam.posZ += dz;
	}

	// $048790-$0487AC: heading from bits 3 and 2
	// $062404 = turn rate (walk=6, run=8)
	uint16_t turnRate = cam.turnRate;

	// Bit 3 SET = turn right (BEQ skip when clear)
	// The original uses plain ADD.W/SUB.W with no masking
	// Heading wraps naturally at 16 bits; TrigLookup masks internally
	if (input & 0x08)
	{
		cam.heading = cam.heading + turnRate;
	}
	// Bit 2 SET = turn left (BEQ skip when clear)
	if (input & 0x04)
	{
		cam.heading = cam.heading - turnRate;
	}

	// $0487AE-$0487C8: walked off landing pad -> fall
	// BCS at $0487BC skips when near pad (carry set); triggers state 9
	// only when player walks AWAY from the pad
	if (cam.landingProx != 0)
	{
		if (!NearLandingPad(cam))
		{
			// $0487C0: MOVE.L #9,($0623FC) -> renderMode=0, flightState=9
			cam.renderMode = 0;
			cam.flightState = 9;
		}
	}
}

// CameraFlightTick -- outdoor flight state dispatch

void CameraFlightTick(Camera &cam)
{
	// $047DAC-$047DB2: MOVE.W ($623FE),D0; BMI loc_048760
	// Negative flightState = grounded
	if (static_cast<int16_t>(cam.flightState) < 0)
	{
		HandleGrounded(cam);
		return;
	}

	if (cam.flightState >= FLIGHT_TABLE_SIZE)
	{
		return;
	}

	const FlightParams &fp = FLIGHT_TABLE[cam.flightState];

	switch (fp.handler)
	{
	case HANDLER_THRUST:
		HandleThrust(cam, fp);
		break;

	case HANDLER_WALK:
		HandleWalk(cam, fp);
		break;

	case HANDLER_GRAVITY:
		HandleGravity(cam, fp);
		break;

	case HANDLER_DESCENT:
		HandleDescent(cam, fp);
		break;

	case HANDLER_INTERSTELLAR:
		HandleSpecial(cam);
		break;
	}
}
