#pragma once

// Top-level game state machine and framebuffer ownership

#include "audio/Audio.h"
#include "game/Camera.h"
#include "game/Intro.h"
#include "game/Outro.h"
#include "game/SaveLoad.h"
#include "renderer/FrameBuffer.h"
#include "renderer/Palette.h"
#include "renderer/ScanlinePalette.h"

#include <cstdint>
#include <string>

struct SDL_Renderer;
struct SDL_Texture;

// Allows Game.cpp to present frames during blocking loops (door wipe etc)
struct PresentContext
{
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	uint32_t *argbBuf;
};

enum GameMode
{
	MODE_TITLE,
	MODE_INTRO,
	MODE_GAMEPLAY,
	MODE_OUTRO, // interstellar escape -- starfield + endgame script
	MODE_QUIT,
};

struct Game
{
	GameMode mode;
	uint32_t frameCount;

	// Double-buffered indexed framebuffer (two 320x200 pages)
	uint8_t indexBuf[2][FB_PIXELS];
	uint8_t drawBuffer;		// 0 or 1 -- back buffer index
	uint8_t *prevBensonBuf; // previous VBL tick's Benson write target

	// Precomputed ARGB palette LUTs
	uint32_t titleLut[PALETTE_ENTRIES];
	uint32_t initialLut[PALETTE_ENTRIES];
	uint32_t gameplayLut[PALETTE_ENTRIES];

	uint32_t *activeLut; // whichever LUT is active this frame

	Audio audio;

	// Script VM -- single instance shared across intro and gameplay
	ScriptVM scriptVM;

	IntroState intro;
	OutroState outro;

	Benson benson;

	// Gameplay runs at ~25 Hz (every other 50 Hz tick)
	bool gameplayTickPhase;

	// VBL palette override for entries 8-11 (ST palette words, $0RGB)
	uint32_t palOverride89;	  // entries 8-9
	uint32_t palOverride1011; // entries 10-11

	int blackoutTimer;

	// Palette 3 flash animation index -- cycles through PAL3_FLASH_TABLE
	int pal3FlashIndex;

	// Damage palette flash state
	int damageFlashTimer;
	uint16_t damageFlashPtr;  // low 12 bits of pointer into randomBuf
	uint16_t randomBuf[2048]; // 4KB random data buffer

	uint16_t prevElevatorActive; // for 1->0 transition detection

	PresentContext presentCtx;

	ScanlinePaletteState scanlinePalette;

	int devInventoryState; // 0=inactive, 1=waiting for blackout, 2=ready

	SaveLoadState saveLoad;
	std::string savePath;
};

inline uint8_t *DrawTarget(Game &game)
{
	return game.indexBuf[game.drawBuffer];
}
inline const uint8_t *FrontBuffer(const Game &game)
{
	return game.indexBuf[1 - game.drawBuffer];
}
inline uint8_t *VBLTarget(Game &game)
{
	return game.indexBuf[1 - game.drawBuffer];
}

void GameInit(Game &game);

void GameTick(Game &game, bool anyKeyDown, bool yKeyDown, bool quit,
			  bool ctrlS = false, bool ctrlL = false, int digitKey = -1,
			  bool returnKey = false);

// Queue a message for Benson display
void MessageDisplay(Game &game, int msgIndex);
