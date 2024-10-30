/*! \file uniforms.hpp
 *
 * CMSC 23740 Autumn 2024 Lab 5.
 *
 * The layout of uniform data for the shaders.  We use the same layout
 * for all of the shaders.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _UNIFORMS_HPP_
#define _UNIFORMS_HPP_

#include "cs237/cs237.hpp"

// we use push-constants for the per-mesh uniform data
struct PC {
    alignas(16) glm::mat4 modelMat;     ///< model-space to world-space transform
    alignas(16) glm::vec3 color;        ///< flat color to use when texturing is disabled
};

// we use a UBO for the per-frame uniform data
struct UB {
    alignas(16) glm::mat4 viewMat;              ///< view matrix
    alignas(16) glm::mat4 projMat;              ///< projection matrix
    alignas(16) glm::mat4 shadowMat;            ///< model-space to light-space transform
    alignas(16) glm::vec3 lightDir;             ///< light direction in eye coordinates
    alignas(4) vk::Bool32 enableTexture;        ///< non-zero when texturing is enabled
    alignas(4) vk::Bool32 enableShadows;        ///< non-zero when shadows are enabled
};

using UBO_t = cs237::UniformBuffer<UB>;

/// the descriptor set numbers for the various uniforms
constexpr int kUBODescSetID = 0;        ///< per-frame UBO descriptor set is 0
constexpr int kCMapDescSetID = 1;       ///< per-drawable color-map descriptor set is 1
constexpr int kSMapDescSetID = 2;       ///< shadow-map descriptor set is 2

#endif // !_UNIFORMS_HPP_
