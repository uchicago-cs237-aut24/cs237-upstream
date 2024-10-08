/*! \file shader.frag
 *
 * Simple fragment shader without lighting
 *
 * \author John Reppy
 */

/* CMSC23740 Lab 2 sample code (Autumn 2024)
 *
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 460

/* input from rasterizer */
layout(location = 0) in vec3 fragColor;

/* output to frame buffer */
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(fragColor, 1.0);
}
