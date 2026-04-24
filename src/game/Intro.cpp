#include "game/Intro.h"

#include "audio/Audio.h"
#include "data/Gen_MData.h"

#include <cstring>

constexpr uint8_t VIEWPORT_FILL = 8;
#include "renderer/FrameBuffer.h"

constexpr uint32_t INIT_DELAY_FRAMES = 100;

// Engine / crash descent sound (sub_0424E0, sub_0425C0)
// Writes tone B+C from freqCounter/freqCounter2, fixed volume 9

static void WriteEngineSoundRegs(Audio &audio, const Starfield &sf)
{
	AudioWriteReg(audio, YM_MIXER, 0xF8);
	AudioWriteReg(audio, YM_TONE_B_FINE,
				  static_cast<uint8_t>(sf.freqCounter & 0xFF));
	AudioWriteReg(audio, YM_TONE_B_COARSE,
				  static_cast<uint8_t>((sf.freqCounter >> 8) & 0x0F));
	AudioWriteReg(audio, YM_TONE_C_FINE,
				  static_cast<uint8_t>(sf.freqCounter2 & 0xFF));
	AudioWriteReg(audio, YM_TONE_C_COARSE,
				  static_cast<uint8_t>((sf.freqCounter2 >> 8) & 0x0F));
	AudioWriteReg(audio, YM_AMP_B, 0x09);
	AudioWriteReg(audio, YM_AMP_C, 0x09);
}

// loc_0426A0: silence channels B+C
static void SilenceEngineRegs(Audio &audio)
{
	AudioWriteReg(audio, YM_AMP_B, 0x00);
	AudioWriteReg(audio, YM_AMP_C, 0x00);
}

// Planet approach sound ($041EF8)
// Noise on channel B with envelope reset, runs each disc loop iteration

static void WritePlanetSoundRegs(Audio &audio, uint8_t &mixerShadow)
{
	mixerShadow |= 0x02;  // BSET #1: disable tone B
	mixerShadow &= ~0x10; // BCLR #4: enable noise B

	AudioWriteReg(audio, YM_NOISE_PERIOD, 0x01);
	AudioWriteReg(audio, YM_MIXER, mixerShadow);
	AudioWriteReg(audio, YM_AMP_B, 0x10);
	AudioWriteReg(audio, YM_ENV_FINE, 0xFF);
	AudioWriteReg(audio, YM_ENV_COARSE, 0xFF);
	AudioWriteReg(audio, YM_ENV_SHAPE, 0x00);
}

void IntroInit(IntroState &state, ScriptVM &vm, Benson &benson,
			   uint8_t *indexBuf)
{
	state.phase = INTRO_PHASE_INIT_DELAY;
	state.frameCount = 0;
	state.mixerShadow = 0xFE; // from init preset dat_040E28

	BensonInit(benson);

	std::memset(indexBuf, VIEWPORT_FILL, FB_WIDTH * VIEWPORT_H);
	std::memcpy(indexBuf + FB_WIDTH * VIEWPORT_H, gen_m::HUD_BITMAP,
				FB_WIDTH * HUD_H);

	StarfieldInit(state.starfield, indexBuf);
}

bool IntroTick(IntroState &state, ScriptVM &vm, Benson &benson,
			   uint8_t *indexBuf, Audio *audio)
{
	// $043316: VBL handler increments $62472 every VBL (50 Hz)
	vm.frameCounter++;

	switch (state.phase)
	{
	case INTRO_PHASE_INIT_DELAY:
	{
		state.frameCount++;
		if (state.frameCount >= INIT_DELAY_FRAMES)
		{
			state.phase = INTRO_PHASE_1;
		}
		break;
	}

	case INTRO_PHASE_1:
	{
		// $0424AE: no starfield draw, no camera advance, no sound
		BensonTick(benson, indexBuf, indexBuf, audio);
		ScriptVMTick(vm, benson);

		if (vm.scriptRunning != 0)
		{
			state.phase = INTRO_PHASE_2;
		}
		break;
	}

	case INTRO_PHASE_2:
	{
		// $0424BE: sub_0424E0 runs first.  It checks if
		// freqCounter == $0400 and returns early (no camera
		// advance, no YM writes) if so
		bool cameraActive = StarfieldAdvanceCamera(state.starfield);
		StarfieldDraw(state.starfield, indexBuf);

		// Engine sound: only when sub_0424E0 didn't return early
		if (audio != nullptr && cameraActive)
		{
			WriteEngineSoundRegs(*audio, state.starfield);
			state.mixerShadow = 0xF8; // track what we wrote
		}

		BensonTick(benson, indexBuf, indexBuf, audio);
		ScriptVMTick(vm, benson);

		int16_t sr = static_cast<int16_t>(vm.scriptRunning);
		if (sr < 0)
		{
			state.phase = INTRO_PHASE_3;
		}
		break;
	}

	case INTRO_PHASE_3:
	{
		// $0424D6: sub_0425C0 runs.  Same guard pattern:
		// if freqCounter == $0700, silence B+C and return Z=1
		// Otherwise: YM writes + camera reverse + return Z=0
		bool crashDone = StarfieldCrashDescent(state.starfield);
		if (!crashDone)
		{
			StarfieldDraw(state.starfield, indexBuf);

			// Crash descent sound: same registers as engine
			if (audio != nullptr)
			{
				WriteEngineSoundRegs(*audio, state.starfield);
			}
		}
		else
		{
			// loc_0426A0: silence B+C when crash complete
			if (audio != nullptr)
			{
				SilenceEngineRegs(*audio);
			}
		}

		BensonTick(benson, indexBuf, indexBuf, audio);
		ScriptVMTick(vm, benson);

		if (crashDone)
		{
			PlanetDiscInit(state.planetDisc);
			state.phase = INTRO_PHASE_4;
		}
		break;
	}

	case INTRO_PHASE_4:
	{
		constexpr int VBL_CYCLES = 20000;
		constexpr int OUTER_OVERHEAD = 690;
		int cyclesRemaining = VBL_CYCLES;
		bool discDone = false;

		while (cyclesRemaining > 0 && !discDone)
		{
			discDone = PlanetDiscTick(state.planetDisc, indexBuf);

			// Planet approach sound: runs every disc loop iteration
			if (audio != nullptr && (vm.scriptRunning & 0xFF) != 0)
			{
				WritePlanetSoundRegs(*audio, state.mixerShadow);
			}

			int hw = state.planetDisc.lastHalfWidth;
			int insidePairs = (hw > 0) ? (hw * 100 / 235) : 0;
			if (insidePairs > 68)
			{
				insidePairs = 68;
			}
			int outsidePairs = 68 - insidePairs;
			int avgFillGroups = (hw > 16) ? (hw / 16) : 0;
			int iterCycles = outsidePairs * 210 +
							 insidePairs * (630 + 96 * avgFillGroups) +
							 OUTER_OVERHEAD;
			cyclesRemaining -= iterCycles;
		}

		BensonTick(benson, indexBuf, indexBuf, audio);
		ScriptVMTick(vm, benson);

		if (discDone)
		{
			return false;
		}
		break;
	}
	}

	return true;
}
