# CMake configuration for Lab 6 shaders
#
# CMSC 23740 -- Introduction to Real-Time Graphics
# Autumn 2024
# University of Chicago
#
# COPYRIGHT (c) 2024 John Reppy
# All rights reserved.
#

set(SRCS
  geom-pass.frag
  geom-pass.vert
  final-pass.frag
  final-pass.vert)

# custom commands for compiling shaders
#
foreach(SHADER_SRC ${SRCS})
  get_filename_component(FILE_NAME ${SHADER_SRC} NAME)
  set(SHADER_FILE "${PROJECT_SOURCE_DIR}/shaders/${SHADER_SRC}")
  set(SPIRV_FILE "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
    COMMAND ${GLSLC} -V -o ${SPIRV_FILE} ${SHADER_FILE}
    DEPENDS ${SHADER_FILE})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV_FILE})
endforeach(SHADER_SRC)

add_custom_target(
  lab6-shaders
  DEPENDS ${SPIRV_BINARY_FILES})
