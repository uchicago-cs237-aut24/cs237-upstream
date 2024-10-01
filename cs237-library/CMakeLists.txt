# CMake configuration for CS237 utility library
#
# CMSC 23740 -- Introduction to Realtime Graphics
# Autumn 2024
# University of Chicago
#
# COPYRIGHT (c) 2024 John Reppy
# All rights reserved.
#

# Generate a CompilationDatabase (compile_commands.json file) for our build,
# for use by clang_complete, YouCompleteMe, etc.
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_subdirectory(src)

if (CS237_ENABLE_DOXYGEN)
  message(STATUS "Doxygen enabled.")
  find_package(Doxygen REQUIRED)
  if (DOXYGEN_FOUND)
    add_subdirectory(doc)
  endif()
endif()
