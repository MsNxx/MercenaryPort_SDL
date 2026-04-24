#pragma once

// Intro starfield -- 140 perspective-projected stars

#include "renderer/FrameBuffer.h"

#include <cstdint>

constexpr int STAR_COUNT = 140;		  // $8B + 1
constexpr int STAR_CENTER_X = 0x00A0; // 160
constexpr int STAR_CENTER_Y = 0x0044; // 68

struct Star
{
	uint32_t x;		   // log-float 3D X
	uint32_t y;		   // log-float 3D Y
	uint32_t z;		   // log-float 3D Z
	uint16_t drawType; // pixel draw variant
	int16_t screenX;   // -1 if off-screen
	int16_t screenY;   // -1 if off-screen
};

struct Starfield
{
	Star stars[STAR_COUNT];
	uint32_t cameraZ; // log-float reference depth
	uint16_t freqCounter;
	uint16_t freqCounter2;
	uint32_t rngState;
};

void StarfieldInit(Starfield &sf, uint8_t *indexBuf);
void StarfieldDraw(Starfield &sf, uint8_t *indexBuf);

// Returns false when animation complete
bool StarfieldAdvanceCamera(Starfield &sf);

// Returns true when crash descent complete
bool StarfieldCrashDescent(Starfield &sf);
