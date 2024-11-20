# CMake configuration for lab assignment 6 sources
#
# CMSC 23740 -- Introduction to Real-Time Graphics
# Autumn 2024
# University of Chicago
#
# COPYRIGHT (c) 2024 John Reppy
# All rights reserved.
#

set(TARGET lab6)

set(SRCS
  crate.cpp
  drawable.cpp
  floor.cpp
  gbuffer.cpp
  main.cpp)

add_executable(${TARGET} ${SRCS})

target_link_libraries(${TARGET} cs237)
add_dependencies(${TARGET} lab6-shaders)
