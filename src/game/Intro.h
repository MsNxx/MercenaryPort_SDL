#pragma once

// Intro sequence -- scripted descent in three phases

#include "game/Benson.h"
#include "game/ScriptVm.h"
#include "renderer/PlanetDisc.h"
#include "renderer/StarField.h"

#include "renderer/FrameBuffer.h"

#include <cstdint>

enum IntroPhase
{
	INTRO_PHASE_INIT_DELAY, // artificial delay replacing ST init time
	INTRO_PHASE_1,			// countdown, static stars
	INTRO_PHASE_2,			// ship animation, stars moving
	INTRO_PHASE_3,			// crash descent, stars decelerating
	INTRO_PHASE_4,			// planet disc growing, stars frozen
};

struct IntroState
{
	Starfield starfield;
	PlanetDisc planetDisc;
	IntroPhase phase;
	uint32_t frameCount;
	uint8_t mixerShadow; // shadow of YM mixer register 7
};

void IntroInit(IntroState &state, ScriptVM &vm, Benson &benson,
			   uint8_t *indexBuf);
bool IntroTick(IntroState &state, ScriptVM &vm, Benson &benson,
			   uint8_t *indexBuf, Audio *audio = nullptr);
