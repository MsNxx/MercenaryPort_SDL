#pragma once

// Growing planet disc for intro Phase 4

#include <cstdint>

struct PlanetDisc
{
	uint32_t scaleFactor; // D3: shrinks each frame via log-multiply
	uint32_t refValue;	  // D1: reference ($000D1000), evolves
	int lastHalfWidth;	  // equatorial half-width from previous tick
	bool complete;		  // true when disc fills the viewport
};

void PlanetDiscInit(PlanetDisc &disc);

// Returns true when the disc fills the viewport
bool PlanetDiscTick(PlanetDisc &disc, uint8_t *indexBuf);
