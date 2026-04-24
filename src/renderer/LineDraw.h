#pragma once

// Line drawing with 3-path visibility dispatch and 4-channel edge clipping

#include "game/Camera.h"
#include "renderer/Projection.h"

#include <cstdint>

enum class LineMode : uint16_t
{
	BSET = 0x0000, // pixel |= 1  (roads)
	BCLR = 0x0020, // pixel &= ~3 (building outlines)
};

// BSET mode wrapper
void DrawLineProjected(uint8_t *indexBuf, const ProjectedVertex &vA,
					   const ProjectedVertex &vB, const Camera &cam);

void DrawLineProjectedMode(uint8_t *indexBuf, const ProjectedVertex &vA,
						   const ProjectedVertex &vB, const Camera &cam,
						   LineMode mode);
