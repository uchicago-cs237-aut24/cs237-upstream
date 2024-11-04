/*! \file shader-uniforms.hpp
 *
 * Type definitions for shader uniform data.  These definitions should agree
 * with the declarations in the shader files.
 *
 * The uniforms are divided into two buffers.  The first, which is used
 * by all vertex shaders holds the camera and viewport-dependent information
 * (i.e., the model and project matrices).  The second is used by the
 * texture and normal-map renderers and holds the scene-dependent lighting
 * information.
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

/// The per-scene uniforms incorporate the camera and viewport-dependent
/// transformation matrices, as well as the lighting information
//
struct SceneUB {
    alignas(16) glm::mat4 viewM;        ///< world-to-camera-space view transform
    alignas(16) glm::mat4 projM;        ///< projection transform
    alignas(16) glm::vec3 ambLight;     ///< intensity of ambient light
    alignas(16) glm::vec3 lightDir;     ///< vector pointing toward directional light
    alignas(16) glm::vec3 lightColor;   ///< intensity of light
};

/// the type of the per-frame UBO
using SceneUBO = cs237::UniformBuffer<SceneUB>;

/// Per-instance data, which we communicate using push constants
struct PushConsts {
    alignas(16) glm::mat4 toWorld;      ///< model transform maps to world space
    alignas(16) glm::mat4 normToWorld;  ///< model transform for normal vectors.  We
                                        ///  represent this transform as a 4x4 matrix
                                        ///  for alignment purposes, but only the upper
                                        ///  3x3 is used in the shaders.
    alignas(16) glm::vec3 color;        ///< uniform color for object
};

#endif // !_SHADER_UNIFORMS_HPP_
