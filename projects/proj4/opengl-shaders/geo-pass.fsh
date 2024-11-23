/*! \file gbuffer.fsh
 *
 * Geometry-pass vertex shader for deferred rendering.
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
uniform int             specularSrc;    // source of the specular lighting component
uniform int             emissiveSrc;    // source of the emissive lighting component
uniform vec3            diffuseC;       // diffuse color when there is no diffuse map
uniform sampler2D       diffuseMap;     // the object's diffuse-color-map texture
uniform bool            hasDiffuseMap;  // true when there is a diffuse map.
uniform vec3            specularC;      // specular color when specularSrc == UniformComponent
uniform sampler2D       specularMap;    // the object's specular map when
                                        // specularSrc == SamplerComponent
uniform float           sharpness;      // specular exponent
uniform vec3            emissiveC;      // emissive color when emissiveSrc == UniformComponent
uniform sampler2D       emissiveMap;    // the object's emissive map when
                                        // emissiveSrc == SamplerComponent
uniform bool            hasNormalMap;   // true if the current mesh has a normalMap
uniform sampler2D       normalMap;

/* Inputs */
in VS_OUT {
    vec3        coord;                  // vertex position in world coordinates
    vec3        norm;                   // vertex normal in world coordinates
    vec3        tan;                    // vertex tangent in world coordinates
    vec3        bitan;                  // vertex bitangent in world coordinates
    smooth vec2 texCoord;               // vertex tex coordinate
} fsIn;

/* Outputs -- indices should agree with order in gbuffer.hxx */
layout (location = 0) out vec4 coordOut;
layout (location = 1) out vec3 diffuseOut;
layout (location = 2) out vec3 specularOut;
layout (location = 3) out vec3 emissiveOut;
layout (location = 4) out vec3 normalOut;

void main ()
{
  // coordinate buffer
    coordOut.xyz = fsIn.coord;

  // diffuse color buffer
    if (hasDiffuseMap) {
        diffuseOut = texture(diffuseMap, fsIn.texCoord).rgb;
    } else {
        diffuseOut = diffuseC;
    }

  // specular color buffer; sharpness is stored in coordOut.w
    if (specularSrc == NoComponent) {
        specularOut = vec3(0);
        coordOut.w = 0;
    }
    else {
        coordOut.w = sharpness;
        if (specularSrc == SamplerComponent) {
            specularOut = texture(specularMap, fsIn.texCoord).rgb;
        } else {
            specularOut = specularC;
        }
    }

  // emissive buffer
    if (emissiveSrc == SamplerComponent) {
        emissiveOut = texture(emissiveMap, fsIn.texCoord).rgb;
    } else if (emissiveSrc == UniformComponent) {
        emissiveOut = emissiveC;
    } else {
        emissiveOut = vec3(0);
    }

  // normal buffer
    if (hasNormalMap) {
        vec3 norm = normalize(2.0 * texture(normalMap, fsIn.texCoord).rgb - vec3(1));
        vec3 N = normalize(fsIn.norm);
	vec3 T = normalize(fsIn.tan);
	vec3 B = normalize(fsIn.bitan);
        normalOut = mat3(T, B, N) * norm;
    } else {
        normalOut = normalize(fsIn.norm);
    }

}
