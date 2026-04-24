#include "game/VblHandler.h"

#include "game/Game.h"

#include "audio/Audio.h"
#include "data/GameData.h"
#include "data/Gen_MData.h"
#include "game/Benson.h"
#include "game/Workspace.h"
#include "renderer/Hud.h"
#include "renderer/Palette.h"
#include "renderer/ScanlinePalette.h"

#include <SDL.h>
#include <cstring>

// sub_040DFC -- present front buffer to screen
// (VSync wait) + hardware display.  Used by the door wipe blocking
// loops to display frames without returning to main.cpp
static void PresentFrame(Game &game)
{
	const uint8_t *displayBuf = FrontBuffer(game);
	ResolveFramebufferScanline(displayBuf, FB_WIDTH, FB_HEIGHT, VIEWPORT_H,
							   game.scanlinePalette.lines, game.gameplayLut,
							   game.presentCtx.argbBuf);
	SDL_UpdateTexture(game.presentCtx.texture, NULL, game.presentCtx.argbBuf,
					  FB_WIDTH * static_cast<int>(sizeof(uint32_t)));
	SDL_SetRenderDrawColor(game.presentCtx.renderer, 0, 0, 0, 255);
	SDL_RenderClear(game.presentCtx.renderer);
	SDL_RenderCopy(game.presentCtx.renderer, game.presentCtx.texture, NULL,
				   NULL);
	SDL_RenderPresent(game.presentCtx.renderer);
}

// Convert a packed ST $0RGB longword pair to four ARGB values
static void StPairToArgb(uint32_t packed89, uint32_t packed1011, uint32_t &e8,
						 uint32_t &e9, uint32_t &e10, uint32_t &e11)
{
	auto convert = [](uint16_t st) -> uint32_t
	{
		return 0xFF000000u | (Expand3to8((st >> 8) & 7) << 16) |
			   (Expand3to8((st >> 4) & 7) << 8) | Expand3to8(st & 7);
	};
	e8 = convert(static_cast<uint16_t>(packed89 >> 16));
	e9 = convert(static_cast<uint16_t>(packed89));
	e10 = convert(static_cast<uint16_t>(packed1011 >> 16));
	e11 = convert(static_cast<uint16_t>(packed1011));
}

// Message drain (sub_043522 flag transitions, E1:3876-3878)

static void MessageDrain(Game &game)
{
	uint16_t &flag = g_workspace.cam.pendingMsgFlag;

	if (flag == 0x0001 && game.benson.state == BENSON_IDLE)
	{
		// Setup transition: start rendering.  E1:3861
		uint16_t idx = g_workspace.cam.pendingMsg;
		if (idx < gen_e4::MESSAGE_COUNT)
		{
			BensonDisplay(game.benson, gen_e4::MESSAGES[idx]);
			flag = 0xFFFF;
		}
		else
		{
			// Invalid index -- clear flag without rendering
			flag = 0x0000;
		}
	}
	else if (flag == 0xFFFF && game.benson.state == BENSON_IDLE)
	{
		// End-of-text cleanup: Benson returned to idle.  E1:3865
		flag = 0x0000;
	}
}

// sub_043034: elevator transition engine (sound + palette cascade per VBL)

