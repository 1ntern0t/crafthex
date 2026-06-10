#ifndef CRAFTHEX_PLATFORM_H
#define CRAFTHEX_PLATFORM_H

#include <stddef.h>

// Cross-platform helpers used by util.c and future portability work.

// Returns 1 on success, 0 on failure.
int platform_get_exe_dir(char *out, size_t out_size);

// Returns 1 if path exists and is readable, 0 otherwise.
int platform_file_readable(const char *path);

#endif
