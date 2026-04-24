// HUD instrument panel -- sub_043710, sub_043A0E, sub_043B54 (E1)

#include "renderer/Hud.h"

#include "data/GameData.h"
#include "data/Gen_MData.h"
#include "game/ObjectSlots.h"
#include "game/Workspace.h"
#include "renderer/FrameBuffer.h"

// Bitplane byte writer -- translates MOVE.B at a bitplane byte offset
// into palette index bit operations in our indexed framebuffer

static void WriteBitplaneByte(uint8_t *indexBuf, int screenOff, uint8_t val)
{
	constexpr int STRIDE = 160;

	int scanline = screenOff / STRIDE;
	int byteInLine = screenOff % STRIDE;
	int wordGroup = byteInLine / 8;
	int byteInGroup = byteInLine % 8;
	int plane = byteInGroup / 2; // 0-3
	int hiLo = byteInGroup & 1;	 // 0 = hi (pixels 0-7), 1 = lo (8-15)
	int pixelX = wordGroup * 16 + hiLo * 8;
	uint8_t mask = static_cast<uint8_t>(1 << plane);

	if (scanline < 0 || scanline >= FB_HEIGHT)
		return;

	uint8_t *line = indexBuf + scanline * FB_WIDTH + pixelX;
	for (int px = 0; px < 8; px++)
	{
		if (val & (0x80 >> px))
			line[px] |= mask;
		else
			line[px] &= ~mask;
	}
}

// Write a 16-bit word to two consecutive bitplane bytes (big-endian)
static void WriteBitplaneWord(uint8_t *indexBuf, int screenOff, uint16_t val)
{
	WriteBitplaneByte(indexBuf, screenOff, static_cast<uint8_t>(val >> 8));
	WriteBitplaneByte(indexBuf, screenOff + 1, static_cast<uint8_t>(val));
}

// Write a 32-bit longword to four consecutive bitplane bytes (big-endian)
static void WriteBitplaneLong(uint8_t *indexBuf, int screenOff, uint32_t val)
{
	WriteBitplaneByte(indexBuf, screenOff, static_cast<uint8_t>(val >> 24));
	WriteBitplaneByte(indexBuf, screenOff + 1, static_cast<uint8_t>(val >> 16));
	WriteBitplaneByte(indexBuf, screenOff + 2, static_cast<uint8_t>(val >> 8));
	WriteBitplaneByte(indexBuf, screenOff + 3, static_cast<uint8_t>(val));
}

// sub_043826 (E1:4104): single digit blitter (unsigned)

static void Sub043826(uint8_t *indexBuf, int glyphIndex, int &a0)
{
	if (glyphIndex < 0 || glyphIndex >= gen_m::DIGIT_GLYPH_COUNT)
		glyphIndex = 0;

	const uint8_t *glyph =
		gen_m::DIGIT_FONT + glyphIndex * gen_m::DIGIT_GLYPH_STRIDE;

	// E1:4108: (A0)+ = (A1)+   -- row 0, A0 auto-increments
	WriteBitplaneByte(indexBuf, a0, glyph[0]);
	a0++;

	// E1:4109-4112: rows 1-4 at offsets 159, 319, 479, 639 from A0
	// (which has already been incremented by 1)
	WriteBitplaneByte(indexBuf, a0 + 159, glyph[1]);
	WriteBitplaneByte(indexBuf, a0 + 319, glyph[2]);
	WriteBitplaneByte(indexBuf, a0 + 479, glyph[3]);
	WriteBitplaneByte(indexBuf, a0 + 639, glyph[4]);

	// E1:4113-4119: check if A0 is even.  If even, skip 6
	if ((a0 & 1) == 0)
		a0 += 6;
}

// sub_043866 (E1:4134): inverted digit blitter (negative values)
// Writes $7F fill to (A0) and XOR-inverted glyph to -5(A0)

