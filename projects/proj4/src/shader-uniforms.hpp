/*! \file shader-uniforms.hpp
 *
 * Type definitions for shader uniform data.  These definitions should agree
 * with the declarations in the shader files.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _SHADER_UNIFORMS_HPP_
#define _SHADER_UNIFORMS_HPP_

#ifndef _CS237_HPP_
#include "cs237/cs237.hpp"
#endif

/// The push constants for wire-frame rendering mode
struct WireFramePushConsts {
    alignas(16) glm::mat4 mvpM;         ///< model-view-projection transform
    alignas(16) glm::vec3 color;        ///< object color
};

/// The push constants for texture rendering mode
struct TexturePushConsts {
    alignas(16) glm::mat4 mvpM;         ///< model-view-projection transform
    alignas(16) glm::mat4 normToWorld;  ///< model transform for normal vectors.  We
                                        ///  represent this transform as a 4x4 matrix
                                        ///  for alignment purposes, but only the upper
                                        ///  3x3 is used in the shaders.
};

/// The per-scene lighting uniform buffer
struct LightingUB {
    alignas(16) glm::vec3 lightDir;             ///< unit vector pointing toward light
    alignas(16) glm::vec3 lightIntensity;       ///< intensity of directional light
    alignas(16) glm::vec3 ambIntensity;         ///< intensity of ambient light
    float shadowFactor;                         ///< scaling for shadowed fragments
/** HINT: you may want to add the following flags for enabling/disabling lighting
 ** modes to this structure (in which case, it becomes a per-frame buffer).
 **
    int enableDirLight;                         ///< enable directional light
    int enableSpotLights;                       ///< enable spot lights
    int enableEmissiveLighting;                 ///< enable emissive lighting
    int enableShadows;                          ///< enable shadows (extra credit)
 **/
};

using LightingUBO = cs237::UniformBuffer<LightingUB>;

/// constants to specify the source of a material property (also see mesh.hpp)
constexpr int kNoProperty = 0;          ///< no color component of that type
constexpr int kUniformProperty = 1;     ///< get color from UBO
constexpr int kSamplerProperty = 2;     ///< get color from sampler

/// The per-object-group uniform buffer that specifies surface-materials
//
struct MaterialUB {
    alignas(16) glm::vec3 albedo;       ///< albedo when there is no color map
    alignas(16) glm::vec3 emissive;     ///< emissive color when there is no sampler
    alignas(16) glm::vec4 specular;     ///< optional specular color (exponent is phong)
    int albedoSrc;                      ///< true if the color-map sampler is defined
    int emissiveSrc;                    ///< source of emissive lighting property
    int specularSrc;                    ///< source of specular lighting property
    int hasNormalMap;                   ///< true if the normal-map sampler is defined
};

using MaterialUBO = cs237::UniformBuffer<MaterialUB>;

/** HINT: define the layouts for the various uniform buffers
 ** that you use in the geometry pass of your renderer here.
 **/

#endif // !_SHADER_UNIFORMS_HPP_
