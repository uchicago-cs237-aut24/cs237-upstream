/*! \file cs237.hpp
 *
 * Support code for CMSC 23740 Autumn 2024.
 *
 * This is the main header file for the CS237 Library.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_HPP_
#define _CS237_HPP_

#include "cs237/config.h"

#include <cmath>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

/* GLM include files; we include the extensions, such as transforms,
 * and enable the experimental support for `to_string`.
 */
#define GLM_FORCE_CXX17 // we are using C++ 2017
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES // make vec3 consistent with GLSL shaders
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Vulkan's clip-space Z-axis is 0..1
#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext.hpp"

/* The GLFW and Vulkan library */
#define VULKAN_HPP_TYPESAFE_CONVERSION 1
#include <vulkan/vulkan.hpp>

/* The GLFW library */
//#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace cs237 {

//! function for reporting errors by raising a runtime exception
//! that includes the file and line number of the error.
[[ noreturn ]]
inline void ReportError (const char *file, int line, std::string const &msg)
{
    std::string s = "[" + std::string(file) + ":" + std::to_string(line) + "] " + msg;
    throw std::runtime_error(s);
}

} // namespace cs237

#define ERROR(msg)      cs237::ReportError (__FILE__, __LINE__, msg);

/* CS23740 support files */
#include "cs237/types.hpp"

#include "cs237/shader.hpp"
#include "cs237/pipeline.hpp"
#include "cs237/application.hpp"
#include "cs237/window.hpp"
#include "cs237/memory-obj.hpp"
#include "cs237/buffer.hpp"
#include "cs237/image.hpp"
#include "cs237/texture.hpp"
#include "cs237/depth-buffer.hpp"

/* geometric types */
#include "cs237/aabb.hpp"
#include "cs237/plane.hpp"

/***** a wrapper for printing GLM vectors *****/
template<glm::length_t L, typename T, glm::qualifier Q>
std::ostream& operator<< (std::ostream& s, glm::vec<L,T,Q> const &v)
{
    return (s << glm::to_string(v));
}

#endif // !_CS237_HPP_