static void Sub043866(uint8_t *indexBuf, int glyphIndex, int &a0)
{
	if (glyphIndex < 0 || glyphIndex >= gen_m::DIGIT_GLYPH_COUNT)
		glyphIndex = 0;

	const uint8_t *glyph =
		gen_m::DIGIT_FONT + glyphIndex * gen_m::DIGIT_GLYPH_STRIDE;

	// E1:4135-4165: for each of the 5 rows
	// Row 0: (A0)+ = $7F; then glyph^$7F -> -4 from glyph byte
	int glyphByte0 = a0;
	WriteBitplaneByte(indexBuf, a0, 0x7F);
	a0++;
	WriteBitplaneByte(indexBuf, glyphByte0 - 4, glyph[0] ^ 0x7F);

	// Rows 1-4: same pattern, -4 from glyph position
	for (int row = 1; row < 5; row++)
	{
		int glyphPos = glyphByte0 + row * 160;
		WriteBitplaneByte(indexBuf, glyphPos, 0x7F);
		WriteBitplaneByte(indexBuf, glyphPos - 4, glyph[row] ^ 0x7F);
	}

	// Even/odd advance (same as sub_043826)
	if ((a0 & 1) == 0)
		a0 += 6;
}

// sub_0438CC (E1:4176): normal digit blitter, clears -5 offset neighbour

static void Sub0438CC(uint8_t *indexBuf, int glyphIndex, int &a0)
{
	if (glyphIndex < 0 || glyphIndex >= gen_m::DIGIT_GLYPH_COUNT)
		glyphIndex = 0;

	const uint8_t *glyph =
		gen_m::DIGIT_FONT + glyphIndex * gen_m::DIGIT_GLYPH_STRIDE;

	// Row 0: (A0)+ = glyph[0]; then $00 at glyphByte0 - 4
	// (plane 0 of the same pixels -- clears any leftover inverted glyph)
	int glyphByte0 = a0;
	WriteBitplaneByte(indexBuf, a0, glyph[0]);
	a0++;
	WriteBitplaneByte(indexBuf, glyphByte0 - 4, 0x00);

	// Rows 1-4: same -4 displacement from the glyph byte position
	for (int row = 1; row < 5; row++)
	{
		int glyphPos = glyphByte0 + row * 160;
		WriteBitplaneByte(indexBuf, glyphPos, glyph[row]);
		WriteBitplaneByte(indexBuf, glyphPos - 4, 0x00);
	}

	if ((a0 & 1) == 0)
		a0 += 6;
}

// sub_043814 (E1:4095): unsigned two-digit renderer via BCD table

static void Sub043814(uint8_t *indexBuf, int d1, int &a0)
{
	// E1:4096: D1 += D1 (word index)
	int idx = (d1 & 0xFF) * 2; // byte index into BCD table
	if (idx + 1 >= 200)
		idx = 0;

	// E1:4097-4098: A2 = BCD table + D1
	int tens = gen_e1::DIGIT_BCD_TABLE[idx];
	int units = gen_e1::DIGIT_BCD_TABLE[idx + 1];

	// E1:4100: BSR sub_043826 (tens)
	Sub043826(indexBuf, tens, a0);

	// E1:4103: falls through to sub_043826 (units)
	Sub043826(indexBuf, units, a0);
}

// sub_04384E (E1:4122): signed two-digit renderer

static void Sub04384E(uint8_t *indexBuf, int16_t d1, int &a0)
{
	// E1:4123: EXT.W D1 -- sign-extend low byte to word
	int8_t signedByte = static_cast<int8_t>(d1 & 0xFF);
	int16_t value = signedByte;

	if (value >= 0)
	{
		// E1:4124: BPL -> loc_0438BA (positive path)
		// loc_0438BA does BCD lookup then calls sub_0438CC for each digit
		int idx = (value & 0xFF) * 2;
		if (idx + 1 >= 200)
			idx = 0;
		int tens = gen_e1::DIGIT_BCD_TABLE[idx];
		int units = gen_e1::DIGIT_BCD_TABLE[idx + 1];
		Sub0438CC(indexBuf, tens, a0);
		Sub0438CC(indexBuf, units, a0);
	}
	else
	{
		// E1:4125: NEG.W D1 -- make positive
		value = -value;
		int idx = (value & 0xFF) * 2;
		if (idx + 1 >= 200)
			idx = 0;
		int tens = gen_e1::DIGIT_BCD_TABLE[idx];
		int units = gen_e1::DIGIT_BCD_TABLE[idx + 1];
		Sub043866(indexBuf, tens, a0);
		Sub043866(indexBuf, units, a0);
	}
}

