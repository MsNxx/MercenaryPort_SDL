#pragma once

// Global game workspace -- typed structs over the original's flat $060000
// region

#include "game/Camera.h"
#include "game/KeyBinds.h"
#include "game/Objects.h"
#include "game/TileDetail.h"
#include "renderer/Projection.h"

struct Workspace
{
	Camera cam;
	ObjectState objs;
	TileDetailState tileDetail;

	// Projected vertex workspace -- shared between coarse grid road
	// drawer and tile detail renderer, indexed by connectivity-byte / 4
	ProjectedVertex projVerts[64];

	uint8_t *frameBuf[2]; // set by Game during init

	bool pendingPing; // interaction ping sound pending

	// Pending event script slot (-1 = none)
	int16_t pendingEventSlot;

	// Inventory stack -- max 11 entries
	static constexpr int INVENTORY_STACK_MAX = 11;
	uint8_t inventoryStack[INVENTORY_STACK_MAX];
	uint16_t inventoryStackDepth;

	Action keyCommand; // game action buffer, consumed by KeyCommandDispatch

	// Room vertex/edge workspace
	// Vertex longwords: byte 2 = position, byte 3 = attr (or 0)
	// Edges: pairs of vertex byte-offsets (index * 4)
	static constexpr int ROOM_SLOT_MAX = 64;
	int32_t vertLongX[ROOM_SLOT_MAX];
	int32_t vertLongY[ROOM_SLOT_MAX];
	int32_t vertLongZ[ROOM_SLOT_MAX];
	int32_t
		vertLongVel[ROOM_SLOT_MAX]; // collapse scatter velocity (packed X/Z)
	uint16_t edgeA[ROOM_SLOT_MAX];
	uint16_t edgeB[ROOM_SLOT_MAX];
	uint16_t roomVertHW; // vertex high-water byte-offset
	uint16_t roomEdgeHW; // edge high-water byte-offset
};

extern Workspace g_workspace;
