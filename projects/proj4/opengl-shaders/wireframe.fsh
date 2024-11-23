/*! \file wireframe.fsh
 *
 * \brief Fragment shader for drawing the wireframe
 *
 * \author John Reppy
 */

/* CMSC23700 Project 4 sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 410

uniform vec4 color;

layout (location = 0) out vec4 fragColor;

void main ()
{
    fragColor = color;
}
