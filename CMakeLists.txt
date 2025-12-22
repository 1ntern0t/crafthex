cmake_minimum_required(VERSION 3.10)

project(crafthex LANGUAGES C)

# ---- Build settings ----
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---- Sources ----
file(GLOB SOURCE_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.c")

option(CRAFTHEX_USE_VENDORED_DEPS "Use deps/ vendored libraries when available" ON)
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/deps")
  set(CRAFTHEX_USE_VENDORED_DEPS OFF)
endif()

option(CRAFTHEX_ENABLE_SANITIZERS "Enable ASan/UBSan in Debug builds (GCC/Clang)" OFF)

add_executable(crafthex ${SOURCE_FILES})

if(CRAFTHEX_USE_VENDORED_DEPS)
  target_sources(crafthex PRIVATE
    deps/glew/src/glew.c
    deps/lodepng/lodepng.c
    deps/noise/noise.c
    deps/sqlite/sqlite3.c
    deps/tinycthread/tinycthread.c
  )
endif()

if(CRAFTHEX_USE_VENDORED_DEPS)
  target_include_directories(crafthex PRIVATE
    "${CMAKE_SOURCE_DIR}/deps/glew/include"
    "${CMAKE_SOURCE_DIR}/deps/glfw/include"
    "${CMAKE_SOURCE_DIR}/deps/lodepng"
    "${CMAKE_SOURCE_DIR}/deps/noise"
    "${CMAKE_SOURCE_DIR}/deps/sqlite"
    "${CMAKE_SOURCE_DIR}/deps/tinycthread"
  )
endif()

# Compile flags
target_compile_options(crafthex PRIVATE
  $<$<C_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wno-unused-parameter>
  $<$<AND:$<CONFIG:Release>,$<C_COMPILER_ID:GNU,Clang>>:-O3>
  $<$<AND:$<CONFIG:Debug>,$<C_COMPILER_ID:GNU,Clang>>:-O0 -g>
)

if(CRAFTHEX_ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(crafthex PRIVATE -fsanitize=address,undefined)
    target_link_options(crafthex PRIVATE -fsanitize=address,undefined)
  endif()
endif()

# Fix strdup + friends on strict C standards
# (applies to craft + glfw because glfw is built as a subdir target)
add_compile_definitions(_POSIX_C_SOURCE=200809L)

# ---- Dependencies ----
# Don’t build GLFW docs/tests/examples (avoids doxygen + extra junk)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

if(CRAFTHEX_USE_VENDORED_DEPS)
  add_subdirectory(deps/glfw)
endif()

find_package(CURL REQUIRED)
find_package(OpenGL REQUIRED)

if(UNIX AND NOT APPLE)
  find_package(X11 REQUIRED)
endif()

# Link CURL (prefer imported target)
if(TARGET CURL::libcurl)
  target_link_libraries(crafthex PRIVATE CURL::libcurl)
else()
  target_include_directories(crafthex PRIVATE ${CURL_INCLUDE_DIRS})
  target_link_libraries(crafthex PRIVATE ${CURL_LIBRARIES})
endif()

# Link GLFW + the stuff vendored-old-GLFW forgets to propagate
if(CRAFTHEX_USE_VENDORED_DEPS)
  target_link_libraries(crafthex PRIVATE glfw)
else()
  find_package(glfw3 REQUIRED)
  if(TARGET glfw)
    target_link_libraries(crafthex PRIVATE glfw)
  elseif(TARGET glfw::glfw)
    target_link_libraries(crafthex PRIVATE glfw::glfw)
  elseif(TARGET glfw3)
    target_link_libraries(crafthex PRIVATE glfw3)
  else()
    message(FATAL_ERROR "glfw3 was found but no known CMake target was exported")
  endif()
endif()

target_link_libraries(crafthex PRIVATE OpenGL::GL)

if(UNIX)
  target_link_libraries(crafthex PRIVATE m dl)
endif()

# X11 family (these are the missing XRR/Xi/Xinerama/Xcursor symbols)
if(UNIX AND NOT APPLE)
  target_link_libraries(crafthex PRIVATE
    X11
    Xrandr
    Xi
    Xinerama
    Xcursor
    Xxf86vm
  )
endif()