// Location meter
// sub_043710, E1:4029-4035

static void RenderLocation(uint8_t *indexBuf, int32_t posX, int32_t posZ)
{
	// E1:4029-4030: LEA $DD45(A5), A0
	// A5 = screen bank base.  $DD45 - $8000 = $5D45
	int a0 = 0x5D45;

	// E1:4031: MOVE.W ($0623A6), D1 -- high word of posX
	int16_t d1 = static_cast<int16_t>(posX >> 16);

	// E1:4032: BSR sub_04384E
	Sub04384E(indexBuf, d1, a0);

	// E1:4033: ADDA.W #7, A0
	a0 += 7;

	// E1:4034: MOVE.W ($0623AE), D1 -- high word of posZ
	d1 = static_cast<int16_t>(posZ >> 16);

	// E1:4035: BSR sub_04384E
	Sub04384E(indexBuf, d1, a0);
}

// Altitude meter
// sub_043710, E1:4009-4028

static void RenderAltitude(uint8_t *indexBuf, int32_t posY)
{
	// E1:4014: LEA $DD6C(A5), A0
	int a0 = 0x5D6C;

	// E1:4017: MOVE.L ($0623AA), D2 -- altitude
	uint32_t d2 = static_cast<uint32_t>(posY);

	// E1:4018-4019: D1 = D2; SWAP D1 -> high word
	uint16_t d1 = static_cast<uint16_t>(d2 >> 16);

	// E1:4020: BSR sub_043814 (ten-thousands, thousands)
	Sub043814(indexBuf, d1, a0);

	// E1:4021: MULU #$64, D2 -- 68000 multiplies D2's LOW WORD by $64,
	// giving a 32-bit result in D2
	d2 = (d2 & 0xFFFF) * 100;

	// E1:4022-4023: D1 = D2; SWAP D1
	d1 = static_cast<uint16_t>(d2 >> 16);

	// E1:4024: BSR sub_043814 (hundreds, tens)
	Sub043814(indexBuf, d1, a0);

	// E1:4025-4027: D2 *= 10; D0 = D2; SWAP D0
	d2 = (d2 & 0xFFFF) * 10;
	uint16_t d0 = static_cast<uint16_t>(d2 >> 16);

	// E1:4028: BSR sub_043826 (units)
	Sub043826(indexBuf, d0, a0);
}

// Speed indicator (E1:4036-4092)
// Log-float -> linear conversion, sign character + 4 digits

