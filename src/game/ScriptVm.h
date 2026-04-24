#pragma once

// Script VM dispatcher -- executes one instruction per tick

#include "audio/Audio.h"
#include "game/Benson.h"
#include "game/Camera.h"
#include "game/Objects.h"
#include "game/Script.h"

#include <cstdint>

constexpr int SCRIPT_CALL_STACK_DEPTH = 8;

struct ScriptVM
{
	int pc; // instruction index into gen_e4::SCRIPT[]

	int callStack[SCRIPT_CALL_STACK_DEPTH];
	int callDepth;

	uint32_t frameCounter; // incremented each tick, reset by CLR_COUNTER

	// Script state variables
	uint16_t scriptRunning;
	uint16_t textSpeed;
	uint16_t attackFlag;
	uint16_t shipHireFlag;
	uint16_t endgameFlag;

	// Flags word at $0624B4 -- SET_FLAGS / CLR_FLAGS / IF_FLAGS
	// bit 6 = event active (BSET by sub_04418C, cleared by script)
	static constexpr uint16_t FLAG_EVENT_ACTIVE = 0x0040;
	uint16_t flags;

	// Keyboard input for Y/N prompts
	bool keyPressed;
	bool yKeyPressed;
	uint8_t lastKeyByte; // scancode of last key press

	Audio *audio;
	uint8_t *mixerShadow;

	ObjectState *objects;
	Camera *camera;

	// Event return address -- used when FLAG_EVENT_ACTIVE is set
	int eventReturnPC;

	uint16_t score;

	// RNG -- 4KB rotating table walked by a 12-bit pointer
	static constexpr int RNG_TABLE_SIZE = 2048;
	uint16_t rngTable[RNG_TABLE_SIZE];
	uint16_t rngPointer;

	// 16-bit word variable table
	static constexpr int WORD_VAR_TABLE_SIZE = 256;
	uint16_t wordVarTable[WORD_VAR_TABLE_SIZE];

	// BCD variable table
	static constexpr int VAR_TABLE_SIZE = 26;
	uint32_t varTable[VAR_TABLE_SIZE];
};

void ScriptVMInit(ScriptVM &vm);
bool ScriptVMTick(ScriptVM &vm, Benson &benson);
uint16_t RngWord(ScriptVM &vm);

// Load an event script by slot index -- pushes current PC, ignored if
// an event is already active
void ScriptVMLoadEvent(ScriptVM &vm, int slot);
