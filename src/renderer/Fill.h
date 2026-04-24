#pragma once

// Sky/ground horizon fill for the gameplay viewport

#include "game/Camera.h"

#include <cstdint>

void FillViewport(uint8_t *indexBuf, const Camera &cam);
