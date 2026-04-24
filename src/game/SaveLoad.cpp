// Save/load -- loc_0451E2, loc_0450DA, loc_048856 (E1)

#include "game/SaveLoad.h"

#include "DevUtils.h"
#include "game/Benson.h"
#include "game/Camera.h"
#include "game/Controls.h"
#include "game/Game.h"
#include "game/Interior.h"
#include "game/Objects.h"
#include "game/TileDetail.h"
#include "game/VblHandler.h"
#include "game/Workspace.h"
#include "renderer/Hud.h"

#include <SDL.h>

#include <cstdio>
#include <cstring>

// Save format constants

#if SECOND_CITY
static constexpr uint8_t SAVE_MAGIC[4] = {0x70, 0x91, 0x5B, 0x41};
#else
static constexpr uint8_t SAVE_MAGIC[4] = {0x32, 0x30, 0x12, 0x53};
#endif
static constexpr uint16_t SAVE_VERSION = 1;

// Message indices (E4 message table, E4Data.cpp MESSAGES[])
static constexpr int MSG_BLANK = 50;
static constexpr int MSG_DISK_ERROR = 51;
static constexpr int MSG_CONFIRM = 56;
static constexpr int MSG_LOAD_PROMPT = 58;
static constexpr int MSG_SAVE_PROMPT = 59;

// Result display time (~1.2 seconds at 50 Hz)
static constexpr int RESULT_FRAMES = 60;

// Binary write/read helpers

static void WriteU8(uint8_t *&p, uint8_t v) { *p++ = v; }

static void WriteU16(uint8_t *&p, uint16_t v)
{
	std::memcpy(p, &v, 2);
	p += 2;
}

static void WriteU32(uint8_t *&p, uint32_t v)
{
	std::memcpy(p, &v, 4);
	p += 4;
}

static void WriteI8(uint8_t *&p, int8_t v) { *p++ = static_cast<uint8_t>(v); }

static void WriteI16(uint8_t *&p, int16_t v)
{
	std::memcpy(p, &v, 2);
	p += 2;
}

static void WriteI32(uint8_t *&p, int32_t v)
{
	std::memcpy(p, &v, 4);
	p += 4;
}

static void WriteBytes(uint8_t *&p, const void *src, int n)
{
	std::memcpy(p, src, n);
	p += n;
}

static uint8_t ReadU8(const uint8_t *&p) { return *p++; }

static uint16_t ReadU16(const uint8_t *&p)
{
	uint16_t v;
	std::memcpy(&v, p, 2);
	p += 2;
	return v;
}

static uint32_t ReadU32(const uint8_t *&p)
{
	uint32_t v;
	std::memcpy(&v, p, 4);
	p += 4;
	return v;
}

static int8_t ReadI8(const uint8_t *&p) { return static_cast<int8_t>(*p++); }

static int16_t ReadI16(const uint8_t *&p)
{
	int16_t v;
	std::memcpy(&v, p, 2);
	p += 2;
	return v;
}

static int32_t ReadI32(const uint8_t *&p)
{
	int32_t v;
	std::memcpy(&v, p, 4);
	p += 4;
	return v;
}

static void ReadBytes(const uint8_t *&p, void *dst, int n)
{
	std::memcpy(dst, p, n);
	p += n;
}

// Serialisation

static constexpr int MAX_SAVE_SIZE = 16384;

