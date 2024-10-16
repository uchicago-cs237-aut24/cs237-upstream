/*! \file shader.frag
 *
 * Trivial shader that just uses the color from the vertex shader.
 */

/* CMSC23740 Sample code
 *
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 460

layout(location = 0) in vec4 color;

layout(location = 0) out vec4 fragColor;

void main ()
{
    fragColor = color;
}
