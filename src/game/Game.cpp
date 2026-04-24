#include "game/Game.h"
#include "data/GameData.h"
#include "game/Combat.h"
#include "game/Controls.h"
#include "game/VblHandler.h"

#include "game/ObjectSlots.h"

#include "data/Gen_MData.h"
#include "game/Benson.h"
#include "game/Interior.h"
#include "game/TileDetail.h"
#include "game/Workspace.h"
#include "renderer/Fill.h"
#include "renderer/Hud.h"
#include "renderer/Roads.h"

#include <SDL.h>
#include <cstring>

// Mode transitions

static void EnterMode(Game &game, GameMode mode)
{
	game.mode = mode;
	game.frameCount = 0;

	if (mode == MODE_INTRO)
	{
		IntroInit(game.intro, game.scriptVM, game.benson, DrawTarget(game));
	}
	else if (mode == MODE_GAMEPLAY)
	{
		// Intro->gameplay handover (loc_0488DE)
		{
			g_workspace.frameBuf[0] = game.indexBuf[0];
			g_workspace.frameBuf[1] = game.indexBuf[1];

			const gen_e2::WorkspaceInit &ws = gen_e2::WORKSPACE_INIT;
			g_workspace.cam.posX = ws.posX;
			g_workspace.cam.posY = ws.posY;
			g_workspace.cam.posZ = ws.posZ;
			g_workspace.cam.pitch = ws.pitch;
			g_workspace.cam.roll = ws.roll;
			g_workspace.cam.heading = ws.heading;
			// loc_0488DE: MOVE.L #$0000000A,($0623FC).L
			// longword write sets the WORD at $0623FC (renderMode) = 0
			// and the WORD at $0623FE (flightState) = $000A (Prestinium)
			// The player starts "inside" the Prestinium during descent
			g_workspace.cam.renderMode = 0;
			g_workspace.cam.flightState = 0x000A;
			g_workspace.cam.grounded = ws.grounded;
			g_workspace.cam.thrust = ws.thrust;
			g_workspace.cam.vertVelocity = ws.vertVelocity;
			g_workspace.cam.thrustAccum = ws.thrustAccum;
			g_workspace.cam.landingProx = ws.landingProx;
			g_workspace.cam.inputFlags = ws.inputFlags;
			g_workspace.cam.crashEvent = false;
			g_workspace.cam.landingDelay = 0;
			g_workspace.cam.crashSoundFlag = 0;
			g_workspace.cam.soundLock = 0;
			g_workspace.cam.elevatorActive = 0;
			g_workspace.cam.transCooldown = 0;
			g_workspace.keyCommand = Action::NONE;
			g_workspace.pendingPing = false;
			g_workspace.pendingEventSlot = -1;
			g_workspace.cam.horizonMirrorMask = ws.horizonMirrorMask;
			g_workspace.tileDetail.spinAngle = 0;
			g_workspace.tileDetail.spinSin = 0;
			g_workspace.tileDetail.spinCos = 0;
			g_workspace.tileDetail.spinSpeed = 0;
			g_workspace.tileDetail.spinBaseX = 0;
			g_workspace.tileDetail.spinBaseZ = 0;
			g_workspace.tileDetail.spinVertCount = 0;
			g_workspace.tileDetail.tileVertCount = 0;
			g_workspace.tileDetail.tileProperty = 0;
			g_workspace.tileDetail.cachedPosX =
				0x00FF; // force reload on first frame
			g_workspace.tileDetail.cachedPosY = 0;
			g_workspace.tileDetail.cachedPosZ = 0x00FF;
			g_workspace.tileDetail.cachedD7 = 0xFF; // force workspace rebuild
			g_workspace.tileDetail.currentTileIndex = 0;
			g_workspace.cam.roadDrawDisable = 0;
			g_workspace.cam.screenCenterX = ws.screenCenterX;
			g_workspace.cam.screenCenterY = ws.screenCenterY;
		}

		// $0488DE-$0488F0: the init code overwrites specific fields
		// Only these three writes exist in the original:
		//   $0488DE: MOVE.L #$41000000,($0623AA) -- posY
		//   $0488E8: MOVE.W #$0300,($06234A)     -- pitch
		//   $0488F0: MOVE.L #$A,($0623FC)        -- renderMode=0,
		//   flightState=10
		g_workspace.cam.posY = 0x41000000;
		g_workspace.cam.pitch = 0x0300;
		g_workspace.cam.renderMode = 0;	  // high word of $0000000A
		g_workspace.cam.flightState = 10; // low word of $0000000A

		game.gameplayTickPhase = false;
		game.blackoutTimer = 0;
		game.pal3FlashIndex = 0;
		game.damageFlashTimer = 0;
		game.damageFlashPtr = 0x7000;

		// E1:2265: Timer B split starts at scanline 136 (bottom of viewport),
		// so top == bottom at init -- no visible split until the game changes
		// one
		g_workspace.cam.timerBScanline = 0x0088;
		g_workspace.cam.palBase89 = game.palOverride89;
		g_workspace.cam.palBase1011 = game.palOverride1011;
		g_workspace.cam.groundPal89 = game.palOverride89;
		g_workspace.cam.groundPal1011 = game.palOverride1011;

		// Interior state -- cleared at gameplay start
		g_workspace.cam.elevatorPhase = 0;
		g_workspace.cam.mirrorMask = 0;
		g_workspace.cam.transCooldown = 0;
		g_workspace.cam.roomChangedFlag = 0;
		g_workspace.cam.doorWipeDir = 0;
		g_workspace.cam.pendingRoomMsg = 0;
		g_workspace.cam.pendingMsg = 0;
		g_workspace.cam.pendingMsgFlag = 0;
		g_workspace.cam.movementSpeed =
			0x00031800;					   // E2+$2400: walk mode (type 3)
		g_workspace.cam.turnRate = 0x0006; // E2+$2404: walk turn rate
		g_workspace.inventoryStackDepth = 0;

		// Combat workspace fields -- cleared at gameplay start
		g_workspace.cam.collapseCountdown = 0;
		g_workspace.cam.collapseTileIndex = 0;
		g_workspace.cam.savedTileByte = 0;
		g_workspace.cam.patrolCounterA = 0;
		g_workspace.cam.patrolCounterB = 0;

		// $048042: initial palette override for entries 8-11
		// Palette 8=$0777, 9=$0357, 10=$0040, 11=$0000
		game.palOverride89 = PAL_DEFAULT_89;
		game.palOverride1011 = PAL_DEFAULT_1011;

		// $048906: Screen clear (sub_040F02)
		// sub_040F02 writes $0000FFFF to BP0+BP1 of the viewport,
		// leaving BP2+BP3 unchanged.  The net effect on our indexed
		// framebuffer is palette index 10 (dark green in the
		// gameplay palette)
		// Fill viewport on both buffers
		for (int i = 0; i < FB_WIDTH * VIEWPORT_H; i++)
		{
			game.indexBuf[0][i] = 10;
			game.indexBuf[1][i] = 10;
		}

		// $04890A: Load gameplay palette (sub_040CFC / dat_040D66)
		game.activeLut = game.gameplayLut;

		// In the normal flow, the intro script runs to completion before
		// gameplay starts.  State-setting instructions (SET_VAR_L,
		// SET_E4_WORD) execute during the intro and their results carry
		// over.  When skipping the intro, fast-forward these to ensure
		// the same state (e.g. credits = 9000)
		for (int i = gen_e4::INTRO_ENTRY; i < gen_e4::SCRIPT_COUNT; i++)
		{
			const ScriptInstr &si = gen_e4::SCRIPT[i];
			if (si.op == OP_SET_VAR_L)
			{
				int slot = static_cast<int>(si.idx) - 0xF8;
				if (slot >= 0 && slot < ScriptVM::VAR_TABLE_SIZE)
					game.scriptVM.varTable[slot] = si.value32;
			}
			else if (si.op == OP_SET_E4_WORD)
			{
				if (si.addr == SVAR_TEXT_SPEED)
					game.scriptVM.textSpeed = si.value;
				else if (si.addr == SVAR_SCRIPT_RUNNING)
					game.scriptVM.scriptRunning = si.value;
			}
			else if (si.op == OP_GOTO || si.op == OP_RETURN)
			{
				break;
			}
		}

#ifndef NDEBUG
		if (DEV_START_CREDITS != 0)
		{
			game.scriptVM.varTable[0] = DEV_START_CREDITS;
		}
#endif

		// $04890E-$048916: Load LANDING_SCRIPT into the script VM
		// The original does NOT reinitialise the VM -- state variables
		// (textSpeed, scriptRunning, flags) carry over from the intro
		game.scriptVM.pc = gen_e4::EVENT_ENTRY[0];
		game.scriptVM.callDepth = 0;

		// Init object state from E2 binary data
		ObjectsInit(g_workspace.objs);

		// loc_048A8E (E1:12744-12746): first-frame outdoor init
		// Clear room (already 0 from ObjectsInit) and build active list
		ObjectsBuildActiveList(g_workspace.objs);
		HudFactionLED();

		// Init Benson for gameplay text
		BensonInit(game.benson);
		game.benson.varTable = game.scriptVM.varTable;

		// Init camera trig and matrix values
		g_workspace.cam.altFactor = 0x00041000;
		g_workspace.cam.screenCenterX = 160;
		g_workspace.cam.screenCenterY = 68;
		g_workspace.cam.horizonMirrorMask = 0;
		for (int i = 0; i < 4; i++)
			g_workspace.cam.channelCoord[i] = 0;
		CameraComputeAltFactor(g_workspace.cam);
		CameraComputeTrig(g_workspace.cam, g_workspace.tileDetail);

		// Blit the HUD background into the framebuffer
		// Blit HUD to both buffers (both are displayed alternately)
		std::memcpy(game.indexBuf[0] + FB_WIDTH * VIEWPORT_H, gen_m::HUD_BITMAP,
					FB_WIDTH * HUD_H);
		std::memcpy(game.indexBuf[1] + FB_WIDTH * VIEWPORT_H, gen_m::HUD_BITMAP,
					FB_WIDTH * HUD_H);

		// $04886E: BSR sub_040F9A -- draw weapon sight indicator
		// Sets the initial walk/run indicator on the HUD panel based
		// on the current movementSpeed type (init value = walk/$0003)
		HudWeaponSightIndicator();

#ifndef NDEBUG
		// Dev inventory deferred until after crash blackout completes
		game.devInventoryState = (DEV_START_INVENTORY != 0) ? 1 : 0;
#endif

		// Set bp0=1 across the Benson text area to match post-intro state
		for (int buf = 0; buf < 2; buf++)
		{
			for (int row = 0; row < BENSON_CHAR_H; row++)
			{
				int y = BENSON_TEXT_Y + row;
				for (int x = BENSON_TEXT_X; x < BENSON_TEXT_X + BENSON_TEXT_W;
					 x++)
				{
					game.indexBuf[buf][y * FB_WIDTH + x] |= 0x01;
				}
			}
		}
	}
}