static void RenderSpeed(uint8_t *indexBuf, uint32_t vertVelocity)
{
	// $043770: LEA $DD8D(A5), A0
	int a0 = 0x5D8D;

	// $043776: MOVE.L ($062344), D3
	// Track D3 as a full 32-bit register, operating on words/bytes as the
	// original does
	uint32_t d3 = vertVelocity;

	// [E1:4039-4058: sound handling omitted -- done in VBLHandler]

	// $0437D6: MOVEQ #11, D0 -- default '+' glyph
	int signGlyph = 11;

	// $0437D8: TST.W D3 -- test LOW WORD only
	// $0437DA: BPL loc_0437E0
	int16_t d3w = static_cast<int16_t>(d3 & 0xFFFF);
	if (d3w < 0)
	{
		// $0437DC: MOVEQ #12, D0 -- '-' glyph
		signGlyph = 12;
		// $0437DE: NEG.W D3 -- negate LOW WORD only
		uint16_t lo = static_cast<uint16_t>(-d3w);
		d3 = (d3 & 0xFFFF0000u) | lo;
	}

	// $0437E0: BSR sub_043826 -- render sign character
	Sub043826(indexBuf, signGlyph, a0);

	// Log-to-linear conversion ($0437E4-$0437FA)

	// $0437E4: MOVE.L D3, D7
	uint32_t d7 = d3;

	// $0437E6: BPL loc_0437EC -- if D3 >= 0 (as longword), proceed
	if (static_cast<int32_t>(d3) < 0)
	{
		// $0437E8: CLR.W D3 -- clear low word of D3
		d3 &= 0xFFFF0000u;
		// $0437EA: BRA loc_0437FC -- skip to digit extraction
	}
	else
	{
		// $0437EC: SWAP D7 -- exponent moves to low word
		d7 = (d7 >> 16) | (d7 << 16);

		// $0437EE: NEG.W D7 -- negate low word (the exponent)
		{
			uint16_t lo = static_cast<uint16_t>(d7);
			lo = static_cast<uint16_t>(-static_cast<int16_t>(lo));
			d7 = (d7 & 0xFFFF0000u) | lo;
		}

		// $0437F0: ADDI.W #$000E, D7
		{
			int16_t lo = static_cast<int16_t>(d7 & 0xFFFF);
			lo += 14;
			d7 = (d7 & 0xFFFF0000u) | static_cast<uint16_t>(lo);

			// $0437F4: BMI loc_0437FC -- if result < 0, overflow
			if (lo < 0)
			{
				// overflow -- skip to digit extraction with D3 unchanged
			}
			else
			{
				// $0437F6: EXT.L D3 -- sign-extend low word to longword
				d3 = static_cast<uint32_t>(
					static_cast<int32_t>(static_cast<int16_t>(d3 & 0xFFFF)));

				// $0437F8: ASL.L #2, D3
				d3 <<= 2;

				// $0437FA: ASR.L D7, D3 -- arithmetic shift right
				int shift = static_cast<int>(d7 & 0xFFFF);
				if (shift > 31)
					shift = 31;
				d3 = static_cast<uint32_t>(static_cast<int32_t>(d3) >> shift);
			}
		}
	}

	// Digit extraction ($0437FC-$04380E)

	// $0437FC: MOVEQ #0, D1
	// $0437FE: ROR.W #8, D3 -- rotate low word right by 8
	{
		uint16_t lo = static_cast<uint16_t>(d3);
		lo = static_cast<uint16_t>((lo >> 8) | (lo << 8));
		d3 = (d3 & 0xFFFF0000u) | lo;
	}

	// $043800: MOVE.B D3, D1 -- low byte (= original high byte of low word)
	int d1 = d3 & 0xFF;

	// $043802: BSR sub_043814 -- first digit pair
	Sub043814(indexBuf, d1, a0);

	// $043806: MULU.W #$0064, D3 -- multiply low word by 100, 32-bit result
	d3 = static_cast<uint16_t>(d3) * 100u;

	// $04380A: SWAP D3 -- high word of product moves to low word
	d3 = (d3 >> 16) | (d3 << 16);

	// $04380C: MOVE.W D3, D1 -- low word to D1 (sub_043814 uses low byte)
	d1 = d3 & 0xFF;

	// $04380E: BSR sub_043814 -- second digit pair
	Sub043814(indexBuf, d1, a0);
}

// Compass strip
// sub_043A0E, E1:4330-4387

