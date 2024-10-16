/*! \file phong.frag
 *
 * Project 1 fragment shader for Phong shading.
 *
 * \author John Reppy
 */

/* CMSC23740 Project 1 sample code (Autumn 2024)
 *
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 460

layout(binding = 0) uniform UBO {
    mat4 viewM;         //!< world-to-camera-space view transform
    mat4 P;             //!< projection transform
    /* shading/lighting support */
    vec3 lightDir;      //!< vector pointing toward directional light
    vec3 lightColor;    //!< intensity of light
    vec3 ambLight;      //!< intensity of ambient light
} ubo;

layout(location = 0) in vec3 color;
layout(location = 1) in vec3 tNorm;

layout(location = 0) out vec4 fragColor;

void main ()
{
    /** HINT: your code here */
}
