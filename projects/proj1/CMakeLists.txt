# CMake configuration for individual project 1
#
# CMSC 23740 -- Introduction to Real-Time Graphics
# Autumn 2024
# University of Chicago
#
# COPYRIGHT (c) 2024 John Reppy
# All rights reserved.
#

project(CMSC237_PROJ_1
  VERSION 1
  HOMEPAGE_URL "https://classes.cs.uchicago.edu/archive/2024/fall/23740-1/index.html"
  LANGUAGES C CXX)

# shader source code
#
add_subdirectory(shaders)

# the source code for the project
#
add_subdirectory(src)
