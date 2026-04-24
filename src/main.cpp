// Entry point.  SDL bootstrap, event loop, presentation, timing
// All game logic lives in Game.h/.cpp

#include <SDL.h>

#include <cstdio>

#include "game/Game.h"
#include "game/KeyBinds.h"
#include "game/Workspace.h"
#include "renderer/Palette.h"

#ifndef PIXEL_SCALE
#define PIXEL_SCALE 3
#endif

constexpr int TICKS_PER_SECOND = 50;

// Input state passed from the event pump to the game tick
struct Input
{
	bool anyKeyDown;
	bool yKeyDown;
	bool quit;
	// Save/load dialog input (Ctrl+S/L trigger, digit/return)
	bool ctrlS;
	bool ctrlL;
	int digitKey; // 0-9 if a digit was pressed, -1 otherwise
	bool returnKey;
};

// SDL handles grouped together so init/shutdown are symmetric
struct Platform
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
};

// Frame timing state for the 50 Hz fixed-step loop
struct Timing
{
	uint64_t perfFreq;
	uint64_t tickPeriod;
	uint64_t nextTick;
};

static bool PlatformInit(Platform &plat)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
	{
		std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		return false;
	}

	plat.window = SDL_CreateWindow(
		"Mercenary", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		FB_WIDTH * PIXEL_SCALE, FB_HEIGHT * PIXEL_SCALE, SDL_WINDOW_RESIZABLE);
	if (plat.window == NULL)
	{
		std::fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
		SDL_Quit();
		return false;
	}

	plat.renderer = SDL_CreateRenderer(
		plat.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (plat.renderer == NULL)
	{
		std::fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
		SDL_DestroyWindow(plat.window);
		SDL_Quit();
		return false;
	}

	SDL_RenderSetLogicalSize(plat.renderer, FB_WIDTH, FB_HEIGHT);
	SDL_RenderSetIntegerScale(plat.renderer, SDL_TRUE);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

	plat.texture =
		SDL_CreateTexture(plat.renderer, SDL_PIXELFORMAT_ARGB8888,
						  SDL_TEXTUREACCESS_STREAMING, FB_WIDTH, FB_HEIGHT);
	if (plat.texture == NULL)
	{
		std::fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
		SDL_DestroyRenderer(plat.renderer);
		SDL_DestroyWindow(plat.window);
		SDL_Quit();
		return false;
	}

	return true;
}

static void PlatformShutdown(Platform &plat)
{
	SDL_DestroyTexture(plat.texture);
	SDL_DestroyRenderer(plat.renderer);
	SDL_DestroyWindow(plat.window);
	SDL_Quit();
}

static Input PollInput()
{
	Input input = {false, false, false, false, false, -1, false};

	// Don't clear keyCommand here -- the game tick clears it
	// at sub_045306 entry (E1:6876).  If we clear every 50Hz
	// frame, keypresses on non-tick frames get lost because
	// the game only processes keys at 25Hz

	SDL_Event ev;
	while (SDL_PollEvent(&ev))
	{
		switch (ev.type)
		{
		case SDL_QUIT:
			input.quit = true;
			break;
		case SDL_KEYDOWN:
			if (ev.key.repeat)
				break; // ignore key repeat -- original consumes once

			if (ev.key.keysym.sym == KEY_QUIT)
			{
				input.quit = true;
			}
			else
			{
				input.anyKeyDown = true;
				if (ev.key.keysym.sym == KEY_YES)
					input.yKeyDown = true;

				// Map SDL key + modifiers to game action via
				// the KEY_MAP[] table in Keybinds.h
				bool ctrlHeld = (ev.key.keysym.mod & KMOD_CTRL) != 0;
				bool shiftHeld = (ev.key.keysym.mod & KMOD_SHIFT) != 0;
				for (int i = 0; i < KEY_MAP_COUNT; i++)
				{
					const auto &m = KEY_MAP[i];
					if (m.flagBit != 0)
						continue; // polled key, handled below
					// Modifier matching: entries with a modMask only
					// match when that modifier is held.  Bare entries
					// (modMask == 0) match regardless of modifiers --
					// on the original ST, the scancode is the same
					// whether Ctrl/Shift are held or not.  Ordering
					// ensures specific modifier entries win first
					bool modMatch;
					if (m.modMask == KMOD_CTRL)
						modMatch = ctrlHeld;
					else if (m.modMask == KMOD_SHIFT)
						modMatch = shiftHeld;
					else
						modMatch = true;
					if (modMatch && ev.key.keysym.sym == m.sdlKey)
					{
						// Save/load set Input flags, not keyCommand
						if (m.action == Action::SAVE_GAME)
							input.ctrlS = true;
						else if (m.action == Action::LOAD_GAME)
							input.ctrlL = true;
						else
							g_workspace.keyCommand = m.action;
						break;
					}
				}

				// Digit keys for save/load slot selection
				if (!ctrlHeld && !(ev.key.keysym.mod & KMOD_SHIFT) &&
					ev.key.keysym.sym >= SDLK_0 && ev.key.keysym.sym <= SDLK_9)
				{
					input.digitKey =
						static_cast<int>(ev.key.keysym.sym - SDLK_0);
				}

				// Return for save/load confirmation
				if (ev.key.keysym.sym == SDLK_RETURN ||
					ev.key.keysym.sym == SDLK_KP_ENTER)
					input.returnKey = true;
			}
			break;
		default:
			break;
		}
	}
	return input;
}

