#pragma once

// VBL interrupt handler -- 50 Hz tick for palette, HUD, Benson, audio

#include "renderer/FrameBuffer.h"

struct Game;

// Called every 50 Hz frame
void VBLHandler(Game &game);

// Door opening reveal -- blocks for 33 sweep + 21 hold frames
void DoorWipeReveal(Game &game);

// Door closing hide -- blocks for 25 wait + 33 sweep frames
void DoorWipeHide(Game &game);

// Set up gameplay palette so the first Present isn't a glitch frame
void InitGameplayPalette(Game &game);
