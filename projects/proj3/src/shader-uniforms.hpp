/*! \file shader-uniforms.hpp
 *
 * Type definitions for shader uniform data.  These definitions should agree
 * with the declarations in the shader files.
 *
 * The uniforms are divided into three buffers.
 *
 *    FrameUB: is used by the vertex shaders and holds the camera and
 *      viewport-dependent information.  There is one of these per frame.
 *
 *    ShadowUB: is used for the shadow-mode passes and holds the world-to-light
 *      transform and the index of the spotlight being rendered.  There is one
 *      of these per light in the scene.
 *
 *    SceneUB: holds the per-scene information about the lights.  There is only
 *      one of these.
 *
 * We use push constants to communicate the per-mesh model to world-space transforms.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _SHADER_UNIFORMS_HPP_
#define _SHADER_UNIFORMS_HPP_

#ifndef _CS237_HPP_
#include "cs237/cs237.hpp"
#endif

/// The per-frame uniforms used in the vertex shaders
//
struct FrameUB {
    alignas(16) glm::mat4 viewM;        ///< view matrix
    alignas(16) glm::mat4 projM;        ///< projection matrix
    int enableNormalMap;                ///< non-zero when normal-mapping is enabled
};

/// the type of the vertex-shader UBO
using FrameUBO = cs237::UniformBuffer<FrameUB>;

/// the per-light information used in computing lighting information in shadow
/// mode.
struct ShadowUB {
    alignas(16) glm::mat4 shadowM;      ///< world-space to light-space transform
    int lightID;                        ///< index of light being rendered
    float shadowFactor;                 ///< scaling factor for in-shadow fragments
};

/// the type of the per-light shadow-info UBO
using ShadowUBO = cs237::UniformBuffer<ShadowUB>;

/// The shader representation of a spot light
//
struct SpotLightUB {
    alignas(16) glm::vec3 position;     ///< the world position of the light
    alignas(16) glm::vec3 direction;    ///< vector pointing toward directional light
    alignas(16) glm::vec3 intensity;    ///< intensity of the light
    alignas(16) glm::vec3 atten;        ///< k0, k1, and k2 as vec3
    float cosCutoff;                    ///< cosine of the cutoff angle
    float exponent;                     ///< fall-off exponent
};

/// The per-scene per-scene lighting information
//
struct SceneUB {
    alignas(16) SpotLightUB lights[4];  ///< array of spot lights
    alignas(16) glm::vec3 ambLight;     ///< intensity of ambient light
    int nLights;                        ///< the number of lights
};

/// the type of the per-scene lighting information UBO
using SceneUBO = cs237::UniformBuffer<SceneUB>;

/// Per-instance data, which we communicate to the vertex shader using push constants
struct PushConsts {
    alignas(16) glm::mat4 toWorld;      ///< model transform maps to world space
    alignas(16) glm::mat4 normToWorld;  ///< model transform for normal vectors.  We
                                        ///  represent this transform as a 4x4 matrix
                                        ///  for alignment purposes, but only the upper
                                        ///  3x3 is used in the shaders.
};

#endif // !_SHADER_UNIFORMS_HPP_
