// Endgame starfield -- $042240 endgame path

#include "game/Outro.h"

#include "audio/Audio.h"
#include "data/Gen_MData.h"

#include <cstring>

constexpr uint8_t VIEWPORT_FILL = 8;

// Engine sound (sub_0424E0)
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

void OutroInit(OutroState &state, ScriptVM &vm, Benson &benson,
			   uint8_t *indexBuf)
{
	state.mixerShadow = 0xFE;

	BensonInit(benson);

	std::memset(indexBuf, VIEWPORT_FILL, FB_WIDTH * VIEWPORT_H);
	std::memcpy(indexBuf + FB_WIDTH * VIEWPORT_H, gen_m::HUD_BITMAP,
				FB_WIDTH * HUD_H);

	StarfieldInit(state.starfield, indexBuf);
}

void OutroTick(OutroState &state, ScriptVM &vm, Benson &benson,
			   uint8_t *indexBuf, Audio *audio)
{
	// $042240 endgame loop: starfield + script VM + engine sound
	vm.frameCounter++;

	StarfieldAdvanceCamera(state.starfield);
	StarfieldDraw(state.starfield, indexBuf);

	if (audio != nullptr)
	{
		WriteEngineSoundRegs(*audio, state.starfield);
	}

	BensonTick(benson, indexBuf, indexBuf, audio);
	ScriptVMTick(vm, benson);
}
