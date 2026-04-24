// YM2149 PSG emulation -- generates at 250 kHz, downsamples to SDL output rate

#include "audio/Audio.h"
#include "audio/VolumeTable.h"

#include <cmath>
#include <cstdio>
#include <cstring>

constexpr int YM_SAMPLES_PER_FRAME = 5008;

struct YmState
{
	// 3D volume lookup: packed 15-bit index -> signed 16-bit sample
	int16_t volumeTable[32768];

	// Envelope wave tables: 16 shapes x 96 positions
	uint16_t envWaves[16][96];

	// Tone channels
	struct Tone
	{
		uint16_t counter;
		uint16_t output; // 0 or 0x1F
	};
	Tone tone[3];

	// Noise
	uint16_t noiseCounter;
	uint32_t noiseShift;
	uint16_t noiseOutput;
	bool noiseHalf;

	// Envelope
	uint16_t envCounter;
	uint32_t envPos;

	// Filters
	int32_t lpfY, lpfX1;
	int32_t hpfX1, hpfY1, hpfY0;

	// 250 kHz buffer + resampler
	int16_t buf250[YM_SAMPLES_PER_FRAME + 16];
	int buf250Size;
	uint32_t resampleFract;
	int resampleReadPos;
};

static YmState g_ym;
static bool g_ymInitialised = false;

static void BuildTables()
{
	if (g_ymInitialised)
	{
		return;
	}

	uint16_t interp[32][32][32];
	ym::YmBuildInterpolatedTable(interp);

	int16_t norm[32][32][32];
	ym::YmNormaliseTable(interp, norm);

	for (int c = 0; c < 32; c++)
	{
		for (int b = 0; b < 32; b++)
		{
			for (int a = 0; a < 32; a++)
			{
				g_ym.volumeTable[(c << ym::YM_SHIFT_C) | (b << ym::YM_SHIFT_B) |
								 a] = norm[c][b][a];
			}
		}
	}

	ym::YmBuildEnvelopeWaves(g_ym.envWaves);
	g_ymInitialised = true;
}

static uint16_t TonePeriod(const Audio &a, int ch)
{
	uint16_t p = ((a.regs[YM_TONE_A_COARSE + ch * 2] & 0x0F) << 8) |
				 a.regs[YM_TONE_A_FINE + ch * 2];
	return p ? p : 1;
}

static uint16_t NoisePeriod(const Audio &a)
{
	uint16_t p = a.regs[YM_NOISE_PERIOD] & 0x1F;
	return p ? p : 1;
}

static uint16_t EnvPeriod(const Audio &a)
{
	uint16_t p = (a.regs[YM_ENV_COARSE] << 8) | a.regs[YM_ENV_FINE];
	return p ? p : 1;
}