static int SerialiseState(const Game &game, uint8_t *buf)
{
	uint8_t *p = buf;
	const Camera &cam = g_workspace.cam;
	const ObjectState &objs = g_workspace.objs;
	const ScriptVM &vm = game.scriptVM;
	const TileDetailState &td = g_workspace.tileDetail;

	// Header
	WriteBytes(p, SAVE_MAGIC, 4);
	WriteU16(p, SAVE_VERSION);
	WriteU16(p, 0); // reserved

	// Camera state
	WriteI32(p, cam.posX);
	WriteI32(p, cam.posY);
	WriteI32(p, cam.posZ);
	WriteU16(p, cam.heading);
	WriteU16(p, cam.roll);
	WriteU16(p, cam.pitch);
	WriteI32(p, cam.renderMode);
	WriteU16(p, cam.flightState);
	WriteU16(p, cam.grounded);
	WriteU32(p, cam.thrust);
	WriteU32(p, cam.vertVelocity);
	WriteU32(p, cam.thrustAccum);
	WriteU16(p, cam.elevatorActive);
	WriteU16(p, cam.soundLock);
	WriteU32(p, cam.movementSpeed);
	WriteU16(p, cam.turnRate);
	WriteU16(p, cam.transCooldown);
	WriteU16(p, cam.mirrorMask);
	WriteU32(p, cam.doorTablePtr);
	WriteU8(p, cam.roomExtentX);
	WriteU8(p, cam.roomExtentZ);
	WriteI8(p, cam.doorWipeDir);
	WriteU16(p, cam.timerBScanline);
	WriteU32(p, cam.palBase89);
	WriteU32(p, cam.palBase1011);
	WriteU32(p, cam.groundPal89);
	WriteU32(p, cam.groundPal1011);
	WriteU32(p, cam.elevatorDest);
	WriteU16(p, cam.elevatorPhase);
	WriteU16(p, cam.roomChangedFlag);
	WriteU16(p, cam.pendingRoomMsg);
	WriteU16(p, cam.collapseCountdown);
	WriteU16(p, cam.collapseTileIndex);
	WriteU8(p, cam.savedTileByte);
	WriteI16(p, cam.patrolCounterA);
	WriteI16(p, cam.patrolCounterB);
	WriteU16(p, cam.horizonMirrorMask);
	WriteU16(p, cam.landingProx);
	WriteU8(p, cam.inputFlags);
	WriteU8(p, cam.wallContactState);
	WriteU16(p, cam.crashSoundFlag);
	WriteU8(p, cam.landingDelay);
	WriteU16(p, cam.roadDrawDisable);

	// Object state
	for (int i = 0; i < OBJ_SLOTS; i++)
		WriteI32(p, objs.posX[i]);
	for (int i = 0; i < OBJ_SLOTS; i++)
		WriteI32(p, objs.posY[i]);
	for (int i = 0; i < OBJ_SLOTS; i++)
		WriteI32(p, objs.posZ[i]);
	for (int i = 0; i < OBJ_SLOTS; i++)
		WriteI32(p, objs.velX[i]);
	for (int i = 0; i < OBJ_SLOTS; i++)
		WriteI32(p, objs.velY[i]);
	for (int i = 0; i < OBJ_SLOTS; i++)
		WriteI32(p, objs.velZ[i]);
	for (int i = 0; i < OBJ_SLOTS; i++)
		WriteU16(p, objs.motionTimer[i]);
	WriteBytes(p, objs.slotTable, OBJ_SLOTS);
	WriteBytes(p, objs.flagsTable, OBJ_SLOTS);
	WriteBytes(p, objs.typeTable, 256);
	WriteBytes(p, objs.rotSelector, 16);
	WriteU8(p, objs.currentRoom);

	// Workspace fields
	WriteU16(p, g_workspace.inventoryStackDepth);
	WriteBytes(p, g_workspace.inventoryStack, Workspace::INVENTORY_STACK_MAX);

	// Room geometry workspace
	for (int i = 0; i < Workspace::ROOM_SLOT_MAX; i++)
		WriteI32(p, g_workspace.vertLongX[i]);
	for (int i = 0; i < Workspace::ROOM_SLOT_MAX; i++)
		WriteI32(p, g_workspace.vertLongY[i]);
	for (int i = 0; i < Workspace::ROOM_SLOT_MAX; i++)
		WriteI32(p, g_workspace.vertLongZ[i]);
	for (int i = 0; i < Workspace::ROOM_SLOT_MAX; i++)
		WriteI32(p, g_workspace.vertLongVel[i]);
	for (int i = 0; i < Workspace::ROOM_SLOT_MAX; i++)
		WriteU16(p, g_workspace.edgeA[i]);
	for (int i = 0; i < Workspace::ROOM_SLOT_MAX; i++)
		WriteU16(p, g_workspace.edgeB[i]);
	WriteU16(p, g_workspace.roomVertHW);
	WriteU16(p, g_workspace.roomEdgeHW);

	// Script VM state
	// The 5 documented script state words (E4 $0100-$0108)
	WriteU16(p, vm.scriptRunning);
	WriteU16(p, vm.textSpeed);
	WriteU16(p, vm.attackFlag);
	WriteU16(p, vm.shipHireFlag);
	WriteU16(p, vm.endgameFlag);
	WriteU16(p, vm.flags);
	WriteI32(p, vm.pc);
	WriteI32(p, vm.callDepth);
	for (int i = 0; i < SCRIPT_CALL_STACK_DEPTH; i++)
		WriteI32(p, vm.callStack[i]);
	WriteU32(p, vm.frameCounter);
	WriteU16(p, vm.score);
	WriteU8(p, (vm.flags & ScriptVM::FLAG_EVENT_ACTIVE) ? 1 : 0);
	WriteI32(p, vm.eventReturnPC);
	for (int i = 0; i < ScriptVM::WORD_VAR_TABLE_SIZE; i++)
		WriteU16(p, vm.wordVarTable[i]);
	for (int i = 0; i < ScriptVM::VAR_TABLE_SIZE; i++)
		WriteU32(p, vm.varTable[i]);
	for (int i = 0; i < ScriptVM::RNG_TABLE_SIZE; i++)
		WriteU16(p, vm.rngTable[i]);
	WriteU16(p, vm.rngPointer);
	WriteU8(p, vm.lastKeyByte);

	// Game struct palette/timer state
	WriteU32(p, game.palOverride89);
	WriteU32(p, game.palOverride1011);
	WriteI32(p, static_cast<int32_t>(game.blackoutTimer));
	WriteI32(p, static_cast<int32_t>(game.pal3FlashIndex));
	WriteI32(p, static_cast<int32_t>(game.damageFlashTimer));
	WriteU16(p, game.damageFlashPtr);
	for (int i = 0; i < 2048; i++)
		WriteU16(p, game.randomBuf[i]);
	WriteU8(p, game.intro.mixerShadow); // $0624EA: YM mixer shadow

	// TileDetailState
	WriteU16(p, td.spinAngle);
	WriteU16(p, td.spinSpeed);
	WriteI16(p, td.spinBaseX);
	WriteI16(p, td.spinBaseZ);
	WriteU8(p, td.spinVertCount);
	WriteI8(p, td.tileProperty);
	WriteU16(p, td.cachedPosX);
	WriteU16(p, td.cachedPosY);
	WriteU16(p, td.cachedPosZ);
	WriteU16(p, td.currentTileIndex);
	WriteU8(p, td.cachedD7);
	WriteU16(p, td.drawModeSplit);
	WriteU8(p, td.tileVertCount);

	return static_cast<int>(p - buf);
}

