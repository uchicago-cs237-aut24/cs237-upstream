/*! \file texture.vvertsh
 *
 * \brief The vertex shader for rendering in texturing mode
 *
 * The purpose of this shader is to support viewing the scene
 * without using the G-buffer renderer.
 *
 * \author John Reppy
 */

/* CMSC23740 Project 5 sample code (Autumn 2024)
 *
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 460

/* Uniforms */
layout (push_constant) uniform PC {
    mat4 mvpM;          ///< model-view-projection transform
    mat4 normM;         ///< to-world transform for normal vectors
} pc;

/* Vertex attributes */
layout (location = 0) in vec3 vPos;     ///< vertex position
layout (location = 1) in vec3 vNorm;    ///< vertex normal
layout (location = 2) in vec2 vTC;      ///< texture coordinate
layout (location = 3) in vec4 vTan;     ///< tangent (unused)

/* Outputs */
layout (location = 0) out vec3 fNorm;   ///< world-space vertex normal
layout (location = 1) out vec2 fTC;     ///< texture coordinate

void main ()
{
    gl_Position = pc.mvpM * vec4(vPos, 1);
    fNorm = mat3(pc.normM) * vNorm;
    fTC = vTC;
}
