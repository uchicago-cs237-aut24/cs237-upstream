/*! \file shader.vert
 *
 * Simple vertex shader that applies the model, view, and projection transforms.
 */

/* CMSC23740 Sample code
 *
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 460

/** HINT: add the Uniform Buffer declaration */

layout (location = 0) in vec3 vPos;             //!< input vertex  position
layout (location = 1) in vec3 vColor;           //!< input vertex  position

layout(location = 0) out vec4 color;            //!< output color

void main ()
{
/** HINT: compute gl_Position and color */
}
