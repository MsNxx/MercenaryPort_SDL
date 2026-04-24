#pragma once

// Keyboard input dispatch, elevator handler, bail-out, interaction ping

struct Game;

// Keyboard command dispatch -- reads keyCommand and dispatches
void KeyCommandDispatch(Game &game);

// Interaction ping sound -- channel B tone with envelope decay
void InteractionPing(Game &game);