static void ElevatorTransitionTick(Game &game)
{
	Camera &cam = g_workspace.cam;
	uint16_t phase = cam.elevatorPhase;

	// E1:3461-3467: compute table index from phase
	// D1 = (phase >> 5) & $F8.  If phase >= $42, set bit 2
	uint16_t idx = (phase >> 5) & 0xF8;
	if (phase >= 0x0042)
		idx |= 0x04;

	// E1:3470-3471: A0 = ELEVATOR_PARAMS + idx
	const uint8_t *entry = gen_e1::ELEVATOR_PARAMS + idx;

	// E1:3475-3476: tone = entry[3] << 4
	uint16_t tone = static_cast<uint16_t>(entry[3]) << 4;
	uint16_t toneB = tone;
	uint16_t toneC = static_cast<uint16_t>(tone + 0x0400);

	// E1:3478-3494: program YM channels B+C
	AudioWriteReg(game.audio, YM_TONE_B_FINE, static_cast<uint8_t>(toneB));
	AudioWriteReg(game.audio, YM_TONE_B_COARSE,
				  static_cast<uint8_t>(toneB >> 8));
	AudioWriteReg(game.audio, YM_TONE_C_FINE, static_cast<uint8_t>(toneC));
	AudioWriteReg(game.audio, YM_TONE_C_COARSE,
				  static_cast<uint8_t>(toneC >> 8));
	AudioWriteReg(game.audio, YM_MIXER, 0xF8);
	AudioWriteReg(game.audio, YM_AMP_B, 0x0B);
	AudioWriteReg(game.audio, YM_AMP_C, 0x0B);

	if (static_cast<int16_t>(cam.elevatorActive) >= 0)
	{
		// === Ascending / exiting path (E1:3495-3525) ===

		// E1:3495: ADD.B step to phase low byte
		uint8_t step = entry[2];
		uint8_t phaseLo = static_cast<uint8_t>(phase) + step;
		phase = (phase & 0xFF00) | phaseLo;

		// E1:3496-3497: if low byte < $84, just store phase
		if (phaseLo < 0x84)
		{
			cam.elevatorPhase = phase;
			// E1:3518-3521: if phase >= $0700, arm Timer B
			if (static_cast<int16_t>(phase) >= 0x0700)
			{
				cam.timerBScanline =
					static_cast<uint16_t>(static_cast<uint8_t>(phase) + 2);
			}
			return;
		}

		// E1:3498-3499: round up to next $x00
		phase = (phase | 0x00FF) + 1;

		// E1:3500-3501: if $0E00, elevator transition complete
		if (phase == 0x0E00)
		{
			// loc_04314A (E1:3528-3535): cleanup
			cam.elevatorActive = 0;
			cam.groundPal89 = game.palOverride89;
			cam.groundPal1011 = game.palOverride1011;
			AudioWriteReg(game.audio, YM_AMP_B, 0x00);
			AudioWriteReg(game.audio, YM_AMP_C, 0x00);
			return;
		}

		// E1:3502-3503: BLT (signed) -- if phase < $0700, just store
		if (static_cast<int16_t>(phase) < 0x0700)
		{
			cam.elevatorPhase = phase;
			return;
		}

		// E1:3504-3514: palette cascade at phase boundary
		// Read new colour from entry[16..17] (offset 16 = +$10)
		uint16_t newColour =
			static_cast<uint16_t>((entry[16] << 8) | entry[17]);
		uint32_t newPacked =
			(static_cast<uint32_t>(newColour) << 16) | newColour;

		// E1:3505-3506: write BASE to hardware (visible this frame)
		// The VBL handler already wrote TOP to gameplayLut; we overwrite
		// entries 8-11 with BASE so this frame's top half shows BASE
		{
			uint32_t e8, e9, e10, e11;
			StPairToArgb(cam.palBase89, cam.palBase1011, e8, e9, e10, e11);
			game.gameplayLut[8] = e8;
			game.gameplayLut[9] = e9;
			game.gameplayLut[10] = e10;
			game.gameplayLut[11] = e11;
		}

		// E1:3507-3514: shift shadows. TOP->BOTTOM, BASE->TOP, NEW->BASE
		cam.groundPal89 = game.palOverride89;
		cam.groundPal1011 = game.palOverride1011;
		game.palOverride89 = cam.palBase89;
		game.palOverride1011 = cam.palBase1011;
		cam.palBase89 = newPacked;
		cam.palBase1011 = newPacked;

		cam.elevatorPhase = phase;
		// E1:3520-3521: arm Timer B with phase low byte + 2
		cam.timerBScanline =
			static_cast<uint16_t>(static_cast<uint8_t>(phase) + 2);
	}
	else
	{
		// === Descending / entering path (E1:3559-3576) ===

		// E1:3559: SUB.B step from phase low byte
		uint8_t step = entry[2];
		uint8_t phaseLo = static_cast<uint8_t>(phase);
		bool borrow = (phaseLo < step);
		phaseLo -= step;
		phase = (phase & 0xFF00) | phaseLo;

		if (!borrow)
		{
			// E1:3560: BCC loc_043130 -- no borrow, store phase
			cam.elevatorPhase = phase;
			if (static_cast<int16_t>(phase) >= 0x0700)
			{
				cam.timerBScanline =
					static_cast<uint16_t>(static_cast<uint8_t>(phase) + 2);
			}
			return;
		}

		// E1:3561-3562: borrow -- back up one phase row
		phase -= 0x0100;
		phase = (phase & 0xFF00) | 0x0083;

		// E1:3563-3564: BLT -- phase < $0600 (signed), just store
		// when phase wraps past $0000 to $FFxx during late frames, the signed
		// comparison treats it as negative and skips the cascade
		if (static_cast<int16_t>(phase) < 0x0600)
		{
			cam.elevatorPhase = phase;
			if (static_cast<int16_t>(phase) >= 0x0700)
			{
				cam.timerBScanline =
					static_cast<uint16_t>(static_cast<uint8_t>(phase) + 2);
			}
			return;
		}

		// E1:3565-3576: palette cascade (reverse direction)
		// Read colour from entry[-24] (offset -24 = $FFE8)
		const uint8_t *prevEntry = entry - 24;
		uint16_t newColour =
			static_cast<uint16_t>((prevEntry[0] << 8) | prevEntry[1]);
		uint32_t newPacked =
			(static_cast<uint32_t>(newColour) << 16) | newColour;

		// E1:3566-3567: write BOTTOM to hardware (visible this frame)
		{
			uint32_t e8, e9, e10, e11;
			StPairToArgb(cam.groundPal89, cam.groundPal1011, e8, e9, e10, e11);
			game.gameplayLut[8] = e8;
			game.gameplayLut[9] = e9;
			game.gameplayLut[10] = e10;
			game.gameplayLut[11] = e11;
		}

		// E1:3568-3575: shift shadows. BOTTOM->TOP, BASE->BOTTOM, NEW->BASE
		game.palOverride89 = cam.groundPal89;
		game.palOverride1011 = cam.groundPal1011;
		cam.groundPal89 = cam.palBase89;
		cam.groundPal1011 = cam.palBase1011;
		cam.palBase89 = newPacked;
		cam.palBase1011 = newPacked;

		cam.elevatorPhase = phase;
		if (static_cast<int16_t>(phase) >= 0x0700)
		{
			cam.timerBScanline =
				static_cast<uint16_t>(static_cast<uint8_t>(phase) + 2);
		}
	}
}