static bool DeserialiseState(Game &game, const uint8_t *buf, int size)
{
	const uint8_t *p = buf;
	const uint8_t *end = buf + size;
	Camera &cam = g_workspace.cam;
	ObjectState &objs = g_workspace.objs;
	ScriptVM &vm = game.scriptVM;
	TileDetailState &td = g_workspace.tileDetail;

	if (size < 8)
		return false;
	if (std::memcmp(p, SAVE_MAGIC, 4) != 0)
		return false;
	p += 4;
	uint16_t ver = ReadU16(p);
	if (ver != SAVE_VERSION)
		return false;
	p += 2; // skip reserved

	// Camera state
	cam.posX = ReadI32(p);
	cam.posY = ReadI32(p);
	cam.posZ = ReadI32(p);
	cam.heading = ReadU16(p);
	cam.roll = ReadU16(p);
	cam.pitch = ReadU16(p);
	cam.renderMode = ReadI32(p);
	cam.flightState = ReadU16(p);
	cam.grounded = ReadU16(p);
	cam.thrust = ReadU32(p);
	cam.vertVelocity = ReadU32(p);
	cam.thrustAccum = ReadU32(p);
	cam.elevatorActive = ReadU16(p);
	cam.soundLock = ReadU16(p);
	cam.movementSpeed = ReadU32(p);
	cam.turnRate = ReadU16(p);
	cam.transCooldown = ReadU16(p);
	cam.mirrorMask = ReadU16(p);
	cam.doorTablePtr = ReadU32(p);
	cam.roomExtentX = ReadU8(p);
	cam.roomExtentZ = ReadU8(p);
	cam.doorWipeDir = ReadI8(p);
	cam.timerBScanline = ReadU16(p);
	cam.palBase89 = ReadU32(p);
	cam.palBase1011 = ReadU32(p);
	cam.groundPal89 = ReadU32(p);
	cam.groundPal1011 = ReadU32(p);
	cam.elevatorDest = ReadU32(p);
	cam.elevatorPhase = ReadU16(p);
	cam.roomChangedFlag = ReadU16(p);
	cam.pendingRoomMsg = ReadU16(p);
	cam.collapseCountdown = ReadU16(p);
	cam.collapseTileIndex = ReadU16(p);
	cam.savedTileByte = ReadU8(p);
	cam.patrolCounterA = ReadI16(p);
	cam.patrolCounterB = ReadI16(p);
	cam.horizonMirrorMask = ReadU16(p);
	cam.landingProx = ReadU16(p);
	cam.inputFlags = ReadU8(p);
	cam.wallContactState = ReadU8(p);
	cam.crashSoundFlag = ReadU16(p);
	cam.landingDelay = ReadU8(p);
	cam.roadDrawDisable = ReadU16(p);

	// Object state
	for (int i = 0; i < OBJ_SLOTS; i++)
		objs.posX[i] = ReadI32(p);
	for (int i = 0; i < OBJ_SLOTS; i++)
		objs.posY[i] = ReadI32(p);
	for (int i = 0; i < OBJ_SLOTS; i++)
		objs.posZ[i] = ReadI32(p);
	for (int i = 0; i < OBJ_SLOTS; i++)
		objs.velX[i] = ReadI32(p);
	for (int i = 0; i < OBJ_SLOTS; i++)
		objs.velY[i] = ReadI32(p);
	for (int i = 0; i < OBJ_SLOTS; i++)
		objs.velZ[i] = ReadI32(p);
	for (int i = 0; i < OBJ_SLOTS; i++)
		objs.motionTimer[i] = ReadU16(p);
	ReadBytes(p, objs.slotTable, OBJ_SLOTS);
	ReadBytes(p, objs.flagsTable, OBJ_SLOTS);
	ReadBytes(p, objs.typeTable, 256);
	ReadBytes(p, objs.rotSelector, 16);
	objs.currentRoom = ReadU8(p);

	// Workspace fields
	g_workspace.inventoryStackDepth = ReadU16(p);
	ReadBytes(p, g_workspace.inventoryStack, Workspace::INVENTORY_STACK_MAX);

	// Room geometry workspace
	for (int i = 0; i < Workspace::ROOM_SLOT_MAX; i++)
		g_workspace.vertLongX[i] = ReadI32(p);
	for (int i = 0; i < Workspace::ROOM_SLOT_MAX; i++)
		g_workspace.vertLongY[i] = ReadI32(p);
	for (int i = 0; i < Workspace::ROOM_SLOT_MAX; i++)
		g_workspace.vertLongZ[i] = ReadI32(p);
	for (int i = 0; i < Workspace::ROOM_SLOT_MAX; i++)
		g_workspace.vertLongVel[i] = ReadI32(p);
	for (int i = 0; i < Workspace::ROOM_SLOT_MAX; i++)
		g_workspace.edgeA[i] = ReadU16(p);
	for (int i = 0; i < Workspace::ROOM_SLOT_MAX; i++)
		g_workspace.edgeB[i] = ReadU16(p);
	g_workspace.roomVertHW = ReadU16(p);
	g_workspace.roomEdgeHW = ReadU16(p);

	// Script VM state
	vm.scriptRunning = ReadU16(p);
	vm.textSpeed = ReadU16(p);
	vm.attackFlag = ReadU16(p);
	vm.shipHireFlag = ReadU16(p);
	vm.endgameFlag = ReadU16(p);
	vm.flags = ReadU16(p);
	vm.pc = ReadI32(p);
	vm.callDepth = ReadI32(p);
	for (int i = 0; i < SCRIPT_CALL_STACK_DEPTH; i++)
		vm.callStack[i] = ReadI32(p);
	vm.frameCounter = ReadU32(p);
	vm.score = ReadU16(p);
	if (ReadU8(p) != 0)
		vm.flags |= ScriptVM::FLAG_EVENT_ACTIVE;
	else
		vm.flags &= ~ScriptVM::FLAG_EVENT_ACTIVE;
	vm.eventReturnPC = ReadI32(p);
	for (int i = 0; i < ScriptVM::WORD_VAR_TABLE_SIZE; i++)
		vm.wordVarTable[i] = ReadU16(p);
	for (int i = 0; i < ScriptVM::VAR_TABLE_SIZE; i++)
		vm.varTable[i] = ReadU32(p);
	for (int i = 0; i < ScriptVM::RNG_TABLE_SIZE; i++)
		vm.rngTable[i] = ReadU16(p);
	vm.rngPointer = ReadU16(p);
	vm.lastKeyByte = ReadU8(p);

	// Game struct palette/timer state
	game.palOverride89 = ReadU32(p);
	game.palOverride1011 = ReadU32(p);
	game.blackoutTimer = static_cast<int>(ReadI32(p));
	game.pal3FlashIndex = static_cast<int>(ReadI32(p));
	game.damageFlashTimer = static_cast<int>(ReadI32(p));
	game.damageFlashPtr = ReadU16(p);
	for (int i = 0; i < 2048; i++)
		game.randomBuf[i] = ReadU16(p);
	game.intro.mixerShadow = ReadU8(p); // $0624EA: YM mixer shadow

	// TileDetailState
	td.spinAngle = ReadU16(p);
	td.spinSpeed = ReadU16(p);
	td.spinBaseX = ReadI16(p);
	td.spinBaseZ = ReadI16(p);
	td.spinVertCount = ReadU8(p);
	td.tileProperty = ReadI8(p);
	td.cachedPosX = ReadU16(p);
	td.cachedPosY = ReadU16(p);
	td.cachedPosZ = ReadU16(p);
	td.currentTileIndex = ReadU16(p);
	td.cachedD7 = ReadU8(p);
	td.drawModeSplit = ReadU16(p);
	td.tileVertCount = ReadU8(p);

	(void)end;
	return true;
}

