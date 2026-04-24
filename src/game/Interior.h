#pragma once

// Indoor scene management -- room loading, projection, drawing

#include "game/Camera.h"
#include <cstdint>

// Room loader -- populates vertex/edge workspace with room geometry
struct ScriptVM;
void RoomLoad(uint16_t room, uint8_t buildingIndex, ScriptVM &vm);

// Project all room vertices through the yaw-only projector
void RoomProjectVertices(const Camera &cam);

// Draw all room edges as BSET lines
void RoomDrawEdges(uint8_t *indexBuf, const Camera &cam);

// Clamp camera posX/posZ to room bounds, returns true at boundary
bool RoomBoundaryClamp(Camera &cam);

// Scan door table for proximity match, returns true if door found
bool RoomDoorScan(Camera &cam, struct Game &game, uint16_t &outRoom,
				  uint8_t &outBuildingIndex);