// sub_041AEE (E1:1458-1522): door opening reveal
// Sweeps UP (132->4), 33 sweep + 21 hold frames
void DoorWipeReveal(Game &game)
{
	Camera &cam = g_workspace.cam;

	// E1:1459-1479: YM sound setup
	AudioWriteReg(game.audio, YM_TONE_C_FINE, 0x90);   // E1:1461
	AudioWriteReg(game.audio, YM_TONE_C_COARSE, 0x08); // E1:1464
	AudioWriteReg(game.audio, YM_TONE_B_FINE, 0x80);   // E1:1467
	AudioWriteReg(game.audio, YM_TONE_B_COARSE, 0x09); // E1:1470
	AudioWriteReg(game.audio, YM_AMP_C, 0x0C);		   // E1:1473
	AudioWriteReg(game.audio, YM_AMP_B, 0x0C);		   // E1:1476
	AudioWriteReg(game.audio, YM_MIXER, 0xF8);		   // E1:1479

	// E1:1480-1481: base -> bottom
	cam.groundPal89 = cam.palBase89;	 // E1:1480
	cam.groundPal1011 = cam.palBase1011; // E1:1481

	// E1:1482-1489: sweep loop.  D0 starts at $84 (132), step -4
	int16_t d0 = 0x0084; // E1:1482
	while (true)
	{
		cam.timerBScanline = static_cast<uint16_t>(d0); // E1:1485
		VBLHandler(game);								// sub_040DFC VSync
		PresentFrame(game);
		d0 -= 4;	 // E1:1487
		if (d0 == 0) // E1:1488 BEQ
			break;
		if (d0 > 0) // E1:1489 BPL
			continue;
		break; // negative: exit
	}

	// E1:1492-1494: cleanup
	cam.timerBScanline = 0;					  // E1:1492
	game.palOverride89 = cam.groundPal89;	  // E1:1493
	game.palOverride1011 = cam.groundPal1011; // E1:1494

	// E1:1497-1506: YM tone restore
	AudioWriteReg(game.audio, YM_TONE_B_FINE, 0x80);   // E1:1497
	AudioWriteReg(game.audio, YM_TONE_B_COARSE, 0x07); // E1:1500
	AudioWriteReg(game.audio, YM_TONE_C_FINE, 0x80);   // E1:1503
	AudioWriteReg(game.audio, YM_TONE_C_COARSE, 0x06); // E1:1506

	// E1:1507-1511: 21-frame hold
	// MOVEQ #20,D0 -> DBF loop = 21 iterations
	for (int i = 0; i <= 20; i++) // E1:1507
	{
		VBLHandler(game); // E1:1509
		PresentFrame(game);
	}

	// E1:1514-1521: silence and clear
	AudioWriteReg(game.audio, YM_AMP_B, 0x00);					 // E1:1514
	AudioWriteReg(game.audio, YM_AMP_C, 0x00);					 // E1:1517
	AudioWriteReg(game.audio, YM_MIXER, game.intro.mixerShadow); // E1:1520
	cam.doorWipeDir = 0;										 // E1:1521
}

