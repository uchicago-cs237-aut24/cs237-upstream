/*! \file flat.frag
 *
 * Project 1 fragment shader for basic rendering without lighting.
 *
 * \author John Reppy
 */

/* CMSC23740 Project 1 sample code (Autumn 2024)
 *
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 460

layout(location = 0) in vec3 color;

layout(location = 0) out vec4 fragColor;

void main ()
{
    fragColor = vec4(color, 1);
}
