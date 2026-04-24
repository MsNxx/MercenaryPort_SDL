#include "game/ScriptVm.h"

#include "data/GameData.h"
#include "game/ObjectSlots.h"
#include "game/Workspace.h"

#include <cstdio>
#include <random>

// sub_0441E8: interaction ping sound

static void InteractionPing(ScriptVM &vm)
{
	if (vm.audio == NULL || vm.mixerShadow == NULL)
		return;

	*vm.mixerShadow &= ~0x02;
	AudioWriteReg(*vm.audio, YM_TONE_B_FINE, 0x60);
	AudioWriteReg(*vm.audio, YM_TONE_B_COARSE, 0x00);
	*vm.mixerShadow |= 0x10;
	AudioWriteReg(*vm.audio, YM_MIXER, *vm.mixerShadow);
	AudioWriteReg(*vm.audio, YM_AMP_B, 0x10);
	AudioWriteReg(*vm.audio, YM_ENV_FINE, 0x00);
	AudioWriteReg(*vm.audio, YM_ENV_COARSE, 0x1F);
	AudioWriteReg(*vm.audio, YM_ENV_SHAPE, 0x00);
	AudioWriteReg(*vm.audio, YM_AMP_C, 0x00);
}

// VM state variable access

static void SetStateVar(ScriptVM &vm, uint16_t addr, uint16_t value)
{
	switch (addr)
	{
	case SVAR_SCRIPT_RUNNING:
		vm.scriptRunning = value;
		break;
	case SVAR_TEXT_SPEED:
		vm.textSpeed = value;
		break;
	case SVAR_ATTACK_FLAG:
		vm.attackFlag = value;
		break;
	case SVAR_SHIP_HIRE_FLAG:
		vm.shipHireFlag = value;
		break;
	case SVAR_ENDGAME_FLAG:
		vm.endgameFlag = value;
		break;
	default:
		std::fprintf(stderr, "ScriptVM: SET_E4_WORD unknown addr $%04X\n",
					 addr);
		break;
	}
}

static uint16_t GetStateVar(const ScriptVM &vm, uint16_t addr)
{
	switch (addr)
	{
	case SVAR_SCRIPT_RUNNING:
		return vm.scriptRunning;
	case SVAR_TEXT_SPEED:
		return vm.textSpeed;
	case SVAR_ATTACK_FLAG:
		return vm.attackFlag;
	case SVAR_SHIP_HIRE_FLAG:
		return vm.shipHireFlag;
	case SVAR_ENDGAME_FLAG:
		return vm.endgameFlag;
	default:
		std::fprintf(stderr, "ScriptVM: IF_E4_WORD_EQ unknown addr $%04X\n",
					 addr);
		return 0;
	}
}

// Conditional branch
// shouldBranch = true means the GOTO should fire
// Bit 7 of condFlags inverts

static void CondBranch(ScriptVM &vm, bool shouldBranch, uint8_t condFlags,
					   int target)
{
	bool branch = shouldBranch;
	if (condFlags & SCRIPT_COND_INVERT)
		branch = !branch;

	if (branch && target >= 0)
		vm.pc = target;
	else
		vm.pc++;
}

// BCD addition

static uint32_t BcdAdd(uint32_t a, uint32_t b)
{
	uint32_t result = 0;
	int carry = 0;
	for (int nibble = 0; nibble < 8; nibble++)
	{
		int sum = (a & 0xF) + (b & 0xF) + carry;
		carry = sum >= 10 ? 1 : 0;
		if (carry)
			sum -= 10;
		result |= static_cast<uint32_t>(sum) << (nibble * 4);
		a >>= 4;
		b >>= 4;
	}
	return result;
}

// RNG -- rotating 4KB table walk ($077000-$077FFF)

uint16_t RngWord(ScriptVM &vm)
{
	int wordIdx = vm.rngPointer >> 1;

	// ROR.W (A0): rotate the word at the current pointer right by 1
	uint16_t w = vm.rngTable[wordIdx];
	uint16_t bit0 = w & 1;
	w = (w >> 1) | (bit0 << 15);
	vm.rngTable[wordIdx] = w;

	// MOVE.W (A0)+: read and advance pointer by 2 bytes
	vm.rngPointer = (vm.rngPointer + 2) & 0x0FFF;

	return w;
}

