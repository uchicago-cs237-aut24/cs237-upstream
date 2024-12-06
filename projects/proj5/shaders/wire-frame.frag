/*! \file wire-frame.frag
 *
 * \brief The fragment shader for rendering in wire-frame mode
 *
 * \author John Reppy
 */

/* CMSC23740 Project 5 sample code (Autumn 2024)
 *
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 460

layout (location = 0) in vec3 fColor;  // vertex color (out)

layout (location = 0) out vec4 fragColor;

void main ()
{
    fragColor = vec4(fColor, 1);
}
