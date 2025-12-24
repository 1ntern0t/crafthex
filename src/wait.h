#ifndef CRAFTHEX_WAIT_H
#define CRAFTHEX_WAIT_H

// Small time/sleep abstraction.
// Goal: centralize timing primitives for future Android/Windows ports.

double wait_time_s(void);
void wait_sleep_ms(int ms);

#endif
