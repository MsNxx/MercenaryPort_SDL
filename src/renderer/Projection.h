#pragma once

// 3D projection -- world coordinates to screen pixels via log-float math

#include "game/Camera.h"

#include <cstdint>

struct ProjectedVertex
{
	int16_t screenX;
	int16_t screenY;

	// $0000=visible, $0001=X off, $0002=Y off, $8000=behind camera
	uint16_t visFlags;

	// Camera-space log-float coordinates
	uint32_t camX;
	uint32_t camY;
	uint32_t camZ;

	// Perspective-divided ratios (before FOV scale), valid when not behind
	// camera
	uint32_t xzRatio;
	uint32_t yzRatio;

	uint16_t xzSign;
	uint16_t yzSign;

	bool isBehind() const { return (visFlags & 0x8000) != 0; }
};

ProjectedVertex ProjectVertex(int32_t worldX, int32_t worldY, int32_t worldZ,
							  const Camera &cam);

// Takes pre-computed log-float inputs (used by road drawer)
ProjectedVertex ProjectVertexLogFloat(uint32_t lfX, uint32_t lfY, uint32_t lfZ,
									  const Camera &cam);
