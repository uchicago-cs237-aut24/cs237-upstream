# CMake configuration for CS237 utility library documentation
#
# CMSC 23740 -- Introduction to Realtime Graphics
# Autumn 2024
# University of Chicago
#
# COPYRIGHT (c) 2024 John Reppy
# All rights reserved.
#

if (DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxygen.cfg.in
    ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg @ONLY)

  add_custom_target(html
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating doxygen documentation." VERBATIM)
endif()