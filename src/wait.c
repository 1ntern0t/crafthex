#include "wait.h"

#include <time.h>

double wait_time_s(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

void wait_sleep_ms(int ms) {
  if (ms <= 0) {
    return;
  }
  struct timespec req;
  req.tv_sec = ms / 1000;
  req.tv_nsec = (long)(ms % 1000) * 1000000L;
  nanosleep(&req, 0);
}
