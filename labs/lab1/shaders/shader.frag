/*! \file no-light.frag
 *
 * Simple fragment shader without lighting
 *
 * \author John Reppy
 */

/* CMSC23740 Project 1 sample code (Autumn 2024)
 *
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 460

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(fragColor, 1.0);
}
