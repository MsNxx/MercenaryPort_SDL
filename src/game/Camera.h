#pragma once

// Camera state and trig computation for the gameplay renderer

#include <cstdint>

struct TileDetailState;
struct ScriptVM;

struct Camera
{
	// World position
	int32_t posX;
	int32_t posY; // altitude, positive = up
	int32_t posZ;

	// Euler angles (10-bit, 0-1023 = full circle)
	uint16_t heading; // yaw
	uint16_t roll;
	uint16_t pitch;

	// Trig lookup results -- log-floats (32-bit: [exp:16 | mantissa:16])
	uint32_t sinRoll;
	uint32_t cosRoll;
	uint32_t tanRoll;
	uint32_t cotRoll;
	uint32_t sinPitch;
	uint32_t cosPitch;
	uint32_t tanPitch;
	uint32_t sinHeading;
	uint32_t cosHeading;

	// 3x3 rotation matrix (9 x 32-bit log-floats)
	uint32_t matrix[9];

	// 0 = outdoor, >0 = indoor/special, $A = intro descent
	int32_t renderMode;

	// Log-float altitude factor for screen centre and channel coords
	uint32_t altFactor;

	// Screen centre offsets (dynamically computed from altitude + matrix)
	uint16_t screenCenterX; // normally 160
	uint16_t screenCenterY; // normally 68

	// 4-edge line clipping channel coordinates
	uint32_t channelCoord[4];

	static constexpr uint32_t FOV_BASE = 0x00070000;

	uint16_t horizonMirrorMask; // 0 = normal, $FFFF = flip horizon/ground

	// Coarse grid workspace
	uint16_t roadDrawDisable; // nonzero suppresses coarse grid draw

	// Movement physics state
	uint16_t flightState;  // index into the flight state table
	uint16_t grounded;	   // nonzero when on the ground
	uint32_t thrust;	   // current thrust (log-float)
	uint32_t vertVelocity; // vertical velocity (log-float)
	uint32_t thrustAccum;  // thrust accumulator (log-float)
	uint16_t landingProx;  // landing pad proximity flag
	uint8_t inputFlags;	   // joystick/keyboard input bits
	bool crashEvent;	   // set by CrashLand, cleared by Game after handling

	// Flight/landing state
	uint8_t landingDelay;	 // 3-frame landing immunity countdown
	uint16_t crashSoundFlag; // nonzero triggers crash sound + YM writes
	uint16_t elevatorActive; // $0001=descending, $8000=ascending
	uint16_t soundLock;		 // nonzero suppresses AMP_C silence

	// Timer B raster split state
	uint16_t timerBScanline; // scanline count for palette split
	uint32_t palBase89;		 // base palette regs 8-9
	uint32_t palBase1011;	 // base palette regs 10-11
	uint32_t groundPal89;	 // bottom palette regs 8-9
	uint32_t groundPal1011;	 // bottom palette regs 10-11

	// Interior state
	uint32_t elevatorDest;	  // destination coords from room transition table
	uint16_t elevatorPhase;	  // elevator transition phase counter
	uint16_t mirrorMask;	  // X-axis flip $0000 or $FFFF
	uint16_t transCooldown;	  // room transition cooldown frames
	uint16_t roomChangedFlag; // room transition flag
	uint16_t pendingRoomMsg;  // pending room message index
	uint16_t pendingMsg;	  // queued message word
	uint16_t pendingMsgFlag;  // 0=idle, $0001=setup, $FFFF=rendering
	uint32_t movementSpeed;	  // indoor movement speed
	uint16_t turnRate;		  // indoor turn rate
	uint32_t doorTablePtr;	  // pointer into room door data
	uint8_t roomExtentX;	  // room X bound
	uint8_t roomExtentZ;	  // room Z bound
	uint8_t wallContactState; // 1 = player clamped to room boundary, 0 = free
	int8_t doorWipeDir;		  // <0 = hide wipe, >0 = reveal wipe, 0 = idle

	// Combat / destruction state
	uint16_t collapseCountdown; // frames remaining (set to 40 on hit)
	uint16_t collapseTileIndex; // packed tile coords of hit building
	uint8_t savedTileByte;		// original tile byte before collapse
	int16_t patrolCounterA;		// Z-axis patrol accumulator
	int16_t patrolCounterB;		// X-axis patrol accumulator
};

// Compute the altitude factor from posY
void CameraComputeAltFactor(Camera &cam);

// Compute trig lookup values from current angles
void CameraComputeTrig(Camera &cam, TileDetailState &td);

// Build the 3x3 rotation matrix from the trig values
void CameraBuildMatrix(Camera &cam);

// Run one frame of outdoor flight state dispatch
void CameraFlightTick(Camera &cam);

// Run one frame of movement (elevator / indoor / outdoor dispatch)
void CameraMovementTick(Camera &cam, ScriptVM &vm);