static void Present(const Platform &plat, const Game &game, uint32_t *argbBuf)
{
	if (game.mode == MODE_GAMEPLAY)
	{
		// Present the front buffer (the one just swapped to display
		// by sub_048B10's buffer toggle at E1:12800-12803)
		const uint8_t *displayBuf = FrontBuffer(game);
		// Per-scanline palette resolve for the viewport (Timer B raster split)
		ResolveFramebufferScanline(displayBuf, FB_WIDTH, FB_HEIGHT, VIEWPORT_H,
								   game.scanlinePalette.lines, game.gameplayLut,
								   argbBuf);
	}
	else
	{
		// Title/intro: no buffer swap, present the draw target directly
		ResolveFramebuffer(game.indexBuf[game.drawBuffer], FB_PIXELS,
						   game.activeLut, argbBuf);
	}
	SDL_UpdateTexture(plat.texture, NULL, argbBuf,
					  FB_WIDTH * static_cast<int>(sizeof(uint32_t)));
	SDL_SetRenderDrawColor(plat.renderer, 0, 0, 0, 255);
	SDL_RenderClear(plat.renderer);
	SDL_RenderCopy(plat.renderer, plat.texture, NULL, NULL);
	SDL_RenderPresent(plat.renderer);
}

static void TimingInit(Timing &t)
{
	t.perfFreq = SDL_GetPerformanceFrequency();
	t.tickPeriod = t.perfFreq / TICKS_PER_SECOND;
	t.nextTick = SDL_GetPerformanceCounter() + t.tickPeriod;
}

static void TimingWait(Timing &t)
{
	uint64_t now = SDL_GetPerformanceCounter();
	if (now < t.nextTick)
	{
		double ms = static_cast<double>(t.nextTick - now) * 1000.0 /
					static_cast<double>(t.perfFreq);
		if (ms > 2.0)
		{
			SDL_Delay(static_cast<uint32_t>(ms - 1.0));
		}
		while (SDL_GetPerformanceCounter() < t.nextTick)
		{
			// busy-wait the last ~1 ms
		}
	}
	t.nextTick += t.tickPeriod;

	// Recover from long stalls (debugger, OS sleep)
	now = SDL_GetPerformanceCounter();
	if (now > t.nextTick + t.tickPeriod * 4)
	{
		t.nextTick = now + t.tickPeriod;
	}
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	Platform plat;
	if (!PlatformInit(plat))
	{
		return 1;
	}

	Game game;
	GameInit(game);

	Timing timing;
	TimingInit(timing);

	uint32_t argbBuf[FB_PIXELS];

	// Wire SDL handles into Game for inline presentation (door wipe loops)
	game.presentCtx.renderer = plat.renderer;
	game.presentCtx.texture = plat.texture;
	game.presentCtx.argbBuf = argbBuf;

	while (game.mode != MODE_QUIT)
	{
		Input input = PollInput();

		// Mute audio when the window loses focus or during title screen
		uint32_t windowFlags = SDL_GetWindowFlags(plat.window);
		bool hasFocus = (windowFlags & SDL_WINDOW_INPUT_FOCUS) != 0;
		game.audio.active = hasFocus && game.mode != MODE_TITLE;

		// Map SDL keyboard to input flags ($0623B4)
		// All bits are active HIGH (SET = pressed)
		// Default = $00 (no input)
		{
			const uint8_t *keys = SDL_GetKeyboardState(NULL);
			uint8_t flags = 0;
			for (int i = 0; i < KEY_MAP_COUNT; i++)
				if (KEY_MAP[i].flagBit != 0 && keys[KEY_MAP[i].scancode])
					flags |= KEY_MAP[i].flagBit;
			g_workspace.cam.inputFlags = flags;
		}

		GameTick(game, input.anyKeyDown, input.yKeyDown, input.quit,
				 input.ctrlS, input.ctrlL, input.digitKey, input.returnKey);
		if (game.mode == MODE_QUIT)
		{
			break;
		}
		Present(plat, game, argbBuf);
		TimingWait(timing);
	}

	PlatformShutdown(plat);
	return 0;
}
