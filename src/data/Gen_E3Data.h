#pragma once

// DO NOT EDIT -- auto-generated

#include <cstdint>

namespace gen_e3
{

// Road line segment
struct RoadLine
{
	uint8_t ax, az;
	uint8_t bx, bz;
};

constexpr int ROAD_LINE_COUNT = 32;
extern const RoadLine ROAD_LINES[ROAD_LINE_COUNT];

// Spatial index, 16x16 tiles, up to 2 line indices per tile
struct TileEntry
{
	int8_t line1;
	int8_t line2;
};

extern const TileEntry TILE_INDEX[16][16];

// Altitude-to-D7 lookup, 256 bytes
constexpr int ALT_TABLE_SIZE = 256;
extern const uint8_t ALT_TO_D7[ALT_TABLE_SIZE];

inline int32_t GridToWorld(int grid) { return grid * 0x10000 + 0x8000; }

// Raw E3 binary, 32256 bytes
constexpr int E3_SIZE = 32256;
extern const uint8_t E3_RAW[E3_SIZE];

constexpr int ROOM_COUNT = 192;
extern const uint16_t ROOM_OFFSETS[ROOM_COUNT];

constexpr uint16_t ROOM_DATA_BASE_OFFSET = 0x0180;
constexpr int ROOM_DATA_SIZE = 3712;
extern uint8_t ROOM_DATA[ROOM_DATA_SIZE];

} // namespace gen_e3
