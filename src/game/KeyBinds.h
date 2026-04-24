#pragma once

// Keyboard bindings and game action definitions

#include <SDL.h>

enum class Action : uint16_t
{
	NONE = 0,

	// Joystick-equivalent (polled every frame)
	MOVE_FORWARD,
	MOVE_BACKWARD,
	TURN_LEFT,
	TURN_RIGHT,
	FIRE,

	// System (Ctrl+key)
	SAVE_GAME,
	LOAD_GAME,

	// Movement mode
	WALK,
	RUN,

	// Speed control (vehicle, outdoor only)
	DECELERATE,
	ACCELERATE,

	// Interaction
	BOARD_VEHICLE,
	LEAVE_VEHICLE,
	TAKE_ITEM,
	DROP_ITEM,
	ELEVATOR,

	BAIL_OUT,

	// Forward thrust (number keys 1-0)
	THRUST_1,
	THRUST_2,
	THRUST_3,
	THRUST_4,
	THRUST_5,
	THRUST_6,
	THRUST_7,
	THRUST_8,
	THRUST_9,
	THRUST_0,

	HALT,

	// Reverse thrust (F1-F10 / Shift+1-0)
	REV_THRUST_1,
	REV_THRUST_2,
	REV_THRUST_3,
	REV_THRUST_4,
	REV_THRUST_5,
	REV_THRUST_6,
	REV_THRUST_7,
	REV_THRUST_8,
	REV_THRUST_9,
	REV_THRUST_0,
};

// Key-to-action mapping entry
// First match wins; modifier entries listed before bare keys for priority
struct KeyMapping
{
	SDL_Keycode sdlKey;
	SDL_Scancode scancode; // non-UNKNOWN = polled every frame
	uint16_t modMask;	   // 0 = no modifier, KMOD_CTRL, KMOD_SHIFT
	uint8_t flagBit;	   // inputFlags bit for polled keys, 0 otherwise
	Action action;
};

constexpr SDL_Keycode KEY_QUIT = SDLK_ESCAPE;
constexpr SDL_Keycode KEY_YES = SDLK_y;

constexpr SDL_Scancode EVENT_ONLY = SDL_SCANCODE_UNKNOWN;

constexpr KeyMapping KEY_MAP[] = {
	// Joystick-equivalent (polled)
	{SDLK_UP, SDL_SCANCODE_UP, 0, 0x01, Action::MOVE_FORWARD},
	{SDLK_DOWN, SDL_SCANCODE_DOWN, 0, 0x02, Action::MOVE_BACKWARD},
	{SDLK_LEFT, SDL_SCANCODE_LEFT, 0, 0x08, Action::TURN_LEFT},
	{SDLK_RIGHT, SDL_SCANCODE_RIGHT, 0, 0x04, Action::TURN_RIGHT},
	{SDLK_SLASH, SDL_SCANCODE_SLASH, 0, 0x80, Action::FIRE},

	// Ctrl+key
	{SDLK_s, EVENT_ONLY, KMOD_CTRL, 0, Action::SAVE_GAME},
	{SDLK_l, EVENT_ONLY, KMOD_CTRL, 0, Action::LOAD_GAME},
	{SDLK_q, EVENT_ONLY, KMOD_CTRL, 0, Action::BAIL_OUT},

	// Shift+number -> reverse thrust (ST Shift+1..0 = F1..F10)
	{SDLK_1, EVENT_ONLY, KMOD_SHIFT, 0, Action::REV_THRUST_1},
	{SDLK_2, EVENT_ONLY, KMOD_SHIFT, 0, Action::REV_THRUST_2},
	{SDLK_3, EVENT_ONLY, KMOD_SHIFT, 0, Action::REV_THRUST_3},
	{SDLK_4, EVENT_ONLY, KMOD_SHIFT, 0, Action::REV_THRUST_4},
	{SDLK_5, EVENT_ONLY, KMOD_SHIFT, 0, Action::REV_THRUST_5},
	{SDLK_6, EVENT_ONLY, KMOD_SHIFT, 0, Action::REV_THRUST_6},
	{SDLK_7, EVENT_ONLY, KMOD_SHIFT, 0, Action::REV_THRUST_7},
	{SDLK_8, EVENT_ONLY, KMOD_SHIFT, 0, Action::REV_THRUST_8},
	{SDLK_9, EVENT_ONLY, KMOD_SHIFT, 0, Action::REV_THRUST_9},
	{SDLK_0, EVENT_ONLY, KMOD_SHIFT, 0, Action::REV_THRUST_0},

	// Interaction
	{SDLK_b, EVENT_ONLY, 0, 0, Action::BOARD_VEHICLE},
	{SDLK_l, EVENT_ONLY, 0, 0, Action::LEAVE_VEHICLE},
	{SDLK_t, EVENT_ONLY, 0, 0, Action::TAKE_ITEM},
	{SDLK_d, EVENT_ONLY, 0, 0, Action::DROP_ITEM},
	{SDLK_e, EVENT_ONLY, 0, 0, Action::ELEVATOR},

	// Movement mode
	{SDLK_w, EVENT_ONLY, 0, 0, Action::WALK},
	{SDLK_r, EVENT_ONLY, 0, 0, Action::RUN},

	// Speed adjust
	{SDLK_MINUS, EVENT_ONLY, 0, 0, Action::DECELERATE},
	{SDLK_EQUALS, EVENT_ONLY, 0, 0, Action::ACCELERATE},
	{SDLK_SPACE, EVENT_ONLY, 0, 0, Action::HALT},

	// Forward thrust (number keys 1-0)
	{SDLK_1, EVENT_ONLY, 0, 0, Action::THRUST_1},
	{SDLK_2, EVENT_ONLY, 0, 0, Action::THRUST_2},
	{SDLK_3, EVENT_ONLY, 0, 0, Action::THRUST_3},
	{SDLK_4, EVENT_ONLY, 0, 0, Action::THRUST_4},
	{SDLK_5, EVENT_ONLY, 0, 0, Action::THRUST_5},
	{SDLK_6, EVENT_ONLY, 0, 0, Action::THRUST_6},
	{SDLK_7, EVENT_ONLY, 0, 0, Action::THRUST_7},
	{SDLK_8, EVENT_ONLY, 0, 0, Action::THRUST_8},
	{SDLK_9, EVENT_ONLY, 0, 0, Action::THRUST_9},
	{SDLK_0, EVENT_ONLY, 0, 0, Action::THRUST_0},

	// Reverse thrust (F-keys)
	{SDLK_F1, EVENT_ONLY, 0, 0, Action::REV_THRUST_1},
	{SDLK_F2, EVENT_ONLY, 0, 0, Action::REV_THRUST_2},
	{SDLK_F3, EVENT_ONLY, 0, 0, Action::REV_THRUST_3},
	{SDLK_F4, EVENT_ONLY, 0, 0, Action::REV_THRUST_4},
	{SDLK_F5, EVENT_ONLY, 0, 0, Action::REV_THRUST_5},
	{SDLK_F6, EVENT_ONLY, 0, 0, Action::REV_THRUST_6},
	{SDLK_F7, EVENT_ONLY, 0, 0, Action::REV_THRUST_7},
	{SDLK_F8, EVENT_ONLY, 0, 0, Action::REV_THRUST_8},
	{SDLK_F9, EVENT_ONLY, 0, 0, Action::REV_THRUST_9},
	{SDLK_F10, EVENT_ONLY, 0, 0, Action::REV_THRUST_0},
};

constexpr int KEY_MAP_COUNT =
	static_cast<int>(sizeof(KEY_MAP) / sizeof(KEY_MAP[0]));