// Per-mode tick functions

static void TickTitle(Game &game)
{
	std::memcpy(DrawTarget(game), gen_e0::TITLE_BITMAP, FB_PIXELS);
	game.activeLut = game.titleLut;

	constexpr uint32_t TITLE_HOLD_FRAMES = 150;
	if (SKIP_INTRO)
	{
		EnterMode(game, MODE_GAMEPLAY);
		InitGameplayPalette(game);
	}
	else if (game.frameCount >= TITLE_HOLD_FRAMES)
	{
		EnterMode(game, MODE_INTRO);
	}
}

static void TickIntro(Game &game, bool /*anyKeyDown*/)
{
	game.activeLut = game.initialLut;

	bool running = IntroTick(game.intro, game.scriptVM, game.benson,
							 DrawTarget(game), &game.audio);
	if (!running)
	{
		EnterMode(game, MODE_GAMEPLAY);
		InitGameplayPalette(game);
	}
}

static void TickOutro(Game &game)
{
	game.activeLut = game.initialLut;
	OutroTick(game.outro, game.scriptVM, game.benson, DrawTarget(game),
			  &game.audio);
}

// Message queue (sub_0441C0, E1:5157-5169)

void MessageDisplay(Game &game, int msgIndex)
{
	(void)game;
	// E1:5158-5159: TST.W ($062478).L; BNE RTS
	if (g_workspace.cam.pendingMsgFlag != 0)
		return;

	// E1:5161-5164: D1 *= 2; A0 = $070000 + D1; MOVE.W (A0),$062470
	// Store the message index.  gen_e4::MESSAGES[i] plays the role that
	// word (A0) -> pointer-low-half plays in the original: a lookup key
	// into the text data region
	g_workspace.cam.pendingMsg = static_cast<uint16_t>(msgIndex);

	// E1:5166: MOVE.W #$0001,($062478).L
	g_workspace.cam.pendingMsgFlag = 0x0001;
}

