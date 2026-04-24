#pragma once

// Tile detail geometry -- loading, spinning animation, collapse, drawing

#include "game/Camera.h"

#include <cstdint>

struct TileDetailState
{
	// Spinning tile object state
	uint16_t spinAngle;	   // global spinning angle (16-bit, wrapping)
	uint32_t spinSin;	   // sin(spinAngle) as log-float
	uint32_t spinCos;	   // cos(spinAngle) as log-float
	uint16_t spinSpeed;	   // rotation speed (from tile data byte 4)
	int16_t spinBaseX;	   // base X position
	int16_t spinBaseZ;	   // base Z position
	uint8_t spinVertCount; // number of spinning vertices (0 = none)

	int8_t tileProperty; // negative = skip spin update

	// Cached high words of posX/posY/posZ for tile change detection
	uint16_t cachedPosX;
	uint16_t cachedPosY;
	uint16_t cachedPosZ;

	// Per-vertex spinning precomputation arrays (log-space deltas)
	static constexpr int MAX_SPIN_VERTS = 64;
	uint32_t spinLogDeltaX[MAX_SPIN_VERTS];
	uint32_t spinLogDeltaZ[MAX_SPIN_VERTS];

	uint8_t tileVertCount; // spinning + non-spinning

	uint8_t cachedD7;		   // cached altitude-to-D7 value; $FF = invalidated
	uint16_t currentTileIndex; // packed (Z<<4)|X, 0 if outside grid
	uint16_t drawModeSplit; // edge index where draw switches from BSET to BCLR
							// (geo[2])
};

// Per-frame spinning vertex rotation
void SpinningVertexUpdate(TileDetailState &td, const Camera &cam);

// Per-frame vertex gravity collapse, returns true if any vertex still active
bool VertexGravityCollapse(TileDetailState &td);

// Outdoor tile detail load -- detects tile changes, loads geometry
void TileDetailLoad(TileDetailState &td, const Camera &cam);

// Tile change handler, bypasses elevatorActive guard
void TileDetailLoadDirect(TileDetailState &td, const Camera &cam);

// Reload tile geometry for repair path
void TileDetailReload(TileDetailState &td, const Camera &cam,
					  uint16_t tileIndex);

// Draw tile detail geometry
void TileDetailDraw(uint8_t *indexBuf, const TileDetailState &td,
					const Camera &cam);