static void RenderCompass(uint8_t *indexBuf, uint16_t heading)
{
	// E1:4331-4332: D0 = heading, mask to 10 bits
	int d0 = heading & 0x03FF;

	// E1:4333: MULU #$9000, D0
	uint32_t prod = static_cast<uint32_t>(d0) * 0x9000;

	// E1:4334: SWAP D0 -> high word = entry index
	d0 = static_cast<int>((prod >> 16) & 0xFFFF);

	// E1:4336-4341: SUBI #15; if negative, add $0240
	d0 -= 15;
	if (d0 < 0)
		d0 += 0x0240;

	// E1:4343-4347: D0 * 10 -> byte offset; A1 = compass tape base + offset
	int tapeOff = d0 * 10;
	if (tapeOff < 0 ||
		tapeOff + 10 > static_cast<int>(sizeof(gen_m::COMPASS_TAPE)))
		return;

	const uint8_t *tape = gen_m::COMPASS_TAPE + tapeOff;

	// E1:4349: original selects the back buffer via EORI.L #$10000
	// Caller already passes VBLTarget, so no flip needed here

	// E1:4351-4355: copy 5 words to column 1 offsets
	static const int col1Off[5] = {0x6ACE, 0x6B6E, 0x6C0E, 0x6CAE, 0x6D4E};
	for (int row = 0; row < 5; row++)
	{
		uint16_t word = (tape[row * 2] << 8) | tape[row * 2 + 1];
		WriteBitplaneWord(indexBuf, col1Off[row], word);
	}

	// E1:4356-4361: second column.  A1 auto-incremented by 10 after
	// reading 5 words, then ADDA.W #$96 adds 150 more.  Total = 160
	int tapeOff2 = tapeOff + 160;
	if (tapeOff2 + 10 > static_cast<int>(sizeof(gen_m::COMPASS_TAPE)))
		return;

	const uint8_t *tape2 = gen_m::COMPASS_TAPE + tapeOff2;
	static const int col2Off[5] = {0x6AD6, 0x6B76, 0x6C16, 0x6CB6, 0x6D56};
	for (int row = 0; row < 5; row++)
	{
		uint16_t word = (tape2[row * 2] << 8) | tape2[row * 2 + 1];
		WriteBitplaneWord(indexBuf, col2Off[row], word);
	}

	// E1:4362-4365: direction tick marks
	// D1 = displayOffset * 4 (from E1:4343 ASL.W #2,D1)
	// E1:4362: ASL.W #1,D1 -> D1 = displayOffset * 8
	// E1:4363: NEG.W D1
	// E1:4364: ANDI.W #$0078,D1
	int d1 = d0 * 4;	 // D1 after E1:4343
	d1 = d1 * 2;		 // E1:4362: ASL.W #1,D1
	d1 = (-d1) & 0x0078; // E1:4363-4364: NEG.W + ANDI

	// E1:4365: ADDI.L #dat_043AD4,D1 -> A1 = tick table + D1
	if (d1 + 8 > 128)
		return;

	const uint8_t *ticks = gen_e1::COMPASS_TICK_TABLE + d1;
	uint16_t tickWords[4];
	for (int i = 0; i < 4; i++)
		tickWords[i] = (ticks[i * 2] << 8) | ticks[i * 2 + 1];

	// E1:4367-4386: each tick word written to 4 offsets
	// (2 above compass strip, 2 below)
	static const int tickOff[4][4] = {
		{0x67AE, 0x67B6, 0x710E, 0x7116},
		{0x684E, 0x6856, 0x706E, 0x7076},
		{0x68EE, 0x68F6, 0x6FCE, 0x6FD6},
		{0x698E, 0x6996, 0x6F2E, 0x6F36},
	};

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
			WriteBitplaneWord(indexBuf, tickOff[i][j], tickWords[i]);
	}
}

// Elevation strip
// sub_043B54, E1:4454-4505

static void RenderElevation(uint8_t *indexBuf, uint16_t pitch)
{
	// E1:4455-4456: D0 = pitch; BCHG #9
	int d0 = pitch;
	d0 ^= 0x0200;

	// E1:4457-4459: ANDI #$03FF; MULU #$9000; SWAP
	d0 &= 0x03FF;
	uint32_t prod = static_cast<uint32_t>(d0) * 0x9000;
	d0 = static_cast<int>((prod >> 16) & 0xFFFF);

	// E1:4461-4466: SUBI #15; if negative, add $0240
	d0 -= 15;
	if (d0 < 0)
		d0 += 0x0240;

	// E1:4467-4469: D0 <<= 2 (= D0 * 4); A1 = $04F200 + D0
	int byteOff = d0 * 4;
	if (byteOff + 128 > gen_m::ELEV_STRIP_BYTES)
		return;

	const uint8_t *data = gen_m::ELEVATION_STRIP + byteOff;

	// E1:4470-4472: original selects the back buffer; caller passes VBLTarget

	// E1:4473-4505: write 32 longwords to consecutive scanlines
	// Destination offsets: $5D2C, $5DCC, $5E6C, ... stepping by $A0
	int destOff = 0x5D2C;
	for (int row = 0; row < 32; row++)
	{
		uint32_t lw = (data[row * 4] << 24) | (data[row * 4 + 1] << 16) |
					  (data[row * 4 + 2] << 8) | data[row * 4 + 3];
		WriteBitplaneLong(indexBuf, destOff, lw);
		destOff += 0xA0; // next scanline
	}
}