// Post-load reinitialisation (loc_048856, E1:12617-12643)

static void PostLoadReinit(Game &game)
{
	Camera &cam = g_workspace.cam;

	// (1) Palette reload
	InitGameplayPalette(game);

	// (2) ScriptVM pointer rewiring
	game.scriptVM.audio = &game.audio;
	game.scriptVM.mixerShadow = &game.intro.mixerShadow;
	game.scriptVM.objects = &g_workspace.objs;
	game.scriptVM.camera = &g_workspace.cam;

	// (3) Benson reset -- clear any in-progress scroll
	BensonInit(game.benson);
	game.benson.varTable = game.scriptVM.varTable;
	cam.pendingMsg = 0;
	cam.pendingMsgFlag = 0;

	// (4) Camera derived state recompute
	CameraComputeAltFactor(cam);
	CameraComputeTrig(cam, g_workspace.tileDetail);
	CameraBuildMatrix(cam);

	// (5) Tile geometry reload
	TileDetailLoadDirect(g_workspace.tileDetail, cam);

	// (6) Object active list rebuild
	ObjectsBuildActiveList(g_workspace.objs);

	// (7) Indoor room reload if needed
	if (cam.renderMode != 0)
	{
		int32_t savedX = cam.posX;
		int32_t savedY = cam.posY;
		int32_t savedZ = cam.posZ;
		RoomLoad(g_workspace.objs.currentRoom, 0, game.scriptVM);
		cam.roomChangedFlag = 0; // CLR.W $0624B8
		cam.posX = savedX;
		cam.posY = savedY;
		cam.posZ = savedZ;
		// E1:12639-12640: restore building base palette to VBL overrides
		game.palOverride89 = cam.palBase89;
		game.palOverride1011 = cam.palBase1011;
	}

	// (8) HUD indicator
	HudWeaponSightIndicator();

	// (9) E1:12622: MOVE.W #$FFFF,($0624F2).L -- suppress AMP_C silence
	cam.soundLock = 0xFFFF;
	// E1:12623: MOVE.W #$00FF,($0623B6).L -- invalidate tile cache
	// to force reload on next frame
	g_workspace.tileDetail.cachedPosX = 0x00FF;
	game.gameplayTickPhase = false;
}

