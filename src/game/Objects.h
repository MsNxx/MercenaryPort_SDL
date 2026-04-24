#pragma once

// World object rendering -- 64 slots for buildings, vehicles, items, interiors

#include "game/Camera.h"
#include "renderer/Projection.h"

#include <cstdint>

constexpr int OBJ_SLOTS = 64;
constexpr int OBJ_MAX_VERTS = 64;

struct ObjectState
{
	// Per-slot mutable state
	int32_t posX[OBJ_SLOTS];
	int32_t posY[OBJ_SLOTS];
	int32_t posZ[OBJ_SLOTS];
	int32_t velX[OBJ_SLOTS];
	int32_t velY[OBJ_SLOTS];
	int32_t velZ[OBJ_SLOTS];
	uint16_t motionTimer[OBJ_SLOTS];
	uint8_t slotTable[OBJ_SLOTS];  // room byte
	uint8_t flagsTable[OBJ_SLOTS]; // flag bits
	uint8_t typeTable[256];		   // first 64 = per-object type/status,
								   // full 256 = per-tile property
	uint8_t rotSelector[16];	   // rotation byte (slots 0-15)

	// Active list built each frame
	uint8_t activeList[OBJ_SLOTS];
	uint16_t activeCount;

	uint8_t currentRoom;
};

// Initialise object state from E2 binary data
void ObjectsInit(ObjectState &objs);

// Build active object list for the current room
void ObjectsBuildActiveList(ObjectState &objs);

// Draw all active objects (BCLR mode, iterates active list backward)
void ObjectsDraw(uint8_t *indexBuf, const Camera &cam, const ObjectState &objs);

// Camera-relative transform with SWAP/EXT.W compression
void CameraRelativeTransform(int32_t objX, int32_t objY, int32_t objZ,
							 const Camera &cam, int32_t &outDX, int32_t &outDY,
							 int32_t &outDZ);

// Chebyshev (L-infinity) distance test
bool WithinDistance(int32_t dx, int32_t dy, int32_t dz, uint32_t threshold);

// Remove player from current vehicle, place it at camera position
void ObjectVehicleExit(Camera &cam, ObjectState &objs);
