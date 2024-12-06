# CMake configuration for individual Project 3 sources
#
# CMSC 23740 -- Introduction to Real-Time Graphics
# Autumn 2024
# University of Chicago
#
# COPYRIGHT (c) 2024 John Reppy
# All rights reserved.
#

set(SRCS
  app.cpp
  ground.cpp
  height-field.cpp
  main.cpp
  mesh.cpp
  scene.cpp
  window.cpp)

add_executable(${TARGET} ${SRCS})

target_compile_definitions(${TARGET} PRIVATE PROJ5_SOURCE_ROOT="${PROJ5_SOURCE_ROOT}")
target_compile_definitions(${TARGET} PRIVATE PROJ5_BINARY_ROOT="${PROJ5_BINARY_ROOT}")

target_link_libraries(${TARGET} cs237)
add_dependencies(${TARGET} ${TARGET}-shaders)
