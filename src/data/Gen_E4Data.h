#pragma once

// DO NOT EDIT -- auto-generated

#include "game/Script.h"

namespace gen_e4
{

constexpr int SCRIPT_COUNT = 525;
extern const ScriptInstr SCRIPT[SCRIPT_COUNT];

constexpr int EVENT_COUNT = 51;
extern const int EVENT_ENTRY[EVENT_COUNT];

constexpr int INTRO_ENTRY = 0;

constexpr int MESSAGE_COUNT = 64;
extern const char *MESSAGES[MESSAGE_COUNT];

struct ScriptStateInit
{
	uint16_t scriptRunning;
	uint16_t textSpeed;
	uint16_t attackFlag;
	uint16_t shipHireFlag;
	uint16_t endgameFlag;
};
extern const ScriptStateInit STATE_INIT;

} // namespace gen_e4
