# CMake configuration for CS237 utility library
#
# CMSC 23740 -- Introduction to Real-Time Graphics
# Autumn 2024
# University of Chicago
#
# COPYRIGHT (c) 2024 John Reppy
# All rights reserved.
#

set(SRCS
  aabb.cpp
  application.cpp
  attachment.cpp
  cone.cpp
  cube.cpp
  depth-buffer.cpp
  image.cpp
  json.cpp
  json-parser.cpp
  memory-obj.cpp
  mtl-reader.cpp
  obj-reader.cpp
  obj.cpp
  shader.cpp
  sphere.cpp
  texture.cpp
  window.cpp)

add_library(cs237
  STATIC
  ${SRCS})
