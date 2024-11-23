/*! \file texture.fsh
 *
 * \brief The fragment shader for rendering in TEXTURING mode.
 *
 * The purpose of this shader is to support viewing the scene without using the
 * G-buffer renderer.  We only use the directional light and do not implement
 * specular lighting (but emissive lights are supported)
 *
 * \author John Reppy
 */

/* CMSC23700 Project 4 sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 410

// to select the source of a component
#define NoComponent 0
#define UniformComponent 1
#define SamplerComponent 2

/* Uniforms */
uniform int             emissiveSrc;    // source of the emissive lighting component
uniform vec3            ambIntensity;   // ambient light intensity
uniform vec3            lightIntensity; // the directional light's intensity
uniform vec3            lightDir;       // the light's direction in world space
uniform vec3            diffuseC;       // diffuse color when there is no diffuse map
uniform sampler2D       diffuseMap;     // the object's diffuse-color-map texture
uniform bool            hasDiffuseMap;  // true when there is a diffuse map.
uniform vec3            emissiveC;      // emissive color when emissiveSrc == UniformComponent
uniform sampler2D       emissiveMap;    // the object's emissive map when
                                        // emissiveSrc == SamplerComponent

/* Inputs */
in VS_OUT {
    vec3        norm;           // interpolated vertex normal in world coordinates
    smooth vec2 texCoord;       // vertex tex coordinate
} fsIn;

/* Outputs */
layout (location = 0) out vec4 fragColor;

void main ()
{
  // renormalize the surface normal
    vec3 norm = normalize(fsIn.norm);

  // direct-lighting contribution
    vec3 intensity = max(dot(lightDir, norm), 0.0) * lightIntensity;

  // surface color
    vec3 surfaceC;
    if (hasDiffuseMap) {
        surfaceC = texture(diffuseMap, fsIn.texCoord).rgb;
    } else {
        surfaceC = diffuseC;
    }

  // emissive lighting
    vec3 emIntensity;
    if (emissiveSrc == SamplerComponent) {
        emIntensity = texture(emissiveMap, fsIn.texCoord).rgb;
    } else if (emissiveSrc == UniformComponent) {
        emIntensity = emissiveC;
    } else {
        emIntensity = vec3(0);
    }

  // the fragment color is the ambient + emissive + diffuse
    vec3 c = clamp (emIntensity + (ambIntensity + intensity) * surfaceC, 0, 1);

    fragColor = vec4(c, 1.0);

}
