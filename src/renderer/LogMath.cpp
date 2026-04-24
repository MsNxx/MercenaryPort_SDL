// Log-domain arithmetic -- tables from M, instruction sequences from E1

#include "renderer/LogMath.h"
#include "data/Gen_MData.h"

#include <cmath>

// Table access

uint16_t LogLookup(uint16_t mantissa)
{
	// Signed displacement from A5 base into LOG_TABLE_FULL
	int16_t signedByteOffset = static_cast<int16_t>(mantissa & 0xFFFE);
	int idx = gen_m::LOG_A5_OFFSET + (signedByteOffset / 2);
	if (idx < 0 || idx >= gen_m::LOG_FULL_WORDS)
	{
		return 0;
	}
	return gen_m::LOG_TABLE_FULL[idx];
}

uint16_t AntiLogLookup(uint16_t logVal)
{
	// ROR.W #4, D
	uint16_t rotated = static_cast<uint16_t>((logVal >> 4) | (logVal << 12));
	// ANDI.W #$1FFE, D
	int idx = (rotated & 0x1FFE) / 2;
	if (idx < 0 || idx >= gen_m::ANTILOG_WORDS)
	{
		return 0;
	}
	return gen_m::ANTILOG_TABLE[idx];
}

// Multiplication: log(A) + log(B) -> antilog

uint32_t LogMultiply(uint32_t a, uint32_t b)
{
	uint16_t logA = LogLookup(static_cast<uint16_t>(a));
	uint16_t logB = LogLookup(static_cast<uint16_t>(b));
	uint32_t result = ((a & 0xFFFF0000u) | logA) + ((b & 0xFFFF0000u) | logB);
	uint16_t mantissa = AntiLogLookup(static_cast<uint16_t>(result));
	return (result & 0xFFFF0000u) | mantissa;
}

// Division
// Same pattern but SUB.L instead of ADD.L

uint32_t LogDivide(uint32_t a, uint32_t b)
{
	uint16_t logA = LogLookup(static_cast<uint16_t>(a));
	uint16_t logB = LogLookup(static_cast<uint16_t>(b));
	uint32_t result = ((a & 0xFFFF0000u) | logA) - ((b & 0xFFFF0000u) | logB);
	uint16_t mantissa = AntiLogLookup(static_cast<uint16_t>(result));
	return (result & 0xFFFF0000u) | mantissa;
}

// Addition with normalization ($048C60-$048CC4)

uint32_t LogFloatAdd(uint32_t a, uint32_t b)
{
	// Sort so a >= b (signed comparison, matching CMP.L + BLT + EXG)
	if (static_cast<int32_t>(b) > static_cast<int32_t>(a))
	{
		uint32_t tmp = a;
		a = b;
		b = tmp;
	}

	uint16_t expSmall = static_cast<uint16_t>(b >> 16);
	uint16_t expLarge = static_cast<uint16_t>(a >> 16);
	int16_t expDiff = static_cast<int16_t>(expLarge - expSmall);

	if (expDiff > 11)
	{
		return a & 0xFFFFFFFEu;
	}

	int16_t mantSmall = static_cast<int16_t>(b & 0xFFFF);
	mantSmall >>= expDiff; // ASR.W D7,D2
	int16_t mantResult = static_cast<int16_t>(a & 0xFFFF) + mantSmall;

	uint32_t result = (a & 0xFFFF0000u) | static_cast<uint16_t>(mantResult);

	// Determine original sign relationship for normalization direction
	bool sameSign = ((b ^ a) & 0x8000) == 0;

	if (mantResult < 0)
	{
		if (sameSign)
		{
			// Same-sign overflow check
			if (mantResult & 0x2000)
			{
				return result & 0xFFFFFFFEu;
			}
			mantResult >>= 1;
			return (((result & 0xFFFF0000u) + 0x00010000u) |
					static_cast<uint16_t>(mantResult)) &
				   0xFFFFFFFEu;
		}
		while (mantResult & 0x1000)
		{
			mantResult <<= 1;
			result -= 0x00010000u;
		}
		return ((result & 0xFFFF0000u) | static_cast<uint16_t>(mantResult)) &
			   0xFFFFFFFEu;
	}

	if (mantResult == 0)
	{
		return (result + LOG_FLOAT_ZERO) & 0xFFFFFFFEu;
	}

	if (sameSign)
	{
		if (!(mantResult & 0x2000))
		{
			return result & 0xFFFFFFFEu;
		}
		mantResult >>= 1;
		return (((result & 0xFFFF0000u) + 0x00010000u) |
				static_cast<uint16_t>(mantResult)) &
			   0xFFFFFFFEu;
	}

	while (!(mantResult & 0x1000))
	{
		mantResult <<= 1;
		result -= 0x00010000u;
	}
	return ((result & 0xFFFF0000u) | static_cast<uint16_t>(mantResult)) &
		   0xFFFFFFFEu;
}

// Integer to log-float
// $042830-$042864 (star respawn normalization)