void HudRenderInstruments(uint8_t *indexBuf, const Camera &cam)
{
	// E1:4010-4011: altitude high word > $7F -> blank all numeric instruments
	uint16_t altWord =
		static_cast<uint16_t>(static_cast<uint32_t>(cam.posY) >> 16);
	if (altWord <= 0x007F)
	{
		RenderLocation(indexBuf, cam.posX, cam.posZ);
		RenderAltitude(indexBuf, cam.posY);
		RenderSpeed(indexBuf, cam.vertVelocity);
	}

	RenderCompass(indexBuf, cam.heading);
	RenderElevation(indexBuf, cam.pitch);
}

// sub_042416 (E1:2245-2257): SIGHTS reticle overlay
// Clears planes 0-1 at two 8-pixel dashes centered on the viewport

void HudSightsReticle(uint8_t *indexBuf)
{
	// E1:2246-2248: TST.B ($0631DC); BMI loc_042420; RTS
	int8_t sightsFlag =
		static_cast<int8_t>(g_workspace.objs.flagsTable[OBJ_SIGHTS]);
	if (sightsFlag >= 0)
		return;

	// loc_042420 (E1:2250-2257): clear planes 0-1 at reticle positions
	// Four MOVEP.W D0,$xxxx(A0) with D0=0:
	//   $2A28 -> scanline 67, pixels 144-151
	//   $2A31 -> scanline 67, pixels 168-175
	//   $2AC8 -> scanline 68, pixels 144-151
	//   $2AD1 -> scanline 68, pixels 168-175
	for (int y = 67; y <= 68; y++)
	{
		uint8_t *line = indexBuf + y * FB_WIDTH;
		for (int x = 144; x <= 151; x++)
			line[x] &= ~3;
		for (int x = 168; x <= 175; x++)
			line[x] &= ~3;
	}
}

// sub_040F9A (E1:637-686): weapon sight indicator
// 5x6 pixel block at X 199-203, walk=palette 15, run=palette 4

void HudWeaponSightIndicator()
{
	// E1:637-639: CMPI.W #$0003,($062400) -- check weapon type
	uint16_t type = static_cast<uint16_t>(g_workspace.cam.movementSpeed >> 16);
	uint8_t colour = (type == 3) ? 15 : 4;

	// loc_04100A: draw into both buffers (A0=$07DD80, A1=$06DD80)
	// 6 scanlines (DBF D0=5), 5 pixels wide
	uint8_t *bufs[2] = {g_workspace.frameBuf[0], g_workspace.frameBuf[1]};
	for (int b = 0; b < 2; b++)
	{
		uint8_t *fb = bufs[b];
		for (int row = 0; row < 6; row++)
		{
			int y = 149 + row;
			for (int px = 0; px < 5; px++)
			{
				fb[y * FB_WIDTH + 199 + px] = colour;
			}
		}
	}
}

// sub_0410F0 (E1:764-835): faction LED indicator at X 55-59
// Palette: 15=bg, 5=red, 14=green, 6=blue

