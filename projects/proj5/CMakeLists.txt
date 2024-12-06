# CMake configuration for individual Project 5
#
# CMSC 23740 -- Introduction to Real-Time Graphics
# Autumn 2024
# University of Chicago
#
# COPYRIGHT (c) 2024 John Reppy
# All rights reserved.
#

project(CMSC237_PROJ_5
  VERSION 1
  HOMEPAGE_URL "https://classes.cs.uchicago.edu/archive/2024/fall/23740-1/index.html"
  LANGUAGES C CXX)

set(TARGET proj5)

set(PROJ5_SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJ5_BINARY_ROOT ${CMAKE_CURRENT_BINARY_DIR})

# shader source code
#
add_subdirectory(shaders)

# the source code for the project
#
add_subdirectory(src)
