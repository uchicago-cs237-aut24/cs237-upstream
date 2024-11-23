/*! \file obj-to-clip.vsh
 *
 * This is a minimal shader that converts object-space vertices to
 * clip-space coordinates.  It is used for the stencil and spotlight
 * shaders.
 *
 * \author John Reppy
 */

/* CMSC23700 Project 4 sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy
 * All rights reserved.
 */

#version 410

uniform mat4 mvpMat;                    // model-view-projectiom transform

layout (location = 0) in vec3 coord;    // vertex position

void main ()
{
    gl_Position = mvpMat * vec4(coord, 1);
}