// sub_041C64 (E1:1525-1594): door closing hide
// Sweeps DOWN (2->130), 25 wait + 33 sweep frames
void DoorWipeHide(Game &game)
{
	Camera &cam = g_workspace.cam;

	// E1:1526-1549: YM sound setup
	AudioWriteReg(game.audio, YM_TONE_C_FINE, 0x90);   // E1:1528
	AudioWriteReg(game.audio, YM_TONE_C_COARSE, 0x08); // E1:1531
	AudioWriteReg(game.audio, YM_TONE_B_FINE, 0x80);   // E1:1534
	AudioWriteReg(game.audio, YM_TONE_B_COARSE, 0x09); // E1:1537
	AudioWriteReg(game.audio, YM_AMP_B, 0x0C);		   // E1:1540
	AudioWriteReg(game.audio, YM_AMP_C, 0x00);		   // E1:1543
	AudioWriteReg(game.audio, YM_AMP_C, 0x0C);		   // E1:1546
	AudioWriteReg(game.audio, YM_MIXER, 0xF8);		   // E1:1549

	// E1:1550-1554: 25-frame wait
	// MOVEQ #24,D0 -> DBF loop = 25 iterations
	for (int i = 0; i <= 24; i++) // E1:1550
	{
		VBLHandler(game); // E1:1553
		PresentFrame(game);
	}

	// E1:1555-1566: YM tone restore
	AudioWriteReg(game.audio, YM_TONE_C_FINE, 0x80);   // E1:1557
	AudioWriteReg(game.audio, YM_TONE_C_COARSE, 0x07); // E1:1560
	AudioWriteReg(game.audio, YM_TONE_B_FINE, 0x80);   // E1:1563
	AudioWriteReg(game.audio, YM_TONE_B_COARSE, 0x06); // E1:1566

	// E1:1567-1572: palette setup for sweep
	cam.groundPal89 = game.palOverride89;	  // E1:1567
	cam.groundPal1011 = game.palOverride1011; // E1:1568
	// palBase1011 high word (entry 10) -> all 4 entries of palOverride
	uint16_t baseHi = static_cast<uint16_t>(cam.palBase1011 >> 16); // E1:1569
	uint32_t uniform = (static_cast<uint32_t>(baseHi) << 16) | baseHi;
	game.palOverride89 = uniform;	// E1:1569-1570
	game.palOverride1011 = uniform; // E1:1571-1572

	// E1:1573-1580: sweep loop.  D0 starts at $02, step +4
	int16_t d0 = 0x0002; // E1:1573
	while (d0 < 0x0086)	 // E1:1579 CMPI #$86; BLT
	{
		cam.timerBScanline = static_cast<uint16_t>(d0); // E1:1576
		VBLHandler(game);								// E1:1577
		PresentFrame(game);
		d0 += 4; // E1:1578
	}

	// E1:1581-1583: cleanup
	cam.timerBScanline = 0;					// E1:1581
	game.palOverride89 = cam.palBase89;		// E1:1582
	game.palOverride1011 = cam.palBase1011; // E1:1583

	// E1:1586-1593: silence and clear
	AudioWriteReg(game.audio, YM_AMP_C, 0x00);					 // E1:1586
	AudioWriteReg(game.audio, YM_AMP_B, 0x00);					 // E1:1589
	AudioWriteReg(game.audio, YM_MIXER, game.intro.mixerShadow); // E1:1592
	cam.doorWipeDir = 0;										 // E1:1593
}

