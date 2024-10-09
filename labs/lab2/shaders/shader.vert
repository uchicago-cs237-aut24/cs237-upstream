/*! \file shader.vert
 *
 * Simple vertex shader with fixed coordinates for a triangle.
 *
 * \author John Reppy
 */

/* CMSC23740 Lab 2 sample code (Autumn 2024)
 *
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 460

/* vertex data */
layout (location = 0) in vec2 vPos;     //!< vertex position
layout (location = 1) in vec3 vColor;   //!< vertex color

/* shader output */
layout (location = 0) out vec3 fragColor;      ///< output fragment color

void main() {
    fragColor = vColor;
}
