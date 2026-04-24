#pragma once

#include <cstdint>

// Named constants for the 64 object/message slots

// Per-object flag bits (flagsTable byte)
enum ObjectFlag : uint8_t
{
	OBJ_FLAG_REWARDED = (1 << 0),	   // room reward already collected
	OBJ_FLAG_BIT1 = (1 << 1),		   // unused
	OBJ_FLAG_TRIGGER_EVENT = (1 << 2), // pickup triggers event script
	OBJ_FLAG_SHOW_MESSAGE = (1 << 3),  // pickup triggers message display
	OBJ_FLAG_NOT_TAKEABLE = (1 << 4),  // needs kitchen sink
	OBJ_FLAG_HEAVY = (1 << 5),		   // needs antigrav to collect
	OBJ_FLAG_BIT6 = (1 << 6),		   // unused
	OBJ_FLAG_TAKEN = (1 << 7),		   // item in player's possession
};

enum ObjectSlot
{
	// Vehicles and NPCs
	OBJ_DOMINION_DART_A = 0x00,
	OBJ_DOMINION_DART_B = 0x01,
	OBJ_CAR = 0x02,
	OBJ_CONCORDE = 0x03,
	OBJ_HEXAPOD = 0x04,
	OBJ_JET_CAR = 0x05,

	// secret vehicle
	OBJ_CHEESE = 0x06,

	OBJ_INTERSTELLAR_CRAFT = 0x07,

	// Combat and special
	OBJ_PLAYER_BULLET = 0x08,
	OBJ_DOMINION_DART_C = 0x09,
	OBJ_PRESTINIUM = 0x0A,
	OBJ_DOMINION_DART_D = 0x0B,
	OBJ_MECHANOID = 0x0C,
	OBJ_DOMINION_DART_E = 0x0D,
	OBJ_ENEMY_BULLET = 0x0E,
	OBJ_ENEMY_SHIP = 0x0F,

	// Collectible items
	OBJ_PHOTON_EMITTER = 0x10, // lights up dark rooms
	OBJ_KEY_A = 0x11,
	OBJ_KEY_B = 0x12,
	OBJ_KEY_C = 0x13,
	OBJ_KEY_D = 0x14,
	OBJ_KEY_E = 0x15,
	OBJ_KEY_F = 0x16,
	OBJ_KEY_G = 0x17,
	OBJ_ANTI_TIME_BOMB = 0x18, // reconstructs a destroyed building
	OBJ_NOVADRIVE = 0x19,	   // needed for interstellar ship
	OBJ_METAL_DETECTOR = 0x1A,
	OBJ_ANTIGRAV = 0x1B,  // allows picking up vehicles and more items
	OBJ_POWER_AMP = 0x1C, // enhances ship to reach colony craft
	OBJ_NEUTRON_FUEL = 0x1D,
	OBJ_ANTENNA = 0x1E, // use in comms room for spaceship rental
	OBJ_ENERGY_CRYSTAL = 0x1F,
	OBJ_COFFIN = 0x20,
	OBJ_LECTERN = 0x21,
	OBJ_LARGE_BOX = 0x22,
	OBJ_USEFUL_ARMAMENT = 0x23,
	OBJ_COOKER = 0x24,
	OBJ_SALES_FORECAST = 0x25,
	OBJ_GOLD = 0x26,
	OBJ_SOFA = 0x27,
	OBJ_SIGHTS = 0x28,
	OBJ_MEDICAL_SUPPLIES = 0x29,
	OBJ_ESSENTIAL_SUPPLY = 0x2A,
	OBJ_WINCHESTER = 0x2B,
	OBJ_CATERING = 0x2C,
	OBJ_DATABANK = 0x2D,
	OBJ_PASS = 0x2E,
	OBJ_KITCHEN_SINK = 0x2F, // allows picking up fixed items

	// World objects and system slots
	OBJ_COBWEB = 0x30, // lockpick, needs kitchen sink
	OBJ_LAMP = 0x31,
	OBJ_UNKNOWN = 0x32, // used as proxy for author's advert destruction state,
						// blocks interstellar ship launch
	OBJ_FIREPLACE = 0x33,
	OBJ_COMMS_PANEL = 0x34,

	OBJ_TABLE = 0x35,

	OBJ_CHAIR = 0x36,
	OBJ_BED = 0x37,
	OBJ_MOBILE_PYRAMID = 0x38, // allows dropping items in flight
	OBJ_HAZARD_SIGN_A = 0x39,
	OBJ_HAZARD_SIGN_B = 0x3A,
	OBJ_HAZARD_SIGN_C = 0x3B,
	OBJ_HAZARD_SIGN_D = 0x3C,
	OBJ_ELEVATOR_A = 0x3D,
	OBJ_ELEVATOR_B = 0x3E,
	OBJ_COLONY_CRAFT = 0x3F,
};