// Main loop tick (~25 Hz), loc_048948

static void MainLoopTick(Game &game)
{
	// sub_045306 (E1:12673): keyboard command dispatch
	KeyCommandDispatch(game);

	// sub_04418C: dispatch pending event script if one was triggered
	// during the rendering pass (vehicle boarding with flagsTable bit 2)
	if (g_workspace.pendingEventSlot >= 0)
	{
		ScriptVMLoadEvent(game.scriptVM, g_workspace.pendingEventSlot);
		g_workspace.pendingEventSlot = -1;
	}

	// sub_042B90: script VM tick
	ScriptVMTick(game.scriptVM, game.benson);

	// sub_0411FC: hired ship descent
	{
		int16_t hire = static_cast<int16_t>(game.scriptVM.shipHireFlag);
		if (hire != 0)
		{
			ObjectState &objs = g_workspace.objs;
			if (hire < 0)
			{
				// Spawn at max altitude, landing coords 08-08
				objs.posY[OBJ_INTERSTELLAR_CRAFT] =
					static_cast<int32_t>(0x7F000000);
				objs.posX[OBJ_INTERSTELLAR_CRAFT] =
					static_cast<int32_t>(0x00089900);
				objs.posZ[OBJ_INTERSTELLAR_CRAFT] =
					static_cast<int32_t>(0x00088B00);
				objs.slotTable[OBJ_INTERSTELLAR_CRAFT] = 0x00;
				game.scriptVM.shipHireFlag = 0x0001;
			}
			else
			{
				// Exponential descent: posY -= (posY >>> 8) + $80
				int32_t y = objs.posY[OBJ_INTERSTELLAR_CRAFT];
				uint32_t delta = (static_cast<uint32_t>(y) >> 8) + 0x80;
				y -= static_cast<int32_t>(delta);
				if (y < 0)
				{
					y = 0;
					game.scriptVM.shipHireFlag = 0x0000;
				}
				objs.posY[OBJ_INTERSTELLAR_CRAFT] = y;
			}
		}
	}

	// sub_047D8C: movement physics
	CameraMovementTick(g_workspace.cam, game.scriptVM);

	// $042240: interstellar escape -- endgameFlag set by script during
	// flight state 7 triggers transition to starfield outro
	// Boarding sets flightState = slot (E1:5051), so state 7 = interstellar
	if (game.scriptVM.endgameFlag != 0 &&
		g_workspace.cam.flightState == OBJ_INTERSTELLAR_CRAFT)
	{
		OutroInit(game.outro, game.scriptVM, game.benson, DrawTarget(game));
		game.mode = MODE_OUTRO;
		return;
	}

	// Crash event: CrashLand (Camera.cpp) already ran the full
	// $0481B8 sequence (posY, grounded, BSR loc_0440AC vehicle exit)
	// We handle the crash sound ($0481CE) + blackout ($041AD2) here
	if (g_workspace.cam.crashEvent)
	{
		g_workspace.cam.crashEvent = false;

		// $0481CE-$04824C: crash sound -- noise burst on channel B
		{
			game.intro.mixerShadow |= 0x02;	 // BSET #1 -- disable tone B
			game.intro.mixerShadow &= ~0x10; // BCLR #4 -- enable noise B

			AudioWriteReg(game.audio, YM_NOISE_PERIOD, 0x1F);
			AudioWriteReg(game.audio, YM_MIXER, game.intro.mixerShadow);
			AudioWriteReg(game.audio, YM_AMP_B, 0x10);
			AudioWriteReg(game.audio, YM_ENV_FINE, 0xFF);
			AudioWriteReg(game.audio, YM_ENV_COARSE, 0x3F);
			AudioWriteReg(game.audio, YM_ENV_SHAPE, 0x00);
			// E1:12623: set soundLock to prevent AMP_C silence
			// during crash envelope decay
			g_workspace.cam.soundLock = 0xFFFF;
		}

		// $041AD2: zero palette entries 8-11 (blackout)
		game.palOverride89 = 0;
		game.palOverride1011 = 0;

		// Delay loop: $50000 iterations on the 68000
		game.blackoutTimer = 5898240;

		for (int i = 8; i < 12; i++)
			game.gameplayLut[i] = 0xFF000000u; // black
	}

	// E1:12676-12677: dispatch on renderMode
	// $0623FC != 0 -> indoor frame loop (loc_04895E)
	// $0623FC == 0 -> outdoor ground path (loc_048A9A)
	if (g_workspace.cam.renderMode != 0)
	{
		// ============ INDOOR PATH (loc_04895E, E1:12679) ============

		// E1:12680: sub_047D8C -- scene transitions (handled above by
		// CameraMovementTick)

		// E1:12681: sub_0410A0 -- clear low altitude warning
		HudAltitudeWarningClear();

		// E1:12682: sub_04655E -- trig computation
		CameraComputeTrig(g_workspace.cam, g_workspace.tileDetail);

		// E1:12683: sub_042416 -- SIGHTS reticle overlay
		HudSightsReticle(DrawTarget(game));

		// sub_048B10 (E1:12787-12812): doorWipeDir dispatch + buffer swap

		// E1:12787-12791: first doorWipeDir test
		if (g_workspace.cam.doorWipeDir != 0)
		{
			if (g_workspace.cam.doorWipeDir < 0)
			{
				// E1:12790: BSR sub_041C64 -- blocks ~58 frames
				DoorWipeHide(game);
				// E1:12791: BRA loc_048B3E -- fall through to buffer swap
			}
			else
			{
				// E1:12793-12796: loc_048B20 -- fill top palette uniform
				// Three MOVE.W from $0623F6 (high word of palOverride1011)
				uint16_t e10 =
					static_cast<uint16_t>(game.palOverride1011 >> 16);
				game.palOverride89 =
					(static_cast<uint32_t>(e10) << 16) | e10; // E1:12794-12795
				game.palOverride1011 =
					(static_cast<uint32_t>(e10) << 16) | e10; // E1:12796
			}
		}

		// E1:12798-12808: loc_048B3E -- buffer swap + VSync
		game.drawBuffer ^= 1; // E1:12800-12803
		VBLHandler(game);	  // E1:12806-12808

		// E1:12810-12812: second doorWipeDir test
		if (g_workspace.cam.doorWipeDir > 0)
		{
			// E1:12812: BSR sub_041AEE -- blocks ~54 frames
			DoorWipeReveal(game);
		}

		FillViewport(DrawTarget(game), g_workspace.cam);

		// E1:12685-12688: check pending room message
		if (g_workspace.cam.pendingRoomMsg != 0)
		{
			MessageDisplay(game, g_workspace.cam.pendingRoomMsg);
			g_workspace.cam.pendingRoomMsg = 0;
		}

		// E1:12691-12692: if renderMode back to 0, exit to outdoor post-fill
		if (g_workspace.cam.renderMode == 0)
			goto outdoor_post_fill;

		// E1:12693-12703: boundary clamp -> door scan -> room transition
		if (RoomBoundaryClamp(g_workspace.cam))
		{
			// E1:12694: BEQ loc_0489BC -- at boundary, check for doors
			// E1:12695-12696: TST.B $062412; BNE skip -- cooldown check
			if (g_workspace.cam.landingDelay == 0)
			{
				// E1:12697: BSR sub_044FC2 -- door proximity scan
				uint16_t doorRoom;
				uint8_t doorBuildIdx;
				if (RoomDoorScan(g_workspace.cam, game, doorRoom, doorBuildIdx))
				{
					// E1:12699: BSR sub_044B5A -- room transition
					RoomLoad(doorRoom, doorBuildIdx, game.scriptVM);
					// E1:12700-12702: if renderMode still != 0,
					// set cooldown = 3
					if (g_workspace.cam.renderMode != 0)
						g_workspace.cam.landingDelay = 3;
				}
			}
		}

		// loc_0489BC (E1:12706-12736): wall collision sound
		// When roomChangedFlag ($0624B8) is set by the boundary clamp's
		// rising-edge detector, play a wall-thump YM sound sequence
		// and clear the flag
		if (g_workspace.cam.roomChangedFlag != 0)
		{
			// E1:12708-12733: YM register sequence for wall collision
			// BCLR #1, mixerShadow -- enable tone B
			game.intro.mixerShadow &= ~0x02;
			// reg 2 = $00 (tone B fine)
			AudioWriteReg(game.audio, YM_TONE_B_FINE, 0x00);
			// reg 3 = $08 (tone B coarse)
			AudioWriteReg(game.audio, YM_TONE_B_COARSE, 0x08);
			// BCLR #4, mixerShadow -- enable noise B
			game.intro.mixerShadow &= ~0x10;
			// reg 6 = $1F (noise period)
			AudioWriteReg(game.audio, YM_NOISE_PERIOD, 0x1F);
			// reg 7 = mixerShadow
			AudioWriteReg(game.audio, YM_MIXER, game.intro.mixerShadow);
			// reg 9 = $10 (amp B = envelope mode)
			AudioWriteReg(game.audio, YM_AMP_B, 0x10);
			// reg $0B = $00 (envelope fine)
			AudioWriteReg(game.audio, YM_ENV_FINE, 0x00);
			// reg $0C = $08 (envelope coarse)
			AudioWriteReg(game.audio, YM_ENV_COARSE, 0x08);
			// reg $0D = $00 (envelope shape = one-shot decay)
			AudioWriteReg(game.audio, YM_ENV_SHAPE, 0x00);

			// E1:12736: CLR.W ($0624B8)
			g_workspace.cam.roomChangedFlag = 0;
		}

		// NOTE: the original does NOT call sub_046684 (CameraBuildMatrix)
		// in the indoor path.  E1:12754 is in the outdoor path only
		// (loc_048A9A).  Screen centres are frozen from the last outdoor
		// frame (elevator descent)

		// loc_048A7E (E1:12738-12741): indoor viewport rendering
		// BSR sub_046CAE -- project all room vertices
		RoomProjectVertices(g_workspace.cam);
		// BSR sub_04741E -- draw all room edges (BSET mode)
		RoomDrawEdges(DrawTarget(game), g_workspace.cam);
		// BSR sub_043C48 -- draw world objects (BCLR mode)
		ObjectsDraw(DrawTarget(game), g_workspace.cam, g_workspace.objs);

		// In the original, sub_045306 clears $062422 each frame before
		// polling the keyboard.  B/T scancodes only persist for one
		// rendering pass.  Clear unconsumed B/T here
		g_workspace.keyCommand = Action::NONE;

		// Play pending interaction ping (sub_0441E8)
		// In the original, sub_0441E8 is called inline from sub_04408E
		// (E1:5067,5074).  The port defers it via pendingPing which
		// must be processed in both the indoor and outdoor paths
		if (g_workspace.pendingPing)
		{
			g_workspace.pendingPing = false;
			InteractionPing(game);
		}

		// E1:12742: BRA loc_048948 -- return to main loop top
		// The indoor path does NOT fall through to the combat ticks
		// (sub_0416D6, sub_041442, sub_041258, sub_041832).  Those
		// only run on the outdoor path at E1:12774-12779
		return;
	}

	// ============ GROUND PATH (loc_048A9A, E1:12748) ============

	// E1:12749: sub_047D8C -- scene transitions (handled above)

	// E1:12750: sub_041036 -- low altitude warning (conditionally show/clear)
	HudAltitudeWarning();

	// E1:12751: sub_04655E -- trig computation
	CameraComputeTrig(g_workspace.cam, g_workspace.tileDetail);

	// E1:12752: sub_044640 -- altitude factor + tile change + D7 + tile load
	// E1:5637-5638: sub_044640 returns immediately when elevatorActive != 0,
	// freezing altFactor and skipping tile-change/D7 updates during
	// transitions. E1:5996-5998: sub_04497E likewise returns immediately during
	// elevator
	if (g_workspace.cam.elevatorActive == 0)
	{
		CameraComputeAltFactor(g_workspace.cam);
		TileDetailLoad(g_workspace.tileDetail, g_workspace.cam);
		RoadsTerrainUpdate(g_workspace.cam);
		SpinningVertexUpdate(g_workspace.tileDetail, g_workspace.cam);
	}

	// E1:12754: sub_046684 -- build rotation matrix
	CameraBuildMatrix(g_workspace.cam);

	// E1:12755: sub_042416 -- SIGHTS reticle overlay
	HudSightsReticle(DrawTarget(game));

	// E1:12756: sub_048B10 -- buffer swap + fill viewport
	game.drawBuffer ^= 1;
	VBLHandler(game);
	FillViewport(DrawTarget(game), g_workspace.cam);

	// E1:12759: sub_0444A2 -- draw coarse road grid (BSET mode)
	RoadsDraw(DrawTarget(game), g_workspace.cam);

	// E1:12760-12761: TST.W ($062420); BEQ skip -- no tile detail if off-grid
	// E1:12770-12771: sub_04741E -- tile detail
	if (g_workspace.tileDetail.currentTileIndex != 0)
		TileDetailDraw(DrawTarget(game), g_workspace.tileDetail,
					   g_workspace.cam);

	// E1:12774: sub_0411E2 -- NPC Dominion Dart orbit update
	// Moves dart A world Z by $100 each frame, wrapping at $FFFFF
	{
		int32_t z = g_workspace.objs.posZ[OBJ_DOMINION_DART_A];
		z += 0x100;
		z &= 0x000FFFFF;
		g_workspace.objs.posZ[OBJ_DOMINION_DART_A] = z;
	}

outdoor_post_fill:
	// E1:12775: sub_043C48 -- draw world objects (BCLR mode)
	// NOTE: original does NOT call sub_043C14 (build active list) here
	// The active list is only rebuilt at state-change events
	ObjectsDraw(DrawTarget(game), g_workspace.cam, g_workspace.objs);

	// Clear unconsumed B/T scancodes after the rendering pass
	// In the original, sub_045306 clears $062422 at the start of
	// each frame, so B/T only persist for one pass
	g_workspace.keyCommand = Action::NONE;

	// Play pending interaction ping (set by vehicle boarding etc)
	if (g_workspace.pendingPing)
	{
		g_workspace.pendingPing = false;
		InteractionPing(game);
	}

	// E1:12776-12779: combat subsystem (projectile, destruction, motion,
	// missile)
	CombatOutdoorTick(game);

	// E1:12780-12782: faction LED (sub_0410F0)
	// Only called when player has metal detector (bit 7 set)
	if (static_cast<int8_t>(g_workspace.objs.flagsTable[OBJ_METAL_DETECTOR]) <
		0)
		HudFactionLED();
}

