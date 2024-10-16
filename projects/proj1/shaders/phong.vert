/*! \file phong.vert
 *
 * Project 1 vertex shader for Phong shading.
 *
 * \author John Reppy
 */

/* CMSC23740 Project 1 sample code (Autumn 2024)
 *
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 460

layout(push_constant) uniform PC {
    mat4 toWorld;      //!< model transform maps to world space
    mat3 normToWorld;  //!< model transform for normal vectors
    /* lighting support */
    vec3 color;        //!< uniform color for object
} pconsts;

layout(binding = 0) uniform UBO {
    mat4 viewM;         //!< world-to-camera-space view transform
    mat4 P;             //!< projection transform
    /* shading/lighting support */
    vec3 lightDir;      //!< vector pointing toward directional light
    vec3 lightColor;    //!< intensity of light
    vec3 ambLight;      //!< intensity of ambient light
} ubo;

layout (location = 0) in vec3 vPos;	//!< vertex position
layout (location = 1) in vec3 vNorm;	//!< vertex normal

layout (location = 0) out vec3 color;   //! object color
layout (location = 1) out vec3 tNorm;   //!< world-space normal

void main ()
{
    /** HINT: your code here */
}
