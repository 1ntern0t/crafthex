#include "platform.h"

#include <string.h>

#if defined(_WIN32)
  #include <windows.h>
  #include <io.h>
  #define access _access
  #define R_OK 4
#elif defined(__APPLE__)
  #include <unistd.h>
  #include <limits.h>
  #include <mach-o/dyld.h>
#else
  #include <unistd.h>
  #include <limits.h>
#endif

int platform_file_readable(const char *path) {
  return path && access(path, R_OK) == 0;
}

int platform_get_exe_dir(char *out, size_t out_size) {
  if (!out || out_size == 0) {
    return 0;
  }

#if defined(_WIN32)
  char exe_path[MAX_PATH];
  DWORD n = GetModuleFileNameA(NULL, exe_path, (DWORD)sizeof(exe_path));
  if (n == 0 || n >= sizeof(exe_path)) {
    out[0] = '\0';
    return 0;
  }
  exe_path[n] = '\0';
  char *slash = strrchr(exe_path, '\\');
  if (!slash) {
    slash = strrchr(exe_path, '/');
  }
  if (!slash) {
    out[0] = '\0';
    return 0;
  }
  *slash = '\0';
  strncpy(out, exe_path, out_size - 1);
  out[out_size - 1] = '\0';
  return 1;

#elif defined(__APPLE__)
  char exe_path[PATH_MAX];
  uint32_t size = (uint32_t)sizeof(exe_path);
  if (_NSGetExecutablePath(exe_path, &size) != 0) {
    out[0] = '\0';
    return 0;
  }
  exe_path[sizeof(exe_path) - 1] = '\0';
  char *slash = strrchr(exe_path, '/');
  if (!slash) {
    out[0] = '\0';
    return 0;
  }
  *slash = '\0';
  strncpy(out, exe_path, out_size - 1);
  out[out_size - 1] = '\0';
  return 1;

#else
  char exe_path[PATH_MAX];
  ssize_t n = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
  if (n <= 0) {
    out[0] = '\0';
    return 0;
  }
  exe_path[n] = '\0';
  char *slash = strrchr(exe_path, '/');
  if (!slash) {
    out[0] = '\0';
    return 0;
  }
  *slash = '\0';
  strncpy(out, exe_path, out_size - 1);
  out[out_size - 1] = '\0';
  return 1;
#endif
}