static void TickGameplay(Game &game, bool anyKeyDown, bool yKeyDown,
						 const SaveLoadInput &slInput)
{
	// Architecture note: in the original, the VBL handler is an
	// interrupt that fires AFTER sub_048B10's VSync wait -- i.e
	// after the buffer swap.  So the VBL handler and main loop
	// rendering both write to the SAME post-swap draw target
	//
	// We replicate this by running VBLHandler AFTER the swap on
	// main-loop ticks, and standalone on off-ticks (where no swap
	// occurs and the draw target is unchanged)

	// Blackout: the main loop is stuck in a busy-wait at $041AD2
	// The VBL handler still fires (palette, Benson, audio)
	if (game.blackoutTimer > 0)
	{
#ifndef NDEBUG
		// Blackout started -- advance to "ready" state
		if (game.devInventoryState == 1)
			game.devInventoryState = 2;
#endif

		VBLHandler(game);
		constexpr int CYCLES_PER_VBL = 160000;
		constexpr int VBL_HANDLER_COST = 30000;
		constexpr int DELAY_CYCLES_PER_VBL = CYCLES_PER_VBL - VBL_HANDLER_COST;

		game.blackoutTimer -= DELAY_CYCLES_PER_VBL;
		if (game.blackoutTimer <= 0)
		{
			game.blackoutTimer = 0;
			game.palOverride89 = PAL_DEFAULT_89;
			game.palOverride1011 = PAL_DEFAULT_1011;
		}
		return;
	}

#ifndef NDEBUG
	// Dev: inject inventory after crash blackout completes
	if (game.devInventoryState == 2)
	{
		game.devInventoryState = 0;
		for (int slot = 0; slot < 64; slot++)
		{
			if (!(DEV_START_INVENTORY & (1ULL << slot)))
				continue;
			if (g_workspace.inventoryStackDepth >=
				Workspace::INVENTORY_STACK_MAX)
				break;
			g_workspace.inventoryStack[g_workspace.inventoryStackDepth] = slot;
			g_workspace.inventoryStackDepth++;
			g_workspace.objs.flagsTable[slot] |= OBJ_FLAG_TAKEN;
			g_workspace.objs.slotTable[slot] = 0xFF;
		}
		ObjectsBuildActiveList(g_workspace.objs);
	}
#endif

	// sub_041AA0: damage flash freezes the main loop
	if (game.damageFlashTimer > 0)
	{
		VBLHandler(game);
		return;
	}

	// Save/load modal -- pauses gameplay, VBL still fires
	// the original's blocking busy-wait during the save/load
	// dialog (sub_045094 keyboard loop + sub_0450C0 frame wait)
	if (SaveLoadActive(game.saveLoad))
	{
		g_workspace.keyCommand = Action::NONE;
		VBLHandler(game);
		SaveLoadTick(game.saveLoad, game, slInput);
		return;
	}

	// Propagate keyboard state to the script VM every VBL tick
	if (anyKeyDown)
		game.scriptVM.flags |= 0x0080;
	if (yKeyDown)
	{
		game.scriptVM.yKeyPressed = true;
		game.scriptVM.lastKeyByte = 0x15;
	}

	// Main loop runs every other VBL (~25 Hz)
	game.gameplayTickPhase = !game.gameplayTickPhase;
	if (!game.gameplayTickPhase)
	{
		VBLHandler(game);
		return;
	}

	MainLoopTick(game);
}