// 250 kHz generation
static void Generate250kHz(const Audio &a, int16_t *dest, int num)
{
	for (int n = 0; n < num; n++)
	{
		// Noise: advance at 125 kHz (every other 250 kHz tick)
		g_ym.noiseHalf = !g_ym.noiseHalf;
		if (!g_ym.noiseHalf)
		{
			g_ym.noiseCounter++;
		}
		uint16_t noisePer = NoisePeriod(a);
		if (g_ym.noiseCounter >= noisePer)
		{
			g_ym.noiseCounter = 0;
			if (g_ym.noiseShift & 1)
			{
				g_ym.noiseShift = (g_ym.noiseShift >> 1) ^ 0x12000;
				g_ym.noiseOutput = 0xFFFF;
			}
			else
			{
				g_ym.noiseShift >>= 1;
				g_ym.noiseOutput = 0;
			}
		}

		// Tone generators: advance at 250 kHz
		for (int ch = 0; ch < 3; ch++)
		{
			g_ym.tone[ch].counter++;
			uint16_t per = TonePeriod(a, ch);
			if (g_ym.tone[ch].counter >= per)
			{
				g_ym.tone[ch].counter = 0;
				g_ym.tone[ch].output ^= ym::YM_SQUARE_UP;
			}
		}

		// Envelope: advance at 250 kHz
		g_ym.envCounter++;
		uint16_t envPer = EnvPeriod(a);
		if (g_ym.envCounter >= envPer)
		{
			g_ym.envCounter = 0;
			g_ym.envPos++;
			if (g_ym.envPos >= ym::YM_ENV_TOTAL_STEPS)
			{
				g_ym.envPos -= 2 * ym::YM_ENV_STEPS_PER_BLOCK;
			}
		}

		// Mixer state (read live each sample)
		uint8_t mixer = a.regs[YM_MIXER];
		uint16_t mixTA = (mixer & 1) ? 0xFFFF : 0;
		uint16_t mixTB = (mixer & 2) ? 0xFFFF : 0;
		uint16_t mixTC = (mixer & 4) ? 0xFFFF : 0;
		uint16_t mixNA = (mixer & 8) ? 0xFFFF : 0;
		uint16_t mixNB = (mixer & 16) ? 0xFFFF : 0;
		uint16_t mixNC = (mixer & 32) ? 0xFFFF : 0;

		// Envelope volume (packed 3-channel, 5 bits each)
		uint16_t envShape = a.regs[YM_ENV_SHAPE] & 0x0F;
		uint16_t env3 = g_ym.envWaves[envShape][g_ym.envPos];

		// Per-channel envelope mask: bit 4 of amp reg selects envelope
		uint16_t envMask = 0;
		if (a.regs[YM_AMP_A] & 0x10)
		{
			envMask |= ym::YM_MASK_1VOICE;
		}
		if (a.regs[YM_AMP_B] & 0x10)
		{
			envMask |= ym::YM_MASK_1VOICE << ym::YM_SHIFT_B;
		}
		if (a.regs[YM_AMP_C] & 0x10)
		{
			envMask |= ym::YM_MASK_1VOICE << ym::YM_SHIFT_C;
		}
		env3 &= envMask;

		// Fixed volume (4-bit -> 5-bit mapped, packed)
		uint16_t vol3 =
			ym::YM_VOLUME_4TO5[a.regs[YM_AMP_A] & 0x0F] |
			(ym::YM_VOLUME_4TO5[a.regs[YM_AMP_B] & 0x0F] << ym::YM_SHIFT_B) |
			(ym::YM_VOLUME_4TO5[a.regs[YM_AMP_C] & 0x0F] << ym::YM_SHIFT_C);

		// Tone output: each channel is 0 or 0x1F
		uint32_t bt;
		uint16_t tone3;

		bt = (g_ym.tone[0].output | mixTA) & (g_ym.noiseOutput | mixNA);
		tone3 = bt & ym::YM_MASK_1VOICE;

		bt = (g_ym.tone[1].output | mixTB) & (g_ym.noiseOutput | mixNB);
		tone3 |= (bt & ym::YM_MASK_1VOICE) << ym::YM_SHIFT_B;

		bt = (g_ym.tone[2].output | mixTC) & (g_ym.noiseOutput | mixNC);
		tone3 |= (bt & ym::YM_MASK_1VOICE) << ym::YM_SHIFT_C;

		// Gate by volume (fixed OR envelope)
		tone3 &= (env3 | vol3);

		// 3D volume table lookup -> signed 16-bit sample
		int16_t sample = g_ym.volumeTable[tone3];

		// PWM alias low-pass filter (from Fuji)
		int32_t x0 = sample;
		if (x0 >= g_ym.lpfY)
		{
			g_ym.lpfY = x0;
		}
		else
		{
			g_ym.lpfY = (3 * (x0 + g_ym.lpfX1) + (g_ym.lpfY << 1)) >> 3;
		}
		g_ym.lpfX1 = x0;

		dest[n] = static_cast<int16_t>(g_ym.lpfY);
	}
}

constexpr int FRAC_BITS = 16;
constexpr uint32_t FRAC_ONE = 1u << FRAC_BITS;
constexpr uint32_t FRAC_MASK = FRAC_ONE - 1;
constexpr int32_t HPF_COEFF = 64;
constexpr int32_t HPF_SCALE = 32768;

static int16_t ApplyHPF(int16_t sample)
{
	int32_t x0 = sample;
	g_ym.hpfY1 += ((x0 - g_ym.hpfX1) * HPF_SCALE) - (g_ym.hpfY0 * HPF_COEFF);
	g_ym.hpfY0 = g_ym.hpfY1 / HPF_SCALE;
	g_ym.hpfX1 = x0;
	return static_cast<int16_t>(g_ym.hpfY0);
}

static void Downsample(int nOutputSamples, int nSampleRate, int16_t *pOut)
{
	uint32_t interval = static_cast<uint32_t>(
		(static_cast<uint64_t>(YM_TONE_CLOCK) * FRAC_ONE) / nSampleRate);

	for (int i = 0; i < nOutputSamples; i++)
	{
		int64_t total = 0;

		if (g_ym.resampleFract)
		{
			int idx = (g_ym.resampleReadPos < g_ym.buf250Size)
						  ? g_ym.resampleReadPos
						  : g_ym.buf250Size - 1;
			total += static_cast<int64_t>(g_ym.buf250[idx]) *
					 (FRAC_ONE - g_ym.resampleFract);
			g_ym.resampleReadPos++;
			g_ym.resampleFract -= FRAC_ONE;
		}

		g_ym.resampleFract += interval;

		while (g_ym.resampleFract & ~FRAC_MASK)
		{
			int idx = (g_ym.resampleReadPos < g_ym.buf250Size)
						  ? g_ym.resampleReadPos
						  : g_ym.buf250Size - 1;
			total += static_cast<int64_t>(g_ym.buf250[idx]) * FRAC_ONE;
			g_ym.resampleReadPos++;
			g_ym.resampleFract -= FRAC_ONE;
		}

		if (g_ym.resampleFract)
		{
			int idx = (g_ym.resampleReadPos < g_ym.buf250Size)
						  ? g_ym.resampleReadPos
						  : g_ym.buf250Size - 1;
			total +=
				static_cast<int64_t>(g_ym.buf250[idx]) * g_ym.resampleFract;
		}

		int16_t sample = static_cast<int16_t>(total / interval);
		sample = ApplyHPF(sample);
		pOut[i] = sample;
	}
}