// File I/O

static std::string MakeSavePath(const std::string &dir, int slot)
{
	char filename[16];
#if SECOND_CITY
	std::snprintf(filename, sizeof(filename), "SEC%d.MGS", slot);
#else
	std::snprintf(filename, sizeof(filename), "MER%d.MGS", slot);
#endif
	return dir + filename;
}

std::string SaveLoadInitPath()
{
	char *path = SDL_GetPrefPath("MercenaryPort", "Mercenary");
	if (!path)
		return std::string();
	std::string result(path);
	SDL_free(path);
	return result;
}

bool SaveToFile(const Game &game, const std::string &dir, int slot)
{
	if (dir.empty())
		return false;

	uint8_t buf[MAX_SAVE_SIZE];
	int size = SerialiseState(game, buf);

	std::string path = MakeSavePath(dir, slot);
	FILE *fp = std::fopen(path.c_str(), "wb");
	if (!fp)
		return false;

	bool ok = (std::fwrite(buf, 1, size, fp) == static_cast<size_t>(size));
	std::fclose(fp);
	return ok;
}

bool LoadFromFile(Game &game, const std::string &dir, int slot)
{
	if (dir.empty())
		return false;

	std::string path = MakeSavePath(dir, slot);
	FILE *fp = std::fopen(path.c_str(), "rb");
	if (!fp)
		return false;

	uint8_t buf[MAX_SAVE_SIZE];
	int bytesRead = static_cast<int>(std::fread(buf, 1, MAX_SAVE_SIZE, fp));
	std::fclose(fp);

	if (bytesRead < 8)
		return false;

	if (std::memcmp(buf, SAVE_MAGIC, 4) != 0)
		return false;

	if (!DeserialiseState(game, buf, bytesRead))
		return false;

	PostLoadReinit(game);

	return true;
}