void HudFactionLED()
{
	uint8_t colour;

	// E1:765-766: TST.B ($0631CE); BPL loc_04111E
	int8_t modeFlag =
		static_cast<int8_t>(g_workspace.objs.flagsTable[OBJ_METAL_DETECTOR]);
	if (modeFlag >= 0)
	{
		// State 1 (loc_04111E): no metal detector -- reset to background
		colour = 15;
	}
	else
	{
		// E1:767-768: TST.B ($062421); BEQ loc_041144
		// $062421 = low byte of currentTileIndex (big-endian)
		uint8_t tileIndexLo = static_cast<uint8_t>(
			g_workspace.tileDetail.currentTileIndex & 0xFF);
		if (tileIndexLo == 0)
		{
			colour = 5; // State 2: off-grid
		}
		// E1:769-770: TST.W ($0623AA); BNE loc_041144
		else if ((g_workspace.cam.posY >> 16) != 0)
		{
			colour = 5; // State 2: airborne
		}
		// E1:771-772: BTST #5,($062492); BNE loc_041144
		else if (g_workspace.tileDetail.tileProperty & 0x20)
		{
			colour = 5; // State 2: bit 5 set
		}
		// E1:773-774: BTST #6,($062492); BNE loc_041190
		else if (g_workspace.tileDetail.tileProperty & 0x40)
		{
			colour = 6; // State 4: faction bit 6 set
		}
		else
		{
			colour = 14; // State 3: faction bit 6 clear
		}
	}

	// sub_0411B6 (E1:818-835): draw into both buffers
	// Framebuffer offset $5D38 = scanline 149, byte 24 = word group 3
	// Pixel X = 48.  Bits 4-8 = pixels 7-11 = absolute X 55-59
	uint8_t *bufs[2] = {g_workspace.frameBuf[0], g_workspace.frameBuf[1]};
	for (int b = 0; b < 2; b++)
	{
		uint8_t *fb = bufs[b];
		for (int row = 0; row < 6; row++)
		{
			int y = 149 + row;
			for (int px = 0; px < 5; px++)
			{
				fb[y * FB_WIDTH + 55 + px] = colour;
			}
		}
	}
}

// sub_041036 (E1:704-733): low altitude warning (SHOW)
// Red indicator at X 127-131 when in vehicle and altitude <= $4000

static bool s_altWarningDrawn = false;

void HudAltitudeWarning()
{
	// E1:705-706: TST.L ($0623FE); BMI sub_0410A0
	// Tests the longword at $0623FE.  Bit 31 = bit 15 of flightState
	// flightState $8000 (on foot) is negative -> clear warning
	if (static_cast<int16_t>(g_workspace.cam.flightState) < 0)
	{
		HudAltitudeWarningClear();
		return;
	}

	// E1:707-709: MOVE.L ($0623AA),D0; SUBI.L #$4000,D0; BGT sub_0410A0
	// If altitude > $4000, clear warning
	if (static_cast<int32_t>(g_workspace.cam.posY) >
		static_cast<int32_t>(0x4000))
	{
		HudAltitudeWarningClear();
		return;
	}

	// E1:710-712: already drawn guard
	if (s_altWarningDrawn)
		return;
	s_altWarningDrawn = true;

	// E1:713-730: draw red indicator -- palette 5 at pixels 127-131
	uint8_t *bufs[2] = {g_workspace.frameBuf[0], g_workspace.frameBuf[1]};
	for (int b = 0; b < 2; b++)
	{
		uint8_t *fb = bufs[b];
		for (int row = 0; row < 6; row++)
		{
			int y = 149 + row;
			for (int px = 0; px < 5; px++)
			{
				fb[y * FB_WIDTH + 127 + px] = 5;
			}
		}
	}
}

// sub_0410A0 (E1:739-761): low altitude warning (CLEAR)
// Restores the indicator pixels to the panel background (palette 15)

void HudAltitudeWarningClear()
{
	// E1:740-742: if not drawn, skip
	if (!s_altWarningDrawn)
		return;
	s_altWarningDrawn = false;

	// E1:743-760: restore panel background -- palette 15 at pixels 127-131
	uint8_t *bufs[2] = {g_workspace.frameBuf[0], g_workspace.frameBuf[1]};
	for (int b = 0; b < 2; b++)
	{
		uint8_t *fb = bufs[b];
		for (int row = 0; row < 6; row++)
		{
			int y = 149 + row;
			for (int px = 0; px < 5; px++)
			{
				fb[y * FB_WIDTH + 127 + px] = 15;
			}
		}
	}
}
