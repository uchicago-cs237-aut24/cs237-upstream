# CMake configuration for individual programming assignments
#
# CMSC 23740 -- Introduction to Real-Time Graphics
# Autumn 2024
# University of Chicago
#
# COPYRIGHT (c) 2024 John Reppy
# All rights reserved.
#

cmake_minimum_required(VERSION 3.21.0)

project(CMSC237_AUT_2024
  VERSION 1
  HOMEPAGE_URL "https://classes.cs.uchicago.edu/archive/2024/fall/23740-1/index.html"
  DESCRIPTION "individual programming projects for CMSC 23740 (Autumn 2024)"
  LANGUAGES C CXX)

# Disable builds in the source tree to prevent corruption.
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "
ERROR: builds in the source tree are not allowed.
       Create a separate directory for build files.
")
endif()

if (WIN32)
  set(CS237_WINDOWS ON)
endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type selected, default to Debug")
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (default Debug)" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard to conform to")
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# Add path for custom modules
set(CMAKE_MODULE_PATH
  "${CMAKE_MODULE_PATH}"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  )

# find libraries that we need

find_package(GLFW 3.4 REQUIRED)

find_package(Vulkan 1.3 REQUIRED)
if (NOT ${CMAKE_HOST_APPLE})
  find_package(X11 REQUIRED)
endif()

# compiler for shader programs
#
if (Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
  set(GLSLC ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE})
elseif (VULKAN_BIN_DIR)
  set(GLSLC "${VULKAN_BIN_DIR}/glslangValidator")
else()
  message(FATAL_ERROR "unable to determine where glslangValidator lives")
endif()
#
if (Vulkan_LIBRARY)
  set (VULKAN_LIBRARY ${Vulkan_LIBRARY})
endif()

find_package(PNG 1.5 REQUIRED)

# options
#
option (CS237_ENABLE_DOXYGEN "Enable doxygen for generating cs237 library documentation." OFF)
option (CS237_VERBOSE_MAKEFILE "Enable verbose makefiles." OFF)
option (CS237_BUILD_LABS "Build the lab assignments" ON)
option (CS237_BUILD_PROJS "Build the individual projects" OFF)

# enable verbose makefiles
#
set(CMAKE_VERBOSE_MAKEFILE ${CS237_VERBOSE_MAKEFILE})

# definitions for the configuration header file
#
include(cs237-config)

# Generate the configuration header file
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config/cs237-config.h.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cs237-library/include/cs237/config.h
  @ONLY)

# path to include files
set(CS237_INCLUDE_DIR
  ${CMAKE_SOURCE_DIR}/cs237-library/include
  ${CMAKE_CURRENT_BINARY_DIR}/cs237-library/include)
include_directories(
  ${GLFW_INCLUDE_DIR}
  ${Vulkan_INCLUDE_DIR}
  ${PNG_INCLUDE_DIR}
  ${CS237_INCLUDE_DIR})

# libraries
link_libraries(${GLFW_LIBRARY})
link_libraries(${VULKAN_LIBRARY})
link_libraries(${PNG_LIBRARY})

# on Linux, we need X11
if (${CMAKE_HOST_LINUX})
  include_directories(${X11_INCLUDE_DIR})
  link_libraries(${X11_LIBRARIES})
endif()

# cs237 library
#
add_subdirectory(cs237-library)
set(CS237_DOXYGEN_HTML_DIR ${CMAKE_SOURCE_DIR}/cs237-library/doc)

# the subdirectory for the labs
#
if (CS237_BUILD_LABS)
  add_subdirectory(labs EXCLUDE_FROM_ALL)
endif()

# the subdirectory for the individual projects
#
if (CS237_BUILD_PROJS)
  add_subdirectory(projects EXCLUDE_FROM_ALL)
endif()

# the subdirectory for the group projects
#
#add_subdirectory(project EXCLUDE_FROM_ALL)