void GameInit(Game &game)
{
	game.mode = MODE_TITLE;
	game.frameCount = 0;
	game.drawBuffer = 0;
	game.prevBensonBuf = nullptr;
	std::memset(game.indexBuf, 0, sizeof(game.indexBuf));

	BuildArgbLut(gen_loader::TITLE_PALETTE, game.titleLut);
	BuildArgbLut(gen_e1::INITIAL_PALETTE, game.initialLut);
	BuildArgbLut(gen_e1::GAMEPLAY_PALETTE, game.gameplayLut);

	game.activeLut = game.titleLut;

	// sub_040C58 (E1:315-330): fill $077000-$077FFF with random data
	// The original uses XBIOS Random(); we use a simple LCG
	{
		uint32_t rng = 0x12345678;
		for (int i = 0; i < 2048; i++)
		{
			rng = rng * 1103515245u + 12345u;
			game.randomBuf[i] = static_cast<uint16_t>(rng >> 16);
		}
		game.damageFlashPtr = 0;
		game.damageFlashTimer = 0;
		game.prevElevatorActive = 0;
	}

	ScriptVMInit(game.scriptVM);
	AudioInit(game.audio);
	game.scriptVM.audio = &game.audio;
	game.scriptVM.mixerShadow = &game.intro.mixerShadow;
	game.scriptVM.objects = &g_workspace.objs;
	game.scriptVM.camera = &g_workspace.cam;

	// Mixer shadow ($0624EA) from E2 workspace
	// Used by both intro and gameplay sound code
	game.intro.mixerShadow = gen_e2::WORKSPACE_INIT.mixerShadow;

	// Save directory (SDL_GetPrefPath)
	game.savePath = SaveLoadInitPath();
}

