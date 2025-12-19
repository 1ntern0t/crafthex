cmake_minimum_required(VERSION 3.10)

project(crafthex LANGUAGES C)

# ---- Build settings ----
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Optional: default to Release if they didn't pick a config
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

# Keep warnings sane, keep optimization per-config (not forced globally)
target_compile_options(crafthex PRIVATE
  $<$<C_COMPILER_ID:GNU,Clang>:-O3 -Wall -Wextra -Wno-unused-parameter>
)

# ---- Dependencies ----
add_subdirectory(deps/glfw)

find_package(CURL REQUIRED)

# Modern CMake: prefer imported target if available
if(TARGET CURL::libcurl)
  target_link_libraries(crafthex PRIVATE CURL::libcurl)
else()
  target_include_directories(crafthex PRIVATE ${CURL_INCLUDE_DIRS})
  target_link_libraries(crafthex PRIVATE ${CURL_LIBRARIES})
endif()

# ---- Platform linking ----
if(APPLE)
  target_link_libraries(crafthex PRIVATE glfw)
elseif(UNIX)
  target_link_libraries(crafthex PRIVATE glfw dl)
elseif(MINGW)
  target_link_libraries(crafthex PRIVATE glfw ws2_32)
endif()

