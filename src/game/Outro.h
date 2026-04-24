#pragma once

// Endgame sequence -- interstellar escape starfield ($042240)

#include "game/Benson.h"
#include "game/ScriptVm.h"
#include "renderer/StarField.h"

#include "renderer/FrameBuffer.h"

#include <cstdint>

struct Audio;

struct OutroState
{
	Starfield starfield;
	uint8_t mixerShadow;
};

void OutroInit(OutroState &state, ScriptVM &vm, Benson &benson,
			   uint8_t *indexBuf);
void OutroTick(OutroState &state, ScriptVM &vm, Benson &benson,
			   uint8_t *indexBuf, Audio *audio = nullptr);