static void AudioCallback(void *userdata, uint8_t *stream, int len)
{
	Audio &audio = *static_cast<Audio *>(userdata);
	int16_t *out = reinterpret_cast<int16_t *>(stream);
	int numSamples = len / static_cast<int>(sizeof(int16_t));

	if (!audio.active)
	{
		audio.pendingCount = 0;
		std::memset(out, 0, len);
		return;
	}

	// Space register writes evenly across the buffer so short tones aren't lost
	int num250Total = static_cast<int>(static_cast<int64_t>(numSamples) *
										   YM_TONE_CLOCK / audio.sampleRate +
									   2);
	if (num250Total > YM_SAMPLES_PER_FRAME)
	{
		num250Total = YM_SAMPLES_PER_FRAME;
	}

	int nPending = audio.pendingCount;
	if (nPending == 0)
	{
		Generate250kHz(audio, g_ym.buf250, num250Total);
	}
	else
	{
		int samplesPerSegment = num250Total / (nPending + 1);
		if (samplesPerSegment < 1)
		{
			samplesPerSegment = 1;
		}
		int pos = 0;

		for (int i = 0; i < nPending; i++)
		{
			// Consecutive writes to the same register: intermediate state
			// lasts ~4us on real hardware; 1 sample at 250kHz matches that
			int segLen = samplesPerSegment;
			if (i > 0 &&
				audio.pendingWrites[i].reg == audio.pendingWrites[i - 1].reg)
			{
				segLen = 1;
			}
			if (pos + segLen > num250Total)
			{
				segLen = num250Total - pos;
			}
			if (segLen > 0)
			{
				Generate250kHz(audio, g_ym.buf250 + pos, segLen);
				pos += segLen;
			}

			uint8_t reg = audio.pendingWrites[i].reg;
			uint8_t val = audio.pendingWrites[i].val;
			audio.regs[reg] = val;
			if (reg == YM_ENV_SHAPE)
			{
				g_ym.envCounter = 0;
				g_ym.envPos = 0;
			}
		}

		if (pos < num250Total)
		{
			Generate250kHz(audio, g_ym.buf250 + pos, num250Total - pos);
		}
	}
	audio.pendingCount = 0;

	g_ym.buf250Size = num250Total;
	g_ym.resampleReadPos = 0;

	Downsample(numSamples, audio.sampleRate, out);
}

static const uint8_t INIT_PRESET[13] = {
	0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x1F,
	0xFE, 0x00, 0x10, 0x00, 0x00, 0x00,
};

bool AudioInit(Audio &audio)
{
	std::memset(&audio, 0, sizeof(audio));
	std::memset(&g_ym, 0, sizeof(g_ym));
	g_ym.noiseShift = 1;
	audio.active = false;

	BuildTables();
	AudioLoadPreset(audio);
	audio.regs[YM_AMP_A] = 0;
	audio.regs[YM_AMP_B] = 0;
	audio.regs[YM_AMP_C] = 0;

	SDL_AudioSpec want;
	std::memset(&want, 0, sizeof(want));
	want.freq = 48000;
	want.format = AUDIO_S16SYS;
	want.channels = 1;
	want.samples = 512;
	want.callback = AudioCallback;
	want.userdata = &audio;

	SDL_AudioSpec have;
	audio.deviceId = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	if (audio.deviceId == 0)
	{
		std::fprintf(stderr, "SDL_OpenAudioDevice: %s\n", SDL_GetError());
		return false;
	}

	audio.sampleRate = have.freq;
	audio.initialised = true;

	SDL_PauseAudioDevice(audio.deviceId, 0);
	return true;
}

void AudioShutdown(Audio &audio)
{
	if (audio.deviceId != 0)
	{
		SDL_CloseAudioDevice(audio.deviceId);
		audio.deviceId = 0;
	}
	audio.initialised = false;
}

void AudioWriteReg(Audio &audio, uint8_t reg, uint8_t val)
{
	if (reg < YM_REG_COUNT)
	{
		SDL_LockAudioDevice(audio.deviceId);
		if (audio.pendingCount < MAX_PENDING_WRITES)
		{
			audio.pendingWrites[audio.pendingCount].reg = reg;
			audio.pendingWrites[audio.pendingCount].val = val;
			audio.pendingCount++;
		}
		audio.regs[reg] = val;
		SDL_UnlockAudioDevice(audio.deviceId);
	}
}

void AudioLoadPreset(Audio &audio)
{
	for (int i = 0; i < 13; i++)
	{
		audio.regs[i] = INIT_PRESET[i];
	}
}
