#pragma once

// Coarse road grid -- altitude-dependent tile window of 32 line segments

#include "game/Camera.h"

#include <cstdint>

void RoadsDraw(uint8_t *indexBuf, const Camera &cam);
void RoadsTerrainUpdate(Camera &cam);
void RoadsBuildWorkspace(const Camera &cam, int d7);
