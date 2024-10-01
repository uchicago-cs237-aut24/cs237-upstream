# Feature checks used to generate the "cs237/config.h" file
#
# COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
# All rights reserved.
#

include(CheckIncludeFile)
include(CheckSymbolExists)

set(CS237_BINARY_DIR ${CMAKE_BINARY_DIR})
set(CS237_SOURCE_DIR ${CMAKE_SOURCE_DIR})

# on Linux systems, we need <strings.h> for strcasencmp
#
check_include_file(strings.h INCLUDE_STRINGS_H)
if (INCLUDE_STRINGS_H)
  check_symbol_exists (strncasecmp "strings.h" HAVE_STRNCASECMP)
else()
#  check_symbol_exists (strncasecmp "" HAVE_STRNCASECMP)
endif()
