#pragma once

// YM2149 PSG emulation via SDL2 audio

#include <SDL.h>
#include <cstdint>

// YM2149 register indices
constexpr uint8_t YM_TONE_A_FINE = 0;
constexpr uint8_t YM_TONE_A_COARSE = 1;
constexpr uint8_t YM_TONE_B_FINE = 2;
constexpr uint8_t YM_TONE_B_COARSE = 3;
constexpr uint8_t YM_TONE_C_FINE = 4;
constexpr uint8_t YM_TONE_C_COARSE = 5;
constexpr uint8_t YM_NOISE_PERIOD = 6;
constexpr uint8_t YM_MIXER = 7;
constexpr uint8_t YM_AMP_A = 8;
constexpr uint8_t YM_AMP_B = 9;
constexpr uint8_t YM_AMP_C = 10;
constexpr uint8_t YM_ENV_FINE = 11;
constexpr uint8_t YM_ENV_COARSE = 12;
constexpr uint8_t YM_ENV_SHAPE = 13;
constexpr uint8_t YM_REG_COUNT = 14;

// YM2149 clock constants
constexpr uint32_t YM_MASTER_CLOCK = 2000000;			// 2 MHz
constexpr uint32_t YM_TONE_CLOCK = YM_MASTER_CLOCK / 8; // 250 kHz

constexpr int MAX_PENDING_WRITES = 64;

struct PendingWrite
{
	uint8_t reg;
	uint8_t val;
};

struct Audio
{
	uint8_t regs[YM_REG_COUNT];

	// Queued writes, applied by the audio callback at buffer boundaries
	PendingWrite pendingWrites[MAX_PENDING_WRITES];
	int pendingCount;

	SDL_AudioDeviceID deviceId;
	int sampleRate;
	bool initialised;
	bool active;
};

bool AudioInit(Audio &audio);
void AudioShutdown(Audio &audio);
void AudioWriteReg(Audio &audio, uint8_t reg, uint8_t val);
void AudioLoadPreset(Audio &audio);