void VBLHandler(Game &game)
{
	// $043302-$04330C: MOVE.L ($623F2),($FF8250); MOVE.L ($623F6),($FF8254)
	// Write "top" palette entries 8-11 from the override variables
	{
		uint32_t e8, e9, e10, e11;
		StPairToArgb(game.palOverride89, game.palOverride1011, e8, e9, e10,
					 e11);
		game.gameplayLut[8] = e8;
		game.gameplayLut[9] = e9;
		game.gameplayLut[10] = e10;
		game.gameplayLut[11] = e11;
	}
	game.activeLut = game.gameplayLut;

	// loc_04334A (E1:3649-3668): palette 3 flash when attackFlag active
	if (game.scriptVM.attackFlag != 0)
	{
		if (g_workspace.cam.renderMode == 0)
		{
			// Outdoor: animate through PAL3_FLASH_TABLE
			uint16_t stCol = gen_e1::PAL3_FLASH_TABLE[game.pal3FlashIndex];
			game.pal3FlashIndex =
				(game.pal3FlashIndex + 1) % gen_e1::PAL3_FLASH_TABLE_SIZE;
			uint8_t r3 = (stCol >> 8) & 7;
			uint8_t g3 = (stCol >> 4) & 7;
			uint8_t b3 = stCol & 7;
			game.gameplayLut[3] = 0xFF000000u | (Expand3to8(r3) << 16) |
								  (Expand3to8(g3) << 8) | Expand3to8(b3);
		}
		else
		{
			// Indoor: fixed $0110
			game.gameplayLut[3] = 0xFF000000u | (Expand3to8(1) << 16) |
								  (Expand3to8(1) << 8) | Expand3to8(0);
		}
	}

	uint16_t armedScanline = 0;

	// E1:3635-3638: elevator transition engine dispatch
	// When elevatorActive != 0, run sub_043034 INSTEAD of normal Timer B
	// arming
	if (g_workspace.cam.elevatorActive != 0)
	{
		// Save timerBScanline before the call.  ElevatorTransitionTick
		// only overwrites it on paths that reach loc_0432D2 (the Timer
		// B arm entry point).  If it doesn't change, Timer B was NOT
		// armed this frame
		g_workspace.cam.timerBScanline = 0;
		ElevatorTransitionTick(game);
		armedScanline = g_workspace.cam.timerBScanline;
	}
	else
	{
		// E1:3641: sub_0432CA -- re-arm Timer B from stored $0624E4
		// The stored value persists across frames; sub_0432CA reads
		// it and arms the MFP each VBL.  When the value is 0 (e.g
		// indoor with no outdoor split), Timer B is not armed
		armedScanline = g_workspace.cam.timerBScanline;

		// E1:3642-3644: sub_043252 -- per-VBL transition cooldown
		if (g_workspace.cam.transCooldown > 0 &&
			g_workspace.cam.doorWipeDir == 0)
		{
			// E1:3582: SUBQ.W #1,$06248E
			g_workspace.cam.transCooldown--;
			// E1:3583-3586: palette entry 10 from TRANS_PALETTE_TABLE
			uint16_t palWord =
				gen_e1::TRANS_PALETTE_TABLE[g_workspace.cam.transCooldown];
			uint8_t rr = (palWord >> 8) & 7;
			uint8_t gg = (palWord >> 4) & 7;
			uint8_t bb = palWord & 7;
			game.gameplayLut[10] = 0xFF000000u | (Expand3to8(rr) << 16) |
								   (Expand3to8(gg) << 8) | Expand3to8(bb);
			uint8_t vol =
				gen_e1::TRANS_VOLUME_TABLE[g_workspace.cam.transCooldown];
			AudioWriteReg(game.audio, YM_TONE_B_FINE, 0x80);   // E1:3590
			AudioWriteReg(game.audio, YM_TONE_B_COARSE, 0x01); // E1:3592
			AudioWriteReg(game.audio, YM_TONE_C_FINE, 0x90);   // E1:3594
			AudioWriteReg(game.audio, YM_TONE_C_COARSE, 0x01); // E1:3596
			AudioWriteReg(game.audio, YM_AMP_B, vol);		   // E1:3598
			AudioWriteReg(game.audio, YM_AMP_C, vol);		   // E1:3600
			AudioWriteReg(game.audio, YM_MIXER, 0xF8);		   // E1:3588
		}
	}

	// Silence channels B+C on elevatorActive 1->0 transition
	if (game.prevElevatorActive != 0 && g_workspace.cam.elevatorActive == 0)
	{
		AudioWriteReg(game.audio, YM_AMP_B, 0x00);
		AudioWriteReg(game.audio, YM_AMP_C, 0x00);
	}
	game.prevElevatorActive = g_workspace.cam.elevatorActive;

	// Build per-scanline palette for this frame
	// Reset all lines to the current gameplayLut (including entries 0-7, 12-15,
	// and the top-half values for 8-11 just written above)
	game.scanlinePalette.Reset(game.gameplayLut);

	// Apply Timer B raster split for entries 8-11
	if (armedScanline > 0 && armedScanline < VIEWPORT_H)
	{
		uint32_t be8, be9, be10, be11;
		StPairToArgb(g_workspace.cam.groundPal89, g_workspace.cam.groundPal1011,
					 be8, be9, be10, be11);

		for (int y = armedScanline; y < VIEWPORT_H; y++)
		{
			game.scanlinePalette.lines[y][8] = be8;
			game.scanlinePalette.lines[y][9] = be9;
			game.scanlinePalette.lines[y][10] = be10;
			game.scanlinePalette.lines[y][11] = be11;
		}
	}

	// sub_041AA0 (E1:1424-1441): damage palette scramble
	if (game.damageFlashTimer > 0)
	{
		game.damageFlashTimer--;
		uint16_t ptr = game.damageFlashPtr;
		for (int y = 0; y < VIEWPORT_H; y++)
		{
			// Run several iterations per scanline (original does ~480
			// iterations per scanline at 65535 total / 136 lines)
			// We run one iteration per scanline for the visual output

			// E1:1430: ROR.W (A0)
			uint16_t idx =
				(ptr >> 1) & 0x07FF; // word index into 2048-word buffer
			game.randomBuf[idx] = static_cast<uint16_t>(
				(game.randomBuf[idx] >> 1) | (game.randomBuf[idx] << 15));

			// E1:1431: MOVE.L (A0)+,D1 -- read two words
			uint16_t w0 = game.randomBuf[idx];
			uint16_t w1 = game.randomBuf[(idx + 1) & 0x07FF];

			// E1:1432-1435: advance and wrap pointer
			ptr = static_cast<uint16_t>(((ptr + 4) & 0x0FFF) | 0x7000);

			// Convert ST $0RGB words to ARGB
			// Original writes same D1 (two words) to both $FF8250 and $FF8254,
			// so regs 8-9 get w0,w1 and regs 10-11 get the same w0,w1
			auto stToArgb = [](uint16_t st) -> uint32_t
			{
				return 0xFF000000u | (Expand3to8((st >> 8) & 7) << 16) |
					   (Expand3to8((st >> 4) & 7) << 8) | Expand3to8(st & 7);
			};
			uint32_t c0 = stToArgb(w0);
			uint32_t c1 = stToArgb(w1);
			game.scanlinePalette.SetLine8to11(y, c0, c1, c0, c1);
		}
		game.damageFlashPtr = ptr;
	}

	// $043316: ADDQ.W #1,($62472)
	game.scriptVM.frameCounter++;

	// sub_043710: engine sound (channel C, velocity-driven)
	// E1:4009-4058
	// Only runs when elevatorActive == 0 (elevator uses channel C too)
	if (game.blackoutTimer <= 0 && g_workspace.cam.elevatorActive == 0)
	{
		uint16_t altWord = static_cast<uint16_t>(
			static_cast<uint32_t>(g_workspace.cam.posY) >> 16);
		if (altWord <= 0x7F)
		{
			int32_t vel = static_cast<int32_t>(g_workspace.cam.vertVelocity);
			if (vel >= 0)
			{
				// E1:4041-4051: positive velocity -- engine sound plays
				uint8_t vol = static_cast<uint8_t>(
					(g_workspace.cam.vertVelocity >> 16) & 0xFF);

				AudioWriteReg(game.audio, YM_TONE_C_FINE, 0x10); // E1:4042
				g_workspace.cam.soundLock = 0xFFFF;				 // E1:4044
				AudioWriteReg(game.audio, YM_MIXER,
							  game.intro.mixerShadow);			   // E1:4045
				AudioWriteReg(game.audio, YM_AMP_C, vol);		   // E1:4047
				AudioWriteReg(game.audio, YM_TONE_C_COARSE, 0x00); // E1:4049
			}
			else
			{
				// E1:4040: BMI loc_0437BC -- negative velocity
				// E1:4054-4058: if soundLock set, silence C and clear
				if (g_workspace.cam.soundLock != 0)
				{
					AudioWriteReg(game.audio, YM_AMP_C, 0x00);
					g_workspace.cam.soundLock = 0;
				}
			}
		}
	}

	// sub_043710 / sub_043A0E / sub_043B54: HUD instruments
	// In the original, VBL pixel writers use $062340 XOR $010000
	// to write to the OPPOSITE buffer (E1:3935-3936, 4348-4349,
	// 4470-4471)
	HudRenderInstruments(VBLTarget(game), g_workspace.cam);

	MessageDrain(game);
	uint8_t *curTarget = VBLTarget(game);
	uint8_t *prevTarget = game.prevBensonBuf ? game.prevBensonBuf : curTarget;
	BensonTick(game.benson, curTarget, prevTarget, &game.audio);
	game.prevBensonBuf = curTarget;

	// loc_0436AC: frame 24 double-buffer sync for Benson text strip
	if (game.benson.frameCount == 24)
	{
		uint8_t *src = prevTarget;
		uint8_t *dst = (prevTarget == game.indexBuf[0]) ? game.indexBuf[1]
														: game.indexBuf[0];
		for (int row = 0; row < BENSON_CHAR_H; row++)
		{
			int y = BENSON_TEXT_Y + row;
			std::memcpy(dst + y * FB_WIDTH + BENSON_TEXT_X,
						src + y * FB_WIDTH + BENSON_TEXT_X, BENSON_TEXT_W);
		}
	}
}

void InitGameplayPalette(Game &game)
{
	uint32_t e8, e9, e10, e11;
	StPairToArgb(game.palOverride89, game.palOverride1011, e8, e9, e10, e11);
	game.gameplayLut[8] = e8;
	game.gameplayLut[9] = e9;
	game.gameplayLut[10] = e10;
	game.gameplayLut[11] = e11;
	game.activeLut = game.gameplayLut;
	game.scanlinePalette.Reset(game.gameplayLut);
}
