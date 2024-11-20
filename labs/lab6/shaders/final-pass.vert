/*! \file final-pass.vert
 *
 * Lab 6 sample code: vertex shader for the final pass
 *
 * \author John Reppy
 */

/* CMSC23740 Sample code
 *
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 460

layout (location = 0) out vec2 fTC;      ///< output screen coordinate

void main ()
{
    // draws a triangle that covers the full screen in CW order.  See
    // https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
    // for details
    fTC = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(fTC * 2.0f + -1.0f, 0.0f, 1.0f);
}
