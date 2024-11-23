/*! \file texture.vsh
 *
 * \brief The vertex shader for rendering in TEXTURING mode
 *
 * The purpose of this shader is to support viewing the scene without using the
 * G-buffer renderer.
 *
 * \author John Reppy
 */

/* CMSC23700 Project 4 sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 410

/* Uniforms */
uniform mat4 mvpMat;                    // model-view-projectiom transform
uniform mat3 normMat;                   // to-world transform for normal vectors

/* Vertex attributes; locations should match the constants in mesh.hxx */
layout (location = 0) in vec3 coord;    // vertex position
layout (location = 1) in vec3 norm;     // vertex normal
layout (location = 2) in vec2 texCoord; // texture coordinate

/* Outputs */
out VS_OUT {
    vec3        norm;           // interpolated vertex normal in world coordinates
    smooth vec2 texCoord;       // vertex tex coordinate
} vsOut;

void main ()
{
    gl_Position = mvpMat * vec4(coord, 1);
    vsOut.norm = normMat * norm;
    vsOut.texCoord = texCoord;
}
