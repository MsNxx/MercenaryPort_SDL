#pragma once

// HUD instrument panel rendering

#include "game/Camera.h"

#include <cstdint>

void HudRenderInstruments(uint8_t *indexBuf, const Camera &cam);
void HudWeaponSightIndicator();
void HudFactionLED();
void HudSightsReticle(uint8_t *indexBuf);
void HudAltitudeWarning();
void HudAltitudeWarningClear();
