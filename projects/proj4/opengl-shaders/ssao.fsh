/*! \file ssao.fsh
 *
 * Fragment shader for the final rendering pass using SSAO.
 *
 * \author John Reppy
 */

/* CMSC23700 Project 4 sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy
 * All rights reserved.
 */
#version 410

#define NSAMPLES        10              // number of random points to sample
#define NSTEPS          4               // number of steps in a given direction
#define RADIUS          0.05            // scaling factor for steps

/* UNIFORMS */
uniform vec2 screenSize;                // screen size of the window
uniform vec3 ambient;                   // ambient lighting
uniform sampler2D gbDiffuseBuf;         // diffuse-color buffer
uniform sampler2D gbEmissiveBuf;        // emissive-color buffer
uniform sampler2D gbNormalBuf;		// world-space normals
uniform sampler2D gbDepthBuf;           // camera-space depth value
uniform sampler2D gbFinalBuf;           // final buffer (holds diffuse and specular)

layout (std140) uniform RandomDirUB {
    vec3        dirs[64];
    float       radii[256];
} randomVecs;

/* OUTPUTS */
layout (location = 0) out vec4 fragColor;

void main ()
{
  // convert window coordinates of [0..1] range
    vec2 fragXY = gl_FragCoord.xy / screenSize;

  // get surface normal vector
    vec3 norm = texture(gbNormalBuf, fragXY).xyz;

  // get depth value
    float depth = texture(gbDepthBuf, fragXY).x;

  // local temporaries for computing screen-space occlusion
    int i, j;
    float occ = 0.0;
    float total = 0.0;

  // pseudo-random number based on coordinates and depth
    int n = (
	int(gl_FragCoord.x * 7123.2315 + 125.232) *
	int(gl_FragCoord.y * 3137.1519 + 234.8)) ^
	int(depth);

  // get a random radius
    float r = randomVecs.radii[n & 255];
r = 0.5;

    for (i = 0;  i < NSAMPLES;  i++) {
      // get direction
        vec3 dir = randomVecs.dirs[i];
      // make sure that dir is in the right hemisphere
        if (dot(norm, dir) < 0.0) {
	    dir = -dir;
	}
      // f is the distance that we've stepped in the direction dir
	float f = 0.0;
      // z is the current depth
	float z = depth;
      //
	for (j = 0;  j < NSTEPS;  j++) {
	  // step forward by the random radius
	    f += r;
	  // adjust depth
	    z -= dir.z * r;
	  // get depth at new sample point
	    float d = texture(gbDepthBuf, fragXY.xy + RADIUS * f * dir.xy).x;
	  // If the sample point is occluded, add to occlusion factor
	    if ((z - d) >= 0) {
	      // compute a weighted contribution to the occlusion
		float amt = d - depth;
		occ += NSTEPS / (1.0 + amt*amt);
	    }
	}
    }

  // calculate occlusion amount
    float aoScale = mix(0.2, 1.0 - (occ / (NSTEPS*NSAMPLES)), 1.0);

    vec3 color = aoScale * ambient * texture(gbDiffuseBuf, fragXY).rgb
            + texture(gbEmissiveBuf, fragXY).rgb
            + texture(gbFinalBuf, fragXY).rgb;

    if (isnan(aoScale)) { color = vec3(1.0, 0.8, 0.8); } else { color = vec3(aoScale); }
//    color = vec3(0.5 * norm + 0.5);

    fragColor = vec4(clamp(color, 0, 1), 1);
}
