# CMake configuration for lab assignment 3 sources
#
# CMSC 23740 -- Introduction to Computer Graphics
# Autumn 2024
# University of Chicago
#
# COPYRIGHT (c) 2023 John Reppy
# All rights reserved.
#

set(TARGET lab3)

set(SRCS
  main.cpp)

add_executable(${TARGET} ${SRCS})

target_link_libraries(${TARGET} cs237)
add_dependencies(${TARGET} lab3-shaders)
