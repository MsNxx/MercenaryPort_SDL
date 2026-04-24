#pragma once

// Script VM types -- opcodes, instruction struct, condition flags

#include <cstdint>

// Opcodes -- values match the original 6-bit opcode field
enum ScriptOp
{
	OP_NOP = 0,
	OP_TEXT = 1,
	OP_CLR_COUNTER = 2,
	OP_GOTO = 3,
	OP_CALL = 4,
	OP_RETURN = 5,
	OP_IF_AT_LOC = 6,
	OP_IF_COUNTER_GE = 7,
	OP_JSR_68K = 8,
	OP_JSR_COND = 9,
	OP_IF_RANDOM_GE = 10,
	OP_IF_VEHICLE = 11,
	OP_IF_ENTITY_ALIVE = 12,
	OP_SET_VAR_L = 13,
	OP_ADD_VAR_BCD = 14,
	OP_SET_VAR_W = 15,
	OP_SET_FLAGS = 16,
	OP_CLR_FLAGS = 17,
	OP_IF_FLAGS = 18,
	OP_IF_IN_BLDG = 19,
	OP_DISPLAY_MSG = 20,
	OP_BCD_OP3 = 21,
	OP_ADD_SCORE = 22,
	OP_IF_SCORE_GE = 23,
	OP_IF_E4_WORD_EQ = 24,
	OP_SET_E4_WORD = 25,
	OP_IF_ENTITY2_ALIVE = 26,
	OP_RANDOM_MSG = 27,
	OP_ENABLE_INPUT = 28,
	OP_IF_ENTITY_OWNER = 29,
	OP_COPY_WORD_VAR = 30,
	OP_IF_CARRYING = 31,
	OP_SET_ENTITY_FLAG = 32,
	OP_IF_ENTITY_FLAG = 33,
	OP_IF_HAS_ITEM = 34,
	OP_IF_VAR_GE = 35,
	OP_SET_ENTITY_LOC = 36,
};

// Condition flags -- bits 6-7 of the original instruction word
constexpr uint8_t SCRIPT_COND_INVERT = 0x80; // negate condition
constexpr uint8_t SCRIPT_COND_CALL = 0x40;	 // CALL not GOTO

// Script state variable offsets (E4-relative)
constexpr uint16_t SVAR_SCRIPT_RUNNING = 0x0100;
constexpr uint16_t SVAR_TEXT_SPEED = 0x0102;
constexpr uint16_t SVAR_ATTACK_FLAG = 0x0104;
constexpr uint16_t SVAR_SHIP_HIRE_FLAG = 0x0106;
constexpr uint16_t SVAR_ENDGAME_FLAG = 0x0108;

struct ScriptInstr
{
	ScriptOp op;
	uint8_t condFlags;

	uint16_t addr;	  // E4-relative offset
	uint16_t value;	  // 16-bit immediate
	uint32_t value32; // 32-bit immediate
	uint16_t idx;	  // variable/message index
	uint16_t mask;
	uint16_t base; // base index (RANDOM_MSG)
	int target;	   // branch/call target as instruction index (-1 = none)

	const char *text; // inline string for OP_TEXT (null otherwise)
};
