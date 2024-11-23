/*! \file gbuffer.vsh
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

/* Uniforms */
uniform mat4 modelMat;			// model-to-world transform
uniform mat4 mvpMat;                    // model-view-projection transform
uniform mat3 normMat;                   // to-world transform for normal vectors
uniform bool hasNormalMap;              // true if the current mesh has a normalMap

/* Vertex attributes; locations should match the constants in mesh.hxx */
layout (location = 0) in vec3 coord;    // vertex position
layout (location = 1) in vec3 norm;     // vertex normal
layout (location = 2) in vec2 texCoord; // texture coordinate
layout (location = 3) in vec4 tanw;     // tangent + w for bump mapping

/* Outputs */
out VS_OUT {
    vec3        coord;                  // vertex position in world coordinates
    vec3        norm;                   // vertex normal in world coordinates
    vec3        tan;                    // vertex tangent in world coordinates
    vec3        bitan;                  // vertex bitangent in world coordinates
    smooth vec2 texCoord;               // vertex tex coordinate
} vsOut;

void main ()
{
    vec4 wPos = modelMat * vec4(coord, 1); // vertex world coordinates
    gl_Position = mvpMat * vec4(coord, 1); // clip-space coordinates

  // Assign outgoing variables
    vsOut.coord = wPos.xyz;
    vsOut.norm = normMat * norm;        // world coordinates for normal vector
    vsOut.texCoord = texCoord;
    if (hasNormalMap) {
      // convert tangent to world space
	vsOut.tan = mat3(modelMat) * tanw.xyz;
      // convert bitangent to world space
	vsOut.bitan = mat3(modelMat) * (tanw.w * cross(norm, tanw.xyz));
    }
}
