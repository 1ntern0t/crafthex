#ifndef CRAFTHEX_ENGINE_H
#define CRAFTHEX_ENGINE_H

// Entry point for the game/engine.
// Kept as a function so the platform layer (desktop, Android, etc.) can
// provide a different bootstrap without touching engine internals.
int engine_run(int argc, char **argv);

#endif
