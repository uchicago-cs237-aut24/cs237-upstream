/*! \file geom-pass.frag
 *
 * Lab 6 sample code: fragment shader for the geometry pass
 *
 * \author John Reppy
 */

/* CMSC23740 Sample code
 *
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#version 460

/// per-mesh color map
layout (set = 1, binding = 0) uniform sampler2D colorMap;

layout (location = 0) in vec3 fNorm;    ///< interpolated vertex normal in
                                        ///  world coordinates
layout (location = 1) in vec2 fTC;      ///< vertex tex coordinate

layout (location = 0) out vec4 gbAlbedo;///< base color output
layout (location = 1) out vec4 gbNorm;  ///< world-space normal vector output

void main ()
{
    // color output
    gbAlbedo = vec4(texture(colorMap, fTC).rgb, 1);

    // world-space normal vector output
    gbNorm = vec4(normalize(fNorm), 0);

}
