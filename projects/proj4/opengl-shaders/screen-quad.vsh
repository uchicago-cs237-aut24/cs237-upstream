/*! \file screen-quad.vsh
 *
 * Vertex shader to draw the screen quad as two triangles
 *
 * \author John Reppy
 */

/* CMSC23700 Project 4 sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy
 * All rights reserved.
 */
#version 410

void main ()
{
    const vec2 quadVerts[6] = vec2[6](
	vec2( -1.0, -1.0), vec2(  1.0, -1.0), vec2( -1.0,  1.0),
	vec2( -1.0,  1.0), vec2(  1.0, -1.0), vec2(  1.0,  1.0));

    gl_Position = vec4(quadVerts[gl_VertexID], 0.0, 1.0);

}