void GameTick(Game &game, bool anyKeyDown, bool yKeyDown, bool quit, bool ctrlS,
			  bool ctrlL, int digitKey, bool returnKey)
{
	// Escape during save/load dialog cancels the dialog, not the game
	if (quit && SaveLoadActive(game.saveLoad))
	{
		quit = false;
		anyKeyDown = true;
	}

	if (quit)
	{
		game.mode = MODE_QUIT;
		return;
	}

	switch (game.mode)
	{
	case MODE_TITLE:
		TickTitle(game);
		break;

	case MODE_INTRO:
		TickIntro(game, anyKeyDown);
		break;

	case MODE_GAMEPLAY:
	{
		// Save/load triggers -- only in gameplay mode
		if (ctrlS && !SaveLoadActive(game.saveLoad))
			SaveLoadTrigger(game.saveLoad, true);
		if (ctrlL && !SaveLoadActive(game.saveLoad))
			SaveLoadTrigger(game.saveLoad, false);

		// Forward save/load input to the state machine
		SaveLoadInput slInput;
		slInput.digitKey = digitKey;
		slInput.returnKey = returnKey;
		slInput.anyKeyDown = anyKeyDown;

		TickGameplay(game, anyKeyDown, yKeyDown, slInput);
		break;
	}

	case MODE_OUTRO:
		TickOutro(game);
		break;

	case MODE_QUIT:
		break;
	}

	game.frameCount++;
}
