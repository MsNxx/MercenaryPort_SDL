// Camera movement dispatcher -- sub_047D8C (E1)

#include "game/Camera.h"
#include "game/Interior.h"
#include "game/ObjectSlots.h"
#include "game/TileDetail.h"
#include "game/Workspace.h"
#include "renderer/Hud.h"
#include "renderer/LogMath.h"
#include "renderer/Palette.h"

// CameraMovementTick -- dispatches flight/grounded/elevator handlers

void CameraMovementTick(Camera &cam, ScriptVM &vm)
{
	// E1:11480-11481: sub_047D8C checks $06244C first
	// When elevatorActive != 0, dispatch to elevator altitude handler
	// (loc_048058)
	if (cam.elevatorActive != 0)
	{
		// loc_048058 (E1:11729): elevator altitude computation
		// Computes camera altitude from elevatorPhase counter
		uint16_t phase = cam.elevatorPhase;
		uint16_t d0 = phase;
		uint16_t d1w = static_cast<uint16_t>(phase & 0x00FF);

		// E1:11733: MULU.W #$F83E,D1 -- multiply low byte by $F83E
		uint32_t d1l = static_cast<uint32_t>(d1w) * 0xF83Eu;
		// E1:11734: SWAP D1 -- take high word of result
		d1w = static_cast<uint16_t>(d1l >> 16);
		// E1:11735: ADDI.W #$0040,D1
		d1w += 0x0040;
		// E1:11736-11738: (d0 >> 1) & $FF80, add to D1
		uint16_t shifted = static_cast<uint16_t>((d0 >> 1) & 0xFF80);
		d1w += shifted;
		// E1:11739: EXT.L D1 -- sign-extend to 32-bit
		int32_t altitude = static_cast<int32_t>(static_cast<int16_t>(d1w));

		// E1:11740-11745: if altitude >= $0600, subtract $0700
		// If $0624A4 (landingProx/antigravity) is set, add $0040FF00
		if (d1w >= 0x0600)
		{
			altitude -= 0x0700;
			if (cam.landingProx != 0)
				altitude += 0x0040FF00;
		}

		// E1:11748: store altitude
		cam.posY = altitude;

		// E1:11736-11737: D0_shifted = (phase >> 1) & $FF80
		// Completion checks compare against this "phase row"
		uint16_t phaseRow = static_cast<uint16_t>((phase >> 1) & 0xFF80);

		// E1:11749-11750: check elevatorActive sign
		if (static_cast<int16_t>(cam.elevatorActive) < 0)
		{
			// loc_048130 (E1:11776+): DESCENDING path (entering from surface)
			// elevatorActive = $8000, phase counts DOWN from $0D84

			if (cam.renderMode != 0)
			{
				// E1:11794: CMPI.W #$0080,D1 -- altitude check
				// E1:11795: BGT loc_048174 -- if > $80, return without
				// completing
				if (altitude <= 0x0080)
				{
					// E1:11796: clear elevatorActive
					// Game.cpp VBLHandler detects 1->0 transition and silences
					// channels B+C (E1:11797-11802)
					cam.elevatorActive = 0;
					// E1:11803: snap posY to $0080 (floor level)
					cam.posY = 0x00000080;
				}
			}
			else
			{
				// E1:11779: compare phaseRow against $0400
				// Using LONG compare in original -- phase row must be exactly
				// $0400
				if (phaseRow == 0x0400)
				{
					// E1:11781: activate indoor rendering
					cam.renderMode = 1;
					// E1:11784: MOVE.B #$20,($00062492) -- tileProperty
					g_workspace.tileDetail.tileProperty = 0x20;
					// E1:11785: BSR sub_044B5A -- room setup
					//   D0 = current room ($062452), D1 = 0
					RoomLoad(g_workspace.objs.currentRoom, 0, vm);
					// E1:11786-11787: snap posX/posZ low words to $0800
					// (middle of tile)
					cam.posX = (cam.posX & 0xFFFF0000) | 0x0800;
					cam.posZ = (cam.posZ & 0xFFFF0000) | 0x0800;
					// E1:11788: clear antigrav flag
					cam.landingProx = 0;
				}
			}
		}
		else
		{
			// ASCENDING path (exiting to surface) -- fall through from
			// E1:11750. elevatorActive = $0001, phase counts UP from $0042

			// E1:11751-11752: only act when renderMode != 0
			if (cam.renderMode != 0)
			{
				// E1:11753: phaseRow == $0600?
				if (phaseRow == 0x0600)
				{
					// E1:11755-11758: snap position from elevatorDest
					// ($0624E6 stores word + word = X + Z from transition
					// table)
					uint16_t destX =
						static_cast<uint16_t>(cam.elevatorDest >> 16);
					uint16_t destZ = static_cast<uint16_t>(cam.elevatorDest);
					// E1:11755: ($0624E8) -> ($0623AE) high word (posZ)
					cam.posZ = (cam.posZ & 0x0000FFFF) |
							   (static_cast<uint32_t>(destZ) << 16);
					// E1:11756: ($0624E6) -> ($0623A6) high word (posX)
					cam.posX = (cam.posX & 0x0000FFFF) |
							   (static_cast<uint32_t>(destX) << 16);
					// E1:11757-11759: posX/Z low words set to $7200 (middle of
					// tile)
					cam.posX = (cam.posX & 0xFFFF0000) | 0x7200;
					cam.posZ = (cam.posZ & 0xFFFF0000) | 0x7200;

					// E1:11760-11764: room 8 special case (antigravity grant)
					if (g_workspace.objs.currentRoom == 0x0008)
					{
						// E1:11762: add $0040FF00 to posY
						cam.posY = static_cast<int32_t>(
							static_cast<uint32_t>(cam.posY) + 0x0040FF00);
						// E1:11763: landingProx = $003F
						cam.landingProx = 0x003F;
						// E1:11764: altFactor = $00041000
						cam.altFactor = 0x00041000;
					}

					// E1:11767: clear indoor mode.  NOTE: the original does
					// NOT clear elevatorActive here -- the elevator engine
					// keeps running until phase reaches $0E00, where
					// sub_043034's cleanup (loc_04314A, E1:3528) clears it
					cam.renderMode = 0;
					// E1:11768: clear room
					g_workspace.objs.currentRoom = 0;
					// E1:11769: BSR sub_043C14 -- rebuild active list
					ObjectsBuildActiveList(g_workspace.objs);
					HudFactionLED();
					// E1:11770: BSR sub_04802C -- reset BASE palette
					cam.palBase89 = PAL_DEFAULT_89;
					cam.palBase1011 = PAL_DEFAULT_1011;
					// E1:11771: MOVE.B #$FF,($000623B7) -- low byte of
					// cachedPosX.  Forces tile change detection on next
					// frame.  Original only writes this one byte
					g_workspace.tileDetail.cachedPosX =
						(g_workspace.tileDetail.cachedPosX & 0xFF00) | 0x00FF;
					// E1:11772: BSR sub_044630 -- tile change handler
					TileDetailLoadDirect(g_workspace.tileDetail, cam);
					// E1:11773: MOVE.B #$01,($00062426) -- cachedD7
					g_workspace.tileDetail.cachedD7 = 0x01;
					// E1:11774: BRA sub_044176 -- reset thrust/velocity
					cam.thrustAccum = LOG_FLOAT_ZERO;
					cam.vertVelocity = LOG_FLOAT_ZERO;
				}
			}
		}
		return;
	}

	// E1:11482-11483: check $0623FC (renderMode)
	// Nonzero -> indoor movement (loc_048760)
	if (cam.renderMode != 0)
	{
		// loc_048760 (E1:12517): indoor movement handler
		// Turn left/right from input, forward/backward when free-roaming
		uint8_t input = cam.inputFlags;

		// E1:12534-12542: turn left/right
		uint16_t turnRate = cam.turnRate;
		if (turnRate == 0)
			turnRate = 0x0008; // default indoor turn rate
		if (input & 0x08)
			cam.heading = static_cast<uint16_t>(cam.heading + turnRate);
		if (input & 0x04)
			cam.heading = static_cast<uint16_t>(cam.heading - turnRate);

		// E1:12522-12531: forward/backward only when flightState < 0
		// (on foot, not inside a vehicle)
		if (static_cast<int16_t>(cam.flightState) < 0)
		{
			// E1:12524-12526: forward (bit 1)
			if (input & 0x02)
			{
				// sub_0487CE (E1:12559-12612): move forward
				// deltaX = linearise(sin(heading) * speed)
				// deltaZ = linearise(cos(heading) * speed)
				uint32_t d2 = LogMultiply(cam.sinHeading, cam.movementSpeed);
				uint32_t d3 = LogMultiply(cam.cosHeading, cam.movementSpeed);
				cam.posX += LogFloatToInt(d2);
				cam.posZ += LogFloatToInt(d3);

				// E1:12610-12612: decrement landingDelay
				// is at the END of sub_0487CE -- it only runs when
				// the player is actually moving forward
				if (cam.landingDelay > 0)
					cam.landingDelay--;
			}
			// E1:12529-12531: backward (bit 0)
			if (input & 0x01)
			{
				// sub_0487CC: NEG.W D1 then fall through to sub_0487CE
				// Negate speed -> move backward.  Falls through to same
				// sub_0487CE code including the landingDelay decrement
				uint32_t negSpeed = cam.movementSpeed;
				negSpeed = (negSpeed & 0xFFFF0000u) |
						   static_cast<uint16_t>(
							   -static_cast<int16_t>(negSpeed & 0xFFFF));
				uint32_t d2 = LogMultiply(cam.sinHeading, negSpeed);
				uint32_t d3 = LogMultiply(cam.cosHeading, negSpeed);
				cam.posX += LogFloatToInt(d2);
				cam.posZ += LogFloatToInt(d3);

				// E1:12610-12612: same decrement (sub_0487CC falls
				// through to sub_0487CE which includes this)
				if (cam.landingDelay > 0)
					cam.landingDelay--;
			}
		}

		return;
	}

	// Outdoor: dispatch to flight state handlers
	CameraFlightTick(cam);
}
