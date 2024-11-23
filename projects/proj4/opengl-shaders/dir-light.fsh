/*! \file dir-light.fsh
 *
 * Direct-light lighting pass for deferred rendering.  This fragment shader
 * needs to compute the diffuse and specular lighting contributions for
 * the given spotlight.
 *
 * \author John Reppy
 */

/* CMSC23700 Project 4 sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy
 * All rights reserved.
 */

#version 410

#define M_1_256		0.00390625	// 1/256

struct DirLight {
    vec3 direction;			// world-space light direction
    vec3 intensity;			// light intensity
};

/* UNIFORMS */
uniform vec2 screenSize;                // screen size of the window
uniform DirLight light;                 // the directional light
uniform sampler2D gbCoordBuf;		// world-space coordinates + specular exponent
uniform sampler2D gbDiffuseBuf;		// diffuse-color
uniform sampler2D gbSpecularBuf;	// specular-color
uniform sampler2D gbNormalBuf;		// world-space normals
uniform vec3 cameraPos;			// the world-space camera position

/* OUTPUTS */
layout (location = 0) out vec4 fragColor;

void main ()
{
  // convert window coordinates to [0..1] range
    vec2 fragXY = gl_FragCoord.xy / screenSize;

  // get surface normal vector
    vec3 norm = texture(gbNormalBuf, fragXY).xyz;

  // check for fragments facing away from the light
    float cosAngle = dot(norm, -light.direction);

    if (cosAngle >= M_1_256) {  // >= 1/256
      // diffuse lighting
        vec3 diffuseC = texture(gbDiffuseBuf, fragXY).rgb;
	vec3 color = cosAngle * light.intensity * diffuseC;
      // specular lighting
      // specular intensity is (h dot n)^sharpness, where h is the unit vector
      // halfway between toCamera and toLight.
        vec4 posPlusW = texture(gbCoordBuf, fragXY);
	vec3 toCamera = normalize(cameraPos - posPlusW.xyz);
	float cosPhi = dot (normalize(-light.direction + toCamera), norm);
	if (cosPhi > M_1_256) {
	    vec3 specularC = texture(gbSpecularBuf, fragXY).rgb;
	    float specI = pow(cosPhi, posPlusW.w);
	    color += specI * texture(gbSpecularBuf, fragXY).rgb * light.intensity;
	}
	fragColor = vec4(clamp(color, 0, 1), 1.0);
    }
    else {
	discard;
    }
}
