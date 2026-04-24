#pragma once
// DO NOT EDIT -- auto-generated

#include <cstdint>

namespace gen_e1
{

// Initial/title palette (32 bytes)
extern const uint16_t INITIAL_PALETTE[16];
// Gameplay palette (32 bytes)
extern const uint16_t GAMEPLAY_PALETTE[16];

// Digit BCD lookup table (200 bytes)
extern const uint8_t DIGIT_BCD_TABLE[200];

// Compass direction tick mark table (128 bytes)
extern const uint8_t COMPASS_TICK_TABLE[128];

// Palette 3 flash animation table (32 bytes)
constexpr int PAL3_FLASH_TABLE_SIZE = 16;
extern const uint16_t PAL3_FLASH_TABLE[PAL3_FLASH_TABLE_SIZE];

// Thrust lookup table (256 bytes)
extern const uint8_t THRUST_TABLE[256];

// Room edge connectivity template (24 bytes)
extern const uint8_t ROOM_EDGE_TEMPLATE[24];

// Interior geometry templates (162 bytes)
constexpr int INTERIOR_TEMPLATES_SIZE = 162;
extern const uint8_t INTERIOR_TEMPLATES[INTERIOR_TEMPLATES_SIZE];

// Template index table (16 bytes)
extern const uint8_t TEMPLATE_INDEX[16];

// Room transition table (48 bytes)
extern const uint8_t ROOM_TRANSITIONS[48];

// Room transition palette table (160 bytes)
extern const uint16_t TRANS_PALETTE_TABLE[80];

// Room transition volume table (80 bytes)
extern const uint8_t TRANS_VOLUME_TABLE[80];

// Elevator parameter table (128 bytes)
extern const uint8_t ELEVATOR_PARAMS[128];

} // namespace gen_e1
