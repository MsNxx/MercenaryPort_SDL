#pragma once
// DO NOT EDIT -- auto-generated

#include <cstdint>

namespace gen_m
{

// HUD panel bitmap, 320x64 pixels
constexpr int HUD_WIDTH = 320;
constexpr int HUD_HEIGHT = 64;
extern const uint8_t HUD_BITMAP[20480];

// Digit font, 13 glyphs x 8 bytes
constexpr int DIGIT_GLYPH_COUNT = 13;
constexpr int DIGIT_GLYPH_WIDTH = 8;
constexpr int DIGIT_GLYPH_HEIGHT = 5;
constexpr int DIGIT_GLYPH_STRIDE = 8;
extern const uint8_t DIGIT_FONT[104];

// Benson font, 64 glyphs x 8 bytes
constexpr int BENSON_GLYPH_COUNT = 64;
constexpr int BENSON_GLYPH_WIDTH = 8;
constexpr int BENSON_GLYPH_HEIGHT = 7;
constexpr int BENSON_GLYPH_STRIDE = 8;
extern const uint8_t BENSON_FONT[512];

// Compass tape, 592 frames x 10 bytes
constexpr int COMPASS_FRAME_COUNT = 592;
constexpr int COMPASS_FRAME_BYTES = 10;
extern const uint8_t COMPASS_TAPE[5920];

// Elevation strip, 2432 bytes
constexpr int ELEV_STRIP_BYTES = 2432;
extern const uint8_t ELEVATION_STRIP[ELEV_STRIP_BYTES];

// Antilog table, 8192 bytes
constexpr int ANTILOG_WORDS = 4096;
extern const uint16_t ANTILOG_TABLE[ANTILOG_WORDS];

// Log table (full signed range), 16386 bytes
// Index with: LOG_TABLE_FULL[LOG_A5_OFFSET + (int16_t)(mantissa) / 2]
constexpr int LOG_FULL_WORDS = 8193;
constexpr int LOG_A5_OFFSET = 4096;
extern const uint16_t LOG_TABLE_FULL[LOG_FULL_WORDS];

} // namespace gen_m
