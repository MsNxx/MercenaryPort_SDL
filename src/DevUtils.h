#pragma once

#include "game/ObjectSlots.h"
#include <cstdint>

// handy debug toggles and overrides


#define SECOND_CITY (0)


#ifdef NDEBUG
//
// release builds
//
#define SKIP_INTRO (0)
constexpr uint64_t DEV_START_INVENTORY = 0;
constexpr uint32_t DEV_START_CREDITS = 0;
#else

//
// debug builds
// 

// skip the initial intro and start during planetary descent
#define SKIP_INTRO (0)

// start with additional inventory items, see ObjectSlots.h
constexpr uint64_t DEV_START_INVENTORY = 0;
	/*(1ULL << OBJ_PHOTON_EMITTER) | (1ULL << OBJ_ANTENNA) |
	(1ULL << OBJ_COBWEB) | (1ULL << OBJ_KITCHEN_SINK);*/


//override starting credits, in BCD. 0 = use default
constexpr uint32_t DEV_START_CREDITS = 0; /*0x1000000;*/

#endif


