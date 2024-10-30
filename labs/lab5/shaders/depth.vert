/*! \file depth.vert
 *
 * Lab 5 sample code: vertex shader for the rendering the depth values for the light.
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
    vec3 color;         ///< flat color to use when texturing is disabled
} pc;

// we use a UBO for the per-frame uniform data
layout (set = 0, binding = 0) uniform UB {
    mat4 viewMat;       ///< view matrix
    mat4 projMat;       ///< projection matrix
    mat4 shadowMat;     ///< world-space to light-space transform
    vec3 lightDir;      ///< light direction in eye coordinates
    int enableTexture;  ///< non-zero when texturing is enabled
    int enableShadows;  ///< non-zero when shadows are enabled
} ubo;

layout (location = 0) in vec3 vPos;	///< vertex position in model coordinates

void main ()
{
    // light-space coordinates
    gl_Position = ubo.shadowMat * pc.modelMat * vec4(vPos, 1);
}
