#pragma once

// DO NOT EDIT -- auto-generated

#include <cstdint>

namespace gen_s2
{

struct WorkspaceInit
{
	int32_t posX;
	int32_t posY;
	int32_t posZ;
	uint16_t roll;
	uint16_t pitch;
	uint16_t heading;
	int32_t renderMode;
	uint16_t flightState;
	uint16_t grounded;
	uint32_t thrust;
	uint32_t vertVelocity;
	uint32_t thrustAccum;
	uint16_t landingProx;
	uint8_t inputFlags;
	uint16_t horizonMirrorMask;
	uint16_t screenCenterX;
	uint16_t screenCenterY;
	uint8_t mixerShadow;
};

extern const WorkspaceInit WORKSPACE_INIT;

// Rendering attribute tables, 128 bytes each
constexpr int RENDER_ATTR_SIZE = 128;
extern const uint8_t RENDER_ATTR_TABLE[RENDER_ATTR_SIZE];
extern const uint8_t RENDER_ATTR_TABLE2[RENDER_ATTR_SIZE];

constexpr int OBJECT_SLOTS = 64;

// Slot table, 64 bytes
extern const uint8_t SLOT_TABLE[OBJECT_SLOTS];

// Flags table, 64 bytes
extern const uint8_t FLAGS_TABLE[OBJECT_SLOTS];

// Type/tile property table, 256 bytes
constexpr int TYPE_TABLE_SIZE = 256;
extern const uint8_t TYPE_TABLE[TYPE_TABLE_SIZE];

// World positions, 256 bytes each
extern const int32_t INIT_POS_X[OBJECT_SLOTS];
extern const int32_t INIT_POS_Y[OBJECT_SLOTS];
extern const int32_t INIT_POS_Z[OBJECT_SLOTS];

// Rotation selectors, 16 bytes
constexpr int ROT_SELECTOR_COUNT = 16;
extern const uint8_t ROTATION_SELECTOR[ROT_SELECTOR_COUNT];

// Rotation table, 160 bytes
constexpr int ROT_TABLE_SIZE = 80;
extern const int16_t ROT_TABLE[ROT_TABLE_SIZE];

} // namespace gen_s2
