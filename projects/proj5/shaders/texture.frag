/*! \file texture.frag
 *
 * \brief The fragment shader for rendering in texturing mode.
 *
 * The purpose of this shader is to support viewing the scene without using the
 * G-buffer renderer.  Lighting is limited to the ambient and directional lights
 * with diffuse lighting.
 *
 * \author John Reppy
 */

/* CMSC23740 Project 5 sample code (Autumn 2024)
 *
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 460

// to select the source of a component
const int kNoProperty = 0;      // no color component of that type
const int kUniformProperty = 1; // get color from UBO
const int kSamplerProperty = 2; // get color from sampler

/* Uniforms */
layout (set = 0, binding = 0) uniform LightingUB {
    vec3 lightDir;              ///< unit vector pointing toward light
    vec3 lightIntensity;        ///< intensity of directional light
    vec3 ambIntensity;          ///< intensity of ambient light
    float shadowFactor;         ///< scaling for shadowed fragments
} lightingUBO;

layout (set = 1, binding = 0) uniform MaterialUB {
    vec3 albedo;                ///< albedo when there is no color map
    vec3 emissive;              ///< emissive color when there is no sampler
    vec4 specular;              ///< optional specular color (exponent is phong)
    int albedoSrc;              ///< true if the color-map sampler is defined
    int emissiveSrc;            ///< source of emissive lighting component
    int specularSrc;            ///< source of specular lighting component
    bool hasNormalMap;          ///< true if the normal-map sampler is defined
} materialUBO;

layout (set = 1, binding = 1) uniform sampler2D colorMap;

/* Inputs */
layout (location = 0) in vec3 fNorm;    ///< world-space vertex normal
layout (location = 1) in vec2 fTC;      ///< texture coordinate

/* Outputs */
layout (location = 0) out vec4 fragColor;

void main ()
{
    // renormalize the surface normal
    vec3 norm = normalize(fNorm);

    // direct-lighting contribution
    float lightFactor = max(dot(lightingUBO.lightDir, norm), 0.0);
    vec3 intensity = lightingUBO.ambIntensity + lightFactor * lightingUBO.lightIntensity;

  // surface color
    vec3 albedo;
    if (materialUBO.albedoSrc == kSamplerProperty) {
        albedo = texture(colorMap, fTC).rgb;
    } else if (materialUBO.albedoSrc == kUniformProperty) {
        albedo = materialUBO.albedo;
    } else {
        albedo = vec3(1,1,1);
    }

    fragColor = vec4(clamp (intensity * albedo, 0, 1), 1);
}
