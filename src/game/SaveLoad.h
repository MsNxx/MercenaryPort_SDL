#pragma once

// Save/load system -- Ctrl+S / Ctrl+L modal dialog

#include <cstdint>
#include <string>

struct Game;

enum SaveLoadPhase
{
	SL_IDLE,
	SL_SHOW_PROMPT,
	SL_AWAIT_DIGIT,
	SL_SHOW_CONFIRM,
	SL_AWAIT_CONFIRM,
	SL_EXECUTE,
	SL_RESULT,
};

struct SaveLoadState
{
	SaveLoadPhase phase = SL_IDLE;
	bool isSave = false;
	int slot = -1;
	int resultTimer = 0;
};

struct SaveLoadInput
{
	int digitKey = -1; // 0-9 if pressed, -1 otherwise
	bool returnKey = false;
	bool anyKeyDown = false;
};

void SaveLoadTrigger(SaveLoadState &sl, bool isSave);
bool SaveLoadActive(const SaveLoadState &sl);
void SaveLoadTick(SaveLoadState &sl, Game &game, const SaveLoadInput &input);

// Initialise save directory path, call once at startup
std::string SaveLoadInitPath();

bool SaveToFile(const Game &game, const std::string &dir, int slot);
bool LoadFromFile(Game &game, const std::string &dir, int slot);
