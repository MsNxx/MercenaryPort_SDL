#pragma once

// Log-float arithmetic using the game's lookup tables from M
//
// Format: high word = exponent, low word = mantissa ($1000-$1FFF)

#include <cstdint>

constexpr uint32_t LOG_FLOAT_ZERO = 0xFFF01000;

uint16_t LogLookup(uint16_t mantissa);
uint16_t AntiLogLookup(uint16_t logVal);
uint32_t LogMultiply(uint32_t a, uint32_t b);
uint32_t LogDivide(uint32_t a, uint32_t b);
uint32_t LogFloatAdd(uint32_t a, uint32_t b);
uint32_t IntToLogFloat(int32_t val);

// Initial exponent 31, full 32-bit range -- used by road drawer
uint32_t IntToLogFloat31(int32_t val);

int32_t LogFloatToScreen(uint32_t val);

// Clamps overflow to 16-bit range
int32_t LogFloatToInt(uint32_t logVal);

// Debug only
float LogFloatToLinear(uint32_t val);
