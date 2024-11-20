/*! \file geom-pass.vert
 *
 * Lab 6 sample code: vertex shader for the geometry pass
 *
 * \author John Reppy
 */

/* CMSC23740 Sample code
 *
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 460

// we use push-constants for the per-mesh uniform data
layout (push_constant) uniform PC {
    mat4 modelMat;      ///< model-space to world-space transform
    mat4 normMat;       ///< model-space to world-space transform for normals
} pc;

// we use a UBO for the per-frame uniform data
layout (set = 0, binding = 0) uniform UB {
    mat4 viewMat;       ///< view matrix
    mat4 projMat;       ///< projection matrix
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

    // world-space normal
    fNorm = mat3(pc.normMat) * vNorm;

    // propagate texture coords to fragment shader
    fTC = vTC;
}