void ScriptVMInit(ScriptVM &vm)
{
	vm.pc = gen_e4::INTRO_ENTRY;
	vm.callDepth = 0;
	vm.frameCounter = 0;

	vm.scriptRunning = gen_e4::STATE_INIT.scriptRunning;
	vm.textSpeed = gen_e4::STATE_INIT.textSpeed;
	vm.attackFlag = gen_e4::STATE_INIT.attackFlag;
	vm.shipHireFlag = gen_e4::STATE_INIT.shipHireFlag;
	vm.endgameFlag = gen_e4::STATE_INIT.endgameFlag;

	vm.flags = 0;

	for (int i = 0; i < ScriptVM::VAR_TABLE_SIZE; i++)
		vm.varTable[i] = 0;
	for (int i = 0; i < ScriptVM::WORD_VAR_TABLE_SIZE; i++)
		vm.wordVarTable[i] = 0;

	vm.keyPressed = false;
	vm.yKeyPressed = false;
	vm.lastKeyByte = 0;
	vm.audio = NULL;
	vm.mixerShadow = NULL;
	vm.objects = NULL;
	vm.camera = NULL;
	vm.eventReturnPC = -1;
	vm.score = 0;

	// Seed the RNG table to simulate power-on RAM garbage
	vm.rngPointer = 0;
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<uint16_t> dist(1, 0xFFFF);
		for (int i = 0; i < ScriptVM::RNG_TABLE_SIZE; i++)
			vm.rngTable[i] = dist(gen);
	}
}

