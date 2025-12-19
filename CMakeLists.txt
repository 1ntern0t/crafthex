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

add_executable(crafthex
  ${SOURCE_FILES}
  deps/glew/src/glew.c
  deps/lodepng/lodepng.c
  deps/noise/noise.c
  deps/sqlite/sqlite3.c
  deps/tinycthread/tinycthread.c
)

target_include_directories(crafthex PRIVATE
  "${CMAKE_SOURCE_DIR}/deps/glew/include"
  "${CMAKE_SOURCE_DIR}/deps/glfw/include"
  "${CMAKE_SOURCE_DIR}/deps/lodepng"
  "${CMAKE_SOURCE_DIR}/deps/noise"
  "${CMAKE_SOURCE_DIR}/deps/sqlite"
  "${CMAKE_SOURCE_DIR}/deps/tinycthread"
)

# Compile flags
target_compile_options(crafthex PRIVATE
  $<$<C_COMPILER_ID:GNU,Clang>:-O3 -Wall -Wextra -Wno-unused-parameter>
)

# Fix strdup + friends on strict C standards
# (applies to craft + glfw because glfw is built as a subdir target)
add_compile_definitions(_POSIX_C_SOURCE=200809L)

# ---- Dependencies ----
# Don’t build GLFW docs/tests/examples (avoids doxygen + extra junk)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(deps/glfw)

find_package(CURL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(X11 REQUIRED)

# Link CURL (prefer imported target)
if(TARGET CURL::libcurl)
  target_link_libraries(crafthex PRIVATE CURL::libcurl)
else()
  target_include_directories(crafthex PRIVATE ${CURL_INCLUDE_DIRS})
  target_link_libraries(crafthex PRIVATE ${CURL_LIBRARIES})
endif()

# Link GLFW + the stuff vendored-old-GLFW forgets to propagate
target_link_libraries(crafthex PRIVATE
  glfw
  OpenGL::GL
  m
  dl
)

# X11 family (these are the missing XRR/Xi/Xinerama/Xcursor symbols)
target_link_libraries(crafthex PRIVATE
  X11
  Xrandr
  Xi
  Xinerama
  Xcursor
  Xxf86vm
)

