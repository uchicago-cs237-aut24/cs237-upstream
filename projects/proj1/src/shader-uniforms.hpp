/*! \file shader-uniforms.hpp
 *
 * Type definitions for shader uniform data.  These definitions should agree
 * with the declarations in the shader files.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _SHADER_UNIFORMS_HPP_
#define _SHADER_UNIFORMS_HPP_

#ifndef _CS237_HPP_
#include "cs237/cs237.hpp"
#endif

/// The layout of a uniform buffer for the scene-specific information.
//
struct SceneUB {
    alignas(16) glm::mat4 viewM;        //!< world-to-camera-space view transform
    alignas(16) glm::mat4 P;            //!< projection transform
    /* lighting support */
    alignas(16) glm::vec3 lightDir;     //!< vector pointing toward directional light
    alignas(16) glm::vec3 lightColor;   //!< intensity of light
    alignas(16) glm::vec3 ambLight;     //!< intensity of ambient light
};

/// the type of the uniform buffer object
using SceneUBO_t = cs237::UniformBuffer<SceneUB>;

/// Per-instance data, which we communicate using push constants
struct PushConsts {
    /* stuff for vertex shaders */
    alignas(16) glm::mat4 toWorld;      //!< model transform maps to world space
    alignas(16) glm::mat3 normToWorld;  //!< model transform for normal vectors
    /* shading support */
    alignas(16) glm::vec3 color;        //!< uniform color for object
};

#endif // !_SHADER_UNIFORMS_HPP_