uint32_t IntToLogFloat(int32_t val)
{
	if (val == 0)
	{
		return LOG_FLOAT_ZERO;
	}

	bool negative = (val < 0);
	uint32_t uval =
		negative ? static_cast<uint32_t>(-val) : static_cast<uint32_t>(val);

	// Find MSB position via the same byte-scanning algorithm the
	// 68000 uses (SWAP, TST.B, ROL.L #8 loop, then ROL.L #1 loop)
	int exponent = 23;
	uint32_t work = (uval >> 16) | (uval << 16); // SWAP
	uint8_t testByte = static_cast<uint8_t>(work & 0xFF);

	while (testByte == 0)
	{
		exponent -= 8;
		work = (work << 8) | (work >> 24); // ROL.L #8
		testByte = static_cast<uint8_t>(work & 0xFF);
	}

	while (!(testByte & 0x80))
	{
		work = (work << 1) | (work >> 31); // ROL.L #1
		testByte = static_cast<uint8_t>(work & 0xFF);
		exponent--;
	}

	// Position mantissa: ROL.L #4, ADD.W D0,D0, SWAP, set exponent
	work = (work << 4) | (work >> 28); // ROL.L #4
	uint16_t mantissa = static_cast<uint16_t>(work);
	mantissa <<= 1; // ADD.W D0,D0

	uint32_t result = (static_cast<uint32_t>(exponent) << 16) | mantissa;

	if (negative)
	{
		mantissa = static_cast<uint16_t>(-static_cast<int16_t>(mantissa));
		result = (result & 0xFFFF0000u) | mantissa;
	}

	return result;
}

// Road-specific integer to log-float ($0444CC-$044514)
// Exponent 31 (not 23), no SWAP, ANDI #$1FFF before sign correction

uint32_t IntToLogFloat31(int32_t val)
{
	if (val == 0)
	{
		return LOG_FLOAT_ZERO;
	}

	bool negative = (val < 0);
	uint32_t uval =
		negative ? static_cast<uint32_t>(-val) : static_cast<uint32_t>(val);

	// $0444E4: MOVEQ #$1F,D7
	int exponent = 31;

	// $0444E6: ROL.L #8,D4 (NO SWAP -- direct ROL)
	uint32_t work = (uval << 8) | (uval >> 24);
	uint8_t testByte = static_cast<uint8_t>(work & 0xFF);

	// $0444E8-$0444F2: byte scan
	while (testByte == 0)
	{
		exponent -= 8;
		work = (work << 8) | (work >> 24);
		testByte = static_cast<uint8_t>(work & 0xFF);
	}

	// $0444F4-$0444FE: bit scan
	while (!(testByte & 0x80))
	{
		work = (work << 1) | (work >> 31);
		testByte = static_cast<uint8_t>(work & 0xFF);
		exponent--;
	}

	// $044500-$04450A: ROL.L #4, ADD.W, SWAP, MOVE.W, SWAP, ANDI
	work = (work << 4) | (work >> 28);
	uint16_t mantissa = static_cast<uint16_t>(work);
	mantissa <<= 1;

	uint32_t result = (static_cast<uint32_t>(exponent) << 16) | mantissa;

	// $04450A: ANDI.W #$1FFF,D4
	mantissa &= 0x1FFF;

	// $04450E-$044512: sign correction
	if (negative)
	{
		mantissa = static_cast<uint16_t>(-static_cast<int16_t>(mantissa));
	}

	result = (static_cast<uint32_t>(exponent) << 16) | mantissa;
	return result;
}

// Log-float to screen coordinate ($04277C-$042792)
// Returns raw linear value before center offset; 0x7FFFFFFF on overflow

int32_t LogFloatToScreen(uint32_t val)
{
	// $04277C-$042792:
	//   MOVE.L D0,D7    ; copy full 32-bit value
	//   BPL loc_042784  ; test bit 31 of the FULL 32-bit value
	//   CLR.W D0        ; if negative (bit 31 set): zero mantissa
	//   BRA loc_042794  ; skip normalisation

	int32_t fullSigned = static_cast<int32_t>(val);
	int16_t mantissa = static_cast<int16_t>(val & 0xFFFF);

	if (fullSigned < 0)
	{
		// Full 32-bit value is negative -> CLR.W D0 -> result = 0
		return 0;
	}

	// SWAP D7  ->  exponent to low word
	int16_t exponent = static_cast<int16_t>(val >> 16);
	// NEG.W D7
	// ADDI.W #$000E, D7  ->  shift = 14 - exponent
	int shift = 14 - exponent;
	if (shift < 0)
	{
		return 0x7FFFFFFF; // overflow (BMI -> respawn)
	}

	// EXT.L D0  ->  sign-extend mantissa to 32 bits
	int32_t extended = static_cast<int32_t>(mantissa);
	// ASL.L #2, D0  ->  multiply by 4
	extended <<= 2;
	// ASR.L D7, D0  ->  arithmetic right shift
	extended >>= shift;

	return extended;
}

// Log-float to integer (velocity variant)
// Same algorithm as LogFloatToScreen but clamps overflow to 16-bit range
// Used by movement physics ($04852A) and projectile velocity (sub_0416D6)

int32_t LogFloatToInt(uint32_t logVal)
{
	int16_t mantissa = static_cast<int16_t>(logVal & 0xFFFF);

	if (static_cast<int32_t>(logVal) < 0)
		return 0;

	int16_t exponent = static_cast<int16_t>(logVal >> 16);
	int shift = 14 - exponent;
	if (shift < 0)
		return (mantissa >= 0) ? 0x7FFF : ~0x7FFF;

	int32_t val = static_cast<int32_t>(mantissa);
	val <<= 2;
	val >>= shift;
	return val;
}

// Debug: log-float to C++ float

float LogFloatToLinear(uint32_t val)
{
	int16_t mantissa = static_cast<int16_t>(val & 0xFFFF);
	int16_t exponent = static_cast<int16_t>(val >> 16);

	if (mantissa == 0)
	{
		return 0.0f;
	}

	return static_cast<float>(mantissa) *
		   exp2f(static_cast<float>(exponent - 12));
}