// UI state machine

void SaveLoadTrigger(SaveLoadState &sl, bool isSave)
{
	if (sl.phase != SL_IDLE)
		return;
	sl.phase = SL_SHOW_PROMPT;
	sl.isSave = isSave;
	sl.slot = -1;
	sl.resultTimer = 0;
}

bool SaveLoadActive(const SaveLoadState &sl) { return sl.phase != SL_IDLE; }

void SaveLoadTick(SaveLoadState &sl, Game &game, const SaveLoadInput &input)
{
	switch (sl.phase)
	{
	case SL_IDLE:
		break;

	case SL_SHOW_PROMPT:
		// Display "SAVE NUMBER 0-9" or "LOAD NUMBER 0-9"
		// Original: BSR sub_0441E8, BSR sub_0450C0 (which busy-waits
		// for pendingMsgFlag == 0 before displaying)
		if (g_workspace.cam.pendingMsgFlag != 0)
			break;
		InteractionPing(game);
		MessageDisplay(game, sl.isSave ? MSG_SAVE_PROMPT : MSG_LOAD_PROMPT);
		sl.phase = SL_AWAIT_DIGIT;
		break;

	case SL_AWAIT_DIGIT:
		if (input.digitKey >= 0 && input.digitKey <= 9)
		{
			sl.slot = input.digitKey;
			sl.phase = SL_SHOW_CONFIRM;
		}
		else if (input.anyKeyDown)
		{
			// Cancel on any non-digit key -- original: SUBI.B #$30,D0 /
			// CMPI.B #$0A / BCC loc_0451CA (E1:6720-6722)
			// loc_0451CA displays blank message (#50)
			MessageDisplay(game, MSG_BLANK);
			sl.phase = SL_RESULT;
			sl.resultTimer = RESULT_FRAMES;
		}
		break;

	case SL_SHOW_CONFIRM:
		// Wait for previous message to finish, then show confirm prompt
		// Original: BSR sub_0441E8, MOVEQ #56,D1, BSR sub_0450C0
		if (g_workspace.cam.pendingMsgFlag == 0)
		{
			InteractionPing(game);
			MessageDisplay(game, MSG_CONFIRM);
			sl.phase = SL_AWAIT_CONFIRM;
		}
		break;

	case SL_AWAIT_CONFIRM:
		if (input.returnKey)
		{
			sl.phase = SL_EXECUTE;
		}
		else if (input.anyKeyDown)
		{
			// Cancel on any non-Return key -- original: CMPI.B #$0D /
			// BNE loc_0451CA (E1:6727-6728)
			MessageDisplay(game, MSG_BLANK);
			sl.phase = SL_RESULT;
			sl.resultTimer = RESULT_FRAMES;
		}
		break;

	case SL_EXECUTE:
	{
		// Wait for Benson to finish the confirm prompt before executing
		// Original: sub_0450B4 busy-waits for $062478 == 0
		if (g_workspace.cam.pendingMsgFlag != 0)
			break;

		// Original: BSR sub_0441E8 before file I/O (E1:6729/6806)
		InteractionPing(game);

		bool ok;
		if (sl.isSave)
			ok = SaveToFile(game, game.savePath, sl.slot);
		else
			ok = LoadFromFile(game, game.savePath, sl.slot);

		// On success: blank message (original: MOVEQ #50)
		// On failure: "DISK ERROR" (original: MOVEQ #51)
		MessageDisplay(game, ok ? MSG_BLANK : MSG_DISK_ERROR);

		sl.resultTimer = RESULT_FRAMES;
		sl.phase = SL_RESULT;
		break;
	}

	case SL_RESULT:
		sl.resultTimer--;
		if (sl.resultTimer <= 0)
		{
			sl.phase = SL_IDLE;
		}
		break;
	}
}
