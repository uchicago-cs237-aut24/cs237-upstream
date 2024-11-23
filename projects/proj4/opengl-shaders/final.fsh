/*! \file final.fsh
 *
 * \author John Reppy
 */

/* CMSC23700 Project 4 sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy
 * All rights reserved.
 */
#version 410

/* UNIFORMS */
uniform vec2 screenSize;                // screen size of the window
uniform vec3 ambient;                   // ambient lighting
uniform sampler2D gbDiffuseBuf;         // diffuse-color buffer
uniform sampler2D gbEmissiveBuf;        // emissive-color buffer
uniform sampler2D gbFinalBuf;           // final buffer (holds diffuse and specular)

/* OUTPUTS */
layout (location = 0) out vec4 fragColor;

void main ()
{
  // convert window coordinates of [0..1] range
    vec2 fragXY = gl_FragCoord.xy / screenSize;
    vec3 color = ambient * texture(gbDiffuseBuf, fragXY).rgb
            + texture(gbEmissiveBuf, fragXY).rgb
            + texture(gbFinalBuf, fragXY).rgb;

    fragColor = vec4(clamp(color, 0, 1), 1);
}
