/*! \file wireframe.vert
 *
 * \brief The vertex shader for rendering in wire-frame mode
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
    vec3 color;         ///< object color
} pc;

/* Vertex attributes */
layout (location = 0) in vec3 vPos;     ///< vertex position
layout (location = 1) in vec3 vNorm;    ///< vertex normal (unused)
layout (location = 2) in vec2 vTC;      ///< texture coordinate (unused)
layout (location = 3) in vec4 vTan;     ///< tangent (unused)

/* Outputs */
layout (location = 0) out vec3 fColor;  // vertex color (out)

void main ()
{
    gl_Position = pc.mvpM * vec4(vPos, 1);
    fColor = pc.color;
}
