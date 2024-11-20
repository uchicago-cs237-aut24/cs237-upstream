/*! \file uniforms.hpp
 *
 * CMSC 23740 Autumn 2024 Lab 6.
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

/// constants that specify what to display on the screen.  These should match the
/// values in the final fragment shader
constexpr int kRenderScene = 0;
constexpr int kShowAlbedoBuffer = 1;
constexpr int kShowNormalBuffer = 2;

// we use push-constants for the per-mesh uniform data
struct PC {
    alignas(16) glm::mat4 modelMat;     ///< model-space to world-space transform
    alignas(16) glm::mat4 normMat;      ///< model-space to world-space transform for normals
};

// we use a UBO for the per-frame uniform data during the geometry pass
struct GeomUB {
    alignas(16) glm::mat4 viewMat;              ///< view matrix
    alignas(16) glm::mat4 projMat;              ///< projection matrix
};

using GeomUBO_t = cs237::UniformBuffer<GeomUB>;

// we use a uniform buffer for the lighting information during the final pass
struct FinalUB {
    alignas(16) glm::vec3 lightDir;     ///< unit vector pointing toward the light
    alignas(16) glm::vec3 lightColor;   ///< the intensity of the light
    alignas(16) glm::vec3 ambient;      ///< the background color for the scene
    alignas(16) glm::vec3 background;   ///< the background color for the scene
    int mode;               ///< render mode
};

using FinalUBO_t = cs237::UniformBuffer<FinalUB>;

/// descriptor set and binding IDs
constexpr uint32_t kGeomUBSet = 0;              ///< Geometry-pass UBO
constexpr uint32_t kGeomUBBind = 0;
constexpr uint32_t kColorSamplerSet = 1;        ///< Geometry-pass color-map sampler
constexpr uint32_t kColorSamplerBind = 0;
constexpr uint32_t kFinalUBSet = 0;             ///< Final-pass UBO
constexpr uint32_t kFinalUBBind = 0;
constexpr uint32_t kGBufSamplerSet = 1;         ///< Final-pass G-Buffer samplers
constexpr uint32_t kGBufAlbedoBind = 0;
constexpr uint32_t kGBufNormalBind = 1;

#endif // !_UNIFORMS_HPP_
