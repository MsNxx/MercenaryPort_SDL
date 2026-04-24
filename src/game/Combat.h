#pragma once

// Outdoor combat -- projectiles, destruction, object motion, missile tracking

struct Game;

// Runs the outdoor combat sequence each main-loop tick
void CombatOutdoorTick(Game &game);
