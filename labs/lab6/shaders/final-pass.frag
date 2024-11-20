/*! \file final-pass.frag
 *
 * Lab 6 sample code: fragment shader for the final pass
 *
 * \author John Reppy
 */

/* CMSC23740 Sample code
 *
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 460

/// constants that specify what to display on the screen.
const int kRenderScene = 0;
const int kShowAlbedoBuffer = 1;
const int kShowNormalBuffer = 2;

layout (set = 0, binding = 0) uniform FinalUB {
    vec3 lightDir;      ///< unit vector pointing toward the light
    vec3 lightColor;    ///< the intensity of the light
    vec3 ambient;       ///< the ambient color for the scene
    vec3 background;    ///< the background color for the scene
    int mode;           ///< what to display on the screen
} ubo;

layout (location = 0) in vec2 fTC;      ///< rasterized screen coordinate

layout (location = 0) out vec4 outColor;

// G-Buffer
layout (set = 1, binding = 0) uniform sampler2D gbAlbedo;
layout (set = 1, binding = 1) uniform sampler2D gbNormal;

void main ()
{
    vec3 color;

    if (ubo.mode == kRenderScene) {
        // get the base color for the pixel
        vec4 albedo = texture(gbAlbedo, fTC);

        if (albedo.a == 0.0) {
            // no geometry for this pixel, so use background color
            color = ubo.background;
        } else {
            vec3 normal = normalize(texture(gbNormal, fTC).rgb);

            float scale = dot(ubo.lightDir, normal);
            if (scale > 0.0) {
                color = (ubo.ambient + scale*ubo.lightColor) * albedo.rgb;
            } else {
                // geometry is facing away from the light
                color = ubo.ambient * albedo.rgb;
            }
        }
    }
    else if (ubo.mode == kShowAlbedoBuffer) {
        // get the base color for the pixel
        vec4 albedo = texture(gbAlbedo, fTC);
        if (albedo.a == 0.0) {
            // no geometry for this pixel, so use background color
            color = ubo.background;
        } else {
            color = albedo.rgb;
        }
    } else { /* (ubo.mode == kShowNormalBuffer) */
        vec3 normal = normalize(texture(gbNormal, fTC).rgb);
        color = 0.5 * (normal + 1);
    }

    outColor = vec4(color, 1.0);
}
