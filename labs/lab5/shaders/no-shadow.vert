/*! \file no-shadow.vert
 *
 * Lab 5 sample code: vertex shader for the scene.
 *
 * \author John Reppy
 */

/* CMSC23740 Sample code
 *
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 460

// matrix for mapping 2x2x1 clip coordinates to [0,1] range
// in column-major order
const mat4 biasMat = mat4(
    0.5, 0.0, 0.0, 0.0,  // first column
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.5, 0.5, 0.0, 1.0);

// we use push-constants for the per-mesh uniform data
layout (push_constant) uniform PC {
    mat4 modelMat;      ///< model-space to world-space transform
    vec3 color;         ///< flat color to use when texturing is disabled
} pc;

// we use a UBO for the per-frame uniform data
layout (set = 0, binding = 0) uniform UB {
    mat4 viewMat;       ///< view matrix
    mat4 projMat;       ///< projection matrix
    mat4 shadowMat;     ///< world-space to light-space transform
    vec3 lightDir;      ///< light direction in eye coordinates
    int enableTexture;  ///< non-zero when texturing is enabled
    int enableShadows;  ///< non-zero when shadows are enabled (should be VK_FALSE)
} ubo;

layout (location = 0) in vec3 vPos;	///< vertex position in model coordinates
layout (location = 1) in vec3 vNorm;    ///< vertex normal in model coordinates
layout (location = 2) in vec2 vTC;      ///< vertex texture coordinate

layout (location = 0) out vec3 fNorm;   ///< interpolated vertex normal in
                                        ///  world coordinates
layout (location = 1) out vec2 fTC;     ///< vertex tex coordinate

void main ()
{
    // clip coordinates for vertex
    gl_Position = ubo.projMat * ubo.viewMat * pc.modelMat * vec4(vPos, 1);

    // eye-space normal; we are assuming the 3x3 linear part of modelMat is
    // orthogonal and, thus, can be used to transform normals
    fNorm = mat3x3(pc.modelMat) * vNorm;

    // propagate texture coords to fragment shader
    fTC = vTC;
}