bool ScriptVMTick(ScriptVM &vm, Benson &benson)
{
	if (benson.state != BENSON_IDLE)
		return true;

	if (vm.pc < 0 || vm.pc >= gen_e4::SCRIPT_COUNT)
		return false;

	const ScriptInstr &instr = gen_e4::SCRIPT[vm.pc];

	switch (instr.op)
	{
	case OP_NOP:
	case OP_ENABLE_INPUT:
		vm.pc++;
		break;

	case OP_TEXT:
		BensonSetSpeed(benson, vm.textSpeed);
		BensonDisplay(benson, instr.text);
		vm.pc++;
		break;

	// OP_DISPLAY_MSG (opcode 20, E1:$042DAC)
	//   idx < 240: direct lookup into MESSAGES[]
	//   idx >= 240: indirect -- wordVarTable[idx] holds the real
	//               message index (E1: CMPI #$01E0 after *2)
	case OP_DISPLAY_MSG:
	{
		uint16_t msgIdx = instr.idx;
		if (msgIdx >= 240)
		{
			// Indirect: idx*2 indexes into wordVarTable ($063114),
			// the word there is the real message index
			if (msgIdx < ScriptVM::WORD_VAR_TABLE_SIZE)
				msgIdx = vm.wordVarTable[msgIdx];
		}
		if (msgIdx < gen_e4::MESSAGE_COUNT)
		{
			BensonSetSpeed(benson, vm.textSpeed);
			// Original adds 1 to the message offset (ADDQ.W #1 at
			// $042DD4), skipping the leading padding byte
			BensonDisplay(benson, gen_e4::MESSAGES[msgIdx] + 1);
		}
		vm.pc++;
		break;
	}

	// OP_RANDOM_MSG (opcode 27, E1:$042E8C)
	// Uses the RNG to pick a message: idx = (random & mask) + base
	case OP_RANDOM_MSG:
	{
		uint16_t rng = RngWord(vm);
		uint16_t idx = (rng & instr.mask) + instr.base;
		if (idx < gen_e4::MESSAGE_COUNT)
		{
			BensonSetSpeed(benson, vm.textSpeed);
			// Same +1 skip as OP_DISPLAY_MSG ($042EBE)
			BensonDisplay(benson, gen_e4::MESSAGES[idx] + 1);
		}
		vm.pc++;
		break;
	}

	case OP_CLR_COUNTER:
		vm.frameCounter = 0;
		vm.pc++;
		break;

	case OP_GOTO:
		vm.pc = instr.target;
		break;

	case OP_CALL:
		if (vm.callDepth >= SCRIPT_CALL_STACK_DEPTH)
		{
			std::fprintf(stderr, "ScriptVM: call stack overflow\n");
			return false;
		}
		vm.callStack[vm.callDepth++] = vm.pc + 1;
		vm.pc = instr.target;
		break;

	case OP_RETURN:
		if (vm.callDepth > 0)
		{
			vm.pc = vm.callStack[--vm.callDepth];
		}
		else if (vm.flags & ScriptVM::FLAG_EVENT_ACTIVE)
		{
			// Event script finished -- pop the saved PC and clear
			// the event-active flag (BCLR #6 at $0624B5)
			vm.pc = vm.eventReturnPC;
			vm.eventReturnPC = -1;
			vm.flags &= ~ScriptVM::FLAG_EVENT_ACTIVE;
		}
		else
		{
			return false;
		}
		break;

	case OP_SET_E4_WORD:
		SetStateVar(vm, instr.addr, instr.value);
		if (instr.addr == SVAR_TEXT_SPEED)
			BensonSetSpeed(benson, instr.value);
		vm.pc++;
		break;

	case OP_IF_E4_WORD_EQ:
	{
		uint16_t val = GetStateVar(vm, instr.addr);
		CondBranch(vm, val == instr.value, instr.condFlags, instr.target);
		break;
	}

	case OP_IF_COUNTER_GE:
		// Original BCS: branches when counter < value (spin-wait)
		CondBranch(vm, vm.frameCounter < instr.value, instr.condFlags,
				   instr.target);
		break;

	case OP_SET_VAR_L:
	{
		int slot = static_cast<int>(instr.idx) - 0xF8;
		if (slot >= 0 && slot < ScriptVM::VAR_TABLE_SIZE)
			vm.varTable[slot] = instr.value32;
		vm.pc++;
		break;
	}

	case OP_ADD_VAR_BCD:
	{
		int slot = static_cast<int>(instr.idx) - 0xF8;
		if (slot >= 0 && slot < ScriptVM::VAR_TABLE_SIZE)
			vm.varTable[slot] = BcdAdd(vm.varTable[slot], instr.value32);
		vm.pc++;
		break;
	}

	case OP_SET_FLAGS:
		vm.flags |= instr.mask;
		vm.pc++;
		break;

	case OP_CLR_FLAGS:
		vm.flags &= ~instr.mask;
		vm.pc++;
		break;

	case OP_IF_FLAGS:
		CondBranch(vm, (vm.flags & instr.mask) != 0, instr.condFlags,
				   instr.target);
		break;

	// OP_IF_VEHICLE (opcode 11, E1:$042CE0)
	// MOVE.W (A0)+, D0 -> vehicle type / key code
	// CMP.B ($062425), D0 -> compare low byte with last key scancode
	// BNE -> FALSE; BSR $0441E8 (ping) -> TRUE
	case OP_IF_VEHICLE:
	{
		uint8_t testKey = static_cast<uint8_t>(instr.value & 0xFF);
		bool met = (vm.lastKeyByte == testKey);

		if (met)
		{
			// consume the keypress so it doesn't re-trigger
			// (original's Crawcin eats it from the OS keyboard buffer)
			vm.lastKeyByte = 0;
			InteractionPing(vm);
		}

		CondBranch(vm, met, instr.condFlags, instr.target);
		break;
	}

	// OP_SET_VAR_W (opcode 15, E1:$042D52)
	// A1 = idx * 2 + $063114; [A1] = value
	case OP_SET_VAR_W:
	{
		int varIdx = static_cast<int>(instr.idx);
		if (varIdx >= 0 && varIdx < ScriptVM::WORD_VAR_TABLE_SIZE)
			vm.wordVarTable[varIdx] = instr.value;
		vm.pc++;
		break;
	}

	// OP_SET_ENTITY_FLAG (opcode 32, E1:$042F2E)
	// Entity index from wordVarTable[$FE] (= $063310)
	// If value byte positive: OR into flags.  If negative: AND
	case OP_SET_ENTITY_FLAG:
	{
		if (vm.objects)
		{
			// $063310 = $063114 + $FE * 2 = wordVarTable[0xFE]
			uint16_t entityIdx = vm.wordVarTable[0xFE];
			if (entityIdx < OBJ_SLOTS)
			{
				int8_t flagVal = static_cast<int8_t>(instr.value & 0xFF);
				if (flagVal < 0)
					vm.objects->flagsTable[entityIdx] &=
						static_cast<uint8_t>(flagVal);
				else
					vm.objects->flagsTable[entityIdx] |=
						static_cast<uint8_t>(flagVal);
			}
		}
		vm.pc++;
		break;
	}

	// OP_SET_ENTITY_LOC (opcode 36, E1:$042E62)
	// MOVEA.W (A0)+, A1 -> entity index
	// ADDA.L #$063134, A1 -> &slotTable[entity]
	// ADDQ.W #1, A0 -> skip high byte of location word
	// MOVE.B (A0)+, (A1) -> write low byte to slotTable
	case OP_SET_ENTITY_LOC:
	{
		if (vm.objects)
		{
			uint16_t entityIdx = instr.idx;
			if (entityIdx < OBJ_SLOTS)
				vm.objects->slotTable[entityIdx] =
					static_cast<uint8_t>(instr.value & 0xFF);
		}
		vm.pc++;
		break;
	}

	// OP_BCD_OP3 (opcode 21, E1:$042DE4)
	// A1=base+idx1*4, A2=base+idx2*4, A3=base+idx3*4
	// MOVE.L (A2)+,(A4) -- copy var[idx2] to scratch, advance A2
	// ABCD -(A1),-(A3) x4 -- var[idx3] += var[idx1]
	// MOVE.L (A2),(A1) -- var[idx1] = *(A2) (idx2+1 due to post-inc)
	// Generator: word1->idx, word2->value, word3->mask
	case OP_BCD_OP3:
	{
		int i1 = static_cast<int>(instr.idx) - 0xF8;
		int i2 = static_cast<int>(instr.value) - 0xF8;
		int i3 = static_cast<int>(instr.mask) - 0xF8;
		if (i1 >= 0 && i1 < ScriptVM::VAR_TABLE_SIZE && i2 >= 0 &&
			i2 < ScriptVM::VAR_TABLE_SIZE && i3 >= 0 &&
			i3 < ScriptVM::VAR_TABLE_SIZE)
		{
			// ABCD: var[idx3] += var[idx1]
			vm.varTable[i3] = BcdAdd(vm.varTable[i1], vm.varTable[i3]);
			// MOVE.L (A2),(A1): A2 post-incremented -> next slot
			int i2next = i2 + 1;
			if (i2next < ScriptVM::VAR_TABLE_SIZE)
				vm.varTable[i1] = vm.varTable[i2next];
		}
		vm.pc++;
		break;
	}

	// OP_ADD_SCORE (opcode 22, E1:$042E1C)
	// ADD.W value, ($0624AE)
	case OP_ADD_SCORE:
		vm.score += instr.value;
		vm.pc++;
		break;

	// OP_COPY_WORD_VAR (opcode 30, E1:$042F00)
	// A1 = base + 2*first_word, A2 = base + 2*second_word
	// MOVE.W (A2), (A1): first_word = dest, second_word = src
	// Generator: first_word -> idx, second_word -> value
	case OP_COPY_WORD_VAR:
	{
		int dst = static_cast<int>(instr.idx);
		int src = static_cast<int>(instr.value);
		if (dst >= 0 && dst < ScriptVM::WORD_VAR_TABLE_SIZE && src >= 0 &&
			src < ScriptVM::WORD_VAR_TABLE_SIZE)
		{
			vm.wordVarTable[dst] = vm.wordVarTable[src];
		}
		vm.pc++;
		break;
	}

	case OP_JSR_68K:
	case OP_JSR_COND:
		vm.pc++;
		break;

	// OP_IF_AT_LOC (opcode 6, E1:$042C68)
	// CMP.B ($0623A7), x_byte -> CMP.B ($0623AF), y_byte
	// $0623A7 = byte 1 of posX (bits 16-23), $0623AF = byte 1 of posZ
	// Condition TRUE when both match (player at grid location)
	case OP_IF_AT_LOC:
	{
		bool met = false;
		if (vm.camera)
		{
			uint8_t gridX =
				static_cast<uint8_t>((vm.camera->posX >> 16) & 0xFF);
			uint8_t gridZ =
				static_cast<uint8_t>((vm.camera->posZ >> 16) & 0xFF);
			uint8_t testX = static_cast<uint8_t>((instr.value >> 8) & 0xFF);
			uint8_t testZ = static_cast<uint8_t>(instr.value & 0xFF);
			met = (gridX == testX) && (gridZ == testZ);
		}
		CondBranch(vm, met, instr.condFlags, instr.target);
		break;
	}

	// OP_IF_RANDOM_GE (opcode 10, E1:$042CBA)
	// Reads an RNG word, compares with threshold
	// CMP.W D2, D0 -> BCS: branches when random (D0) < threshold (D2)
	case OP_IF_RANDOM_GE:
	{
		uint16_t rng = RngWord(vm);
		CondBranch(vm, rng < instr.value, instr.condFlags, instr.target);
		break;
	}

	// OP_IF_ENTITY_ALIVE (opcode 12, E1:$042CF0)
	// MOVEA.W ($063310), A1 -> entity index from wordVarTable[$FE]
	// ADDA.L #$0631B4, A1 -> &flagsTable[entity]
	// TST.B (A1) -> BMI: GOTO fires when bit 7 set (entity taken)
	case OP_IF_ENTITY_ALIVE:
	{
		bool met = false;
		if (vm.objects)
		{
			uint16_t entityIdx = vm.wordVarTable[0xFE];
			if (entityIdx < OBJ_SLOTS)
				met = (vm.objects->flagsTable[entityIdx] & OBJ_FLAG_TAKEN) != 0;
		}
		CondBranch(vm, met, instr.condFlags, instr.target);
		break;
	}

	// OP_IF_IN_BLDG (opcode 19, E1:$042D98)
	// MOVE.W ($063312), D0 -> wordVarTable[$FF]
	// CMP.W ($062452), D0 -> BEQ: condition TRUE when equal to currentRoom
	case OP_IF_IN_BLDG:
	{
		bool met = false;
		if (vm.objects)
			met = (vm.wordVarTable[0xFF] == vm.objects->currentRoom);
		CondBranch(vm, met, instr.condFlags, instr.target);
		break;
	}

	// OP_IF_SCORE_GE (opcode 23, E1:$042E2C)
	// MOVE.W (A0)+, D0 -> threshold
	// CMP.W ($0624AE), D0 -> BCS: branches when threshold < score
	case OP_IF_SCORE_GE:
		CondBranch(vm, instr.value < vm.score, instr.condFlags, instr.target);
		break;

	// OP_IF_ENTITY2_ALIVE (opcode 26, E1:$042E76)
	// MOVEA.W ($06330E), A1 -> entity index from wordVarTable[$FD]
	// ADDA.L #$0631F4, A1 -> &typeTable[entity]
	// TST.B (A1) -> BMI: condition TRUE when bit 7 set
	case OP_IF_ENTITY2_ALIVE:
	{
		bool met = false;
		if (vm.objects)
		{
			uint16_t entityIdx = vm.wordVarTable[0xFD];
			if (entityIdx < OBJ_SLOTS)
				met = (vm.objects->typeTable[entityIdx] & 0x80) != 0;
		}
		CondBranch(vm, met, instr.condFlags, instr.target);
		break;
	}

	// OP_IF_ENTITY_OWNER (opcode 29, E1:$042EE4)
	// MOVE.W ($063312), D0 -> wordVarTable[$FF]
	// MOVEA.W ($063310), A1 -> wordVarTable[$FE] (entity index)
	// ADDA.L #$063134, A1 -> &slotTable[entity]
	// CMP.B (A1)+, D0 -> BEQ: TRUE when low byte of var[$FF] ==
	// slotTable[entity]
	case OP_IF_ENTITY_OWNER:
	{
		bool met = false;
		if (vm.objects)
		{
			uint16_t entityIdx = vm.wordVarTable[0xFE];
			if (entityIdx < OBJ_SLOTS)
			{
				uint8_t owner = vm.objects->slotTable[entityIdx];
				uint8_t test = static_cast<uint8_t>(vm.wordVarTable[0xFF]);
				met = (owner == test);
			}
		}
		CondBranch(vm, met, instr.condFlags, instr.target);
		break;
	}

	// OP_IF_CARRYING (opcode 31, E1:$042F1A)
	// MOVE.W ($06330E), D0 -> wordVarTable[$FD]
	// CMP.B ($062420), D0 -> BEQ: TRUE when low byte matches
	// $062420 = Camera::currentTileIndex (packed grid position)
	case OP_IF_CARRYING:
	{
		bool met = false;
		if (vm.camera)
		{
			uint8_t carry = static_cast<uint8_t>(
				(g_workspace.tileDetail.currentTileIndex >> 8) & 0xFF);
			uint8_t test = static_cast<uint8_t>(vm.wordVarTable[0xFD]);
			met = (carry == test);
		}
		CondBranch(vm, met, instr.condFlags, instr.target);
		break;
	}

	// OP_IF_ENTITY_FLAG (opcode 33, E1:$042F4E)
	// MOVEA.W ($063310), A1 -> wordVarTable[$FE] (entity index)
	// ADDA.L #$0631B4, A1 -> &flagsTable[entity]
	// MOVE.W (A0)+, D0 -> mask value from script
	// AND.B (A1), D0 -> BNE: TRUE when (flags & mask) != 0
	case OP_IF_ENTITY_FLAG:
	{
		bool met = false;
		if (vm.objects)
		{
			uint16_t entityIdx = vm.wordVarTable[0xFE];
			if (entityIdx < OBJ_SLOTS)
			{
				uint8_t flags = vm.objects->flagsTable[entityIdx];
				uint8_t mask = static_cast<uint8_t>(instr.value & 0xFF);
				met = (flags & mask) != 0;
			}
		}
		CondBranch(vm, met, instr.condFlags, instr.target);
		break;
	}

	// OP_IF_HAS_ITEM (opcode 34, E1:$042F66)
	// MOVE.W (A0)+, D0 -> item value
	// CMP.W ($0623FE), D0 -> BEQ: TRUE when value == flightState
	case OP_IF_HAS_ITEM:
	{
		bool met = false;
		if (vm.camera)
			met = (instr.value == vm.camera->flightState);
		CondBranch(vm, met, instr.condFlags, instr.target);
		break;
	}

	// OP_IF_VAR_GE (opcode 35, E1:$042D1A)
	// MOVEA.W (A0)+, A1 -> idx, A1*4 -> A1 + base = &var[idx]
	// MOVE.L (A1), D0 -> load 32-bit variable
	// CMP.L (A0)+, D0 -> BCS (val < threshold) to BTST (no toggle),
	// BRA (val >= threshold) to BCHG (toggle)
	// condFlags=0: val < threshold -> branch taken
	case OP_IF_VAR_GE:
	{
		int slot = static_cast<int>(instr.idx) - 0xF8;
		uint32_t val = 0;
		if (slot >= 0 && slot < ScriptVM::VAR_TABLE_SIZE)
			val = vm.varTable[slot];
		CondBranch(vm, val < instr.value32, instr.condFlags, instr.target);
		break;
	}

	default:
		std::fprintf(stderr, "ScriptVM: unknown opcode %d at pc=%d\n", instr.op,
					 vm.pc);
		return false;
	}

	return true;
}

// Event script loading (sub_04418C, E1:5139-5154)

void ScriptVMLoadEvent(ScriptVM &vm, int slot)
{
	if (vm.flags & ScriptVM::FLAG_EVENT_ACTIVE)
		return;
	if (slot < 0 || slot >= gen_e4::EVENT_COUNT)
		return;

	vm.flags |= ScriptVM::FLAG_EVENT_ACTIVE;
	vm.eventReturnPC = vm.pc;
	vm.pc = gen_e4::EVENT_ENTRY[slot];
	vm.callDepth = 0;
}
