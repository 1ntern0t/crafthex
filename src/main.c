#include "engine.h"

// Thin platform bootstrap.
// Desktop (GLFW) currently lives inside engine_run().
// Later: swap this file per-platform (Android NDK entry, etc.).
int main(int argc, char **argv) {
  return engine_run(argc, argv);
}
