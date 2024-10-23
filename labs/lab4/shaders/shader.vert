/*! \file shader.vert
 *
 * Lab 4 vertex shader that uses texture mapping
 *
 * \author John Reppy
 */

/* CMSC23700 Sample code
 *
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 460

layout(binding = 0) uniform UBO {
    mat4 MV;    //!< model-view transform
    mat4 P;     //!< projection transform
} ubo;

layout (location = 0) in vec3 vPos;	//!< vertex position
layout (location = 1) in vec3 vNorm;	//!< normal vector
layout (location = 2) in vec2 tCoord;   //!< texture coordinate
layout (location = 3) in vec4 vColor;	//!< vertex color

layout (location = 0) out vec2 f_tCoord;

void main ()
{
    /** HINT: transform the vertex position */
    /** HINT: copy the texture coordinate */

}
