/*! \file render.frag
 *
 * Lab 7 sample code: fragment shader for rendering the computed image
 *
 * \author John Reppy
 */

/* CMSC23740 Sample code
 *
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 460

// The computed image
layout (set = 0, binding = 0) uniform sampler2D image;

layout (location = 0) in vec2 fTC;      ///< output screen coordinate

layout (location = 0) out vec4 outColor;

void main ()
{
    outColor = vec4(texture(image, fTC).rgb, 1.0);
}
