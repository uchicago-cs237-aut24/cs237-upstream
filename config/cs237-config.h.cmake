/*! \file cs237-config.h
 *
 * Generated configuration file
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_CONFIG_H_
#define _CS237_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif // C++

//! the path to the root of the build directory
#cmakedefine CS237_BINARY_DIR "@CS237_BINARY_DIR@"

//! the path to the root of the source directory
#cmakedefine CS237_SOURCE_DIR "@CS237_SOURCE_DIR@"

//! if <strings.h> is available and needed for strncasecmp
#cmakedefine INCLUDE_STRINGS_H <strings.h>

//! is strncasecmp available?
#cmakedefine HAVE_STRNCASECMP

//! flag for windows build
#cmakedefine CS237_WINDOWS

#ifdef __cplusplus
}
#endif // C++

#endif // !_CS237_CONFIG_H_
