/*! \file spot-light.fsh
 *
 * Spot-light lighting pass for deferred rendering.  This fragment shader
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

#define M_1_256         0.00390625      // 1/256

struct SpotLight {                      // spot-light properties
    vec3 position;                      // light world-space position
    vec3 intensity;                     // light intensity
    float constant;                     // constant attenuation coefficient
    float linear;                       // linear attenuation coefficient
    float quadratic;                    // quadratic attenuation coefficient
    vec3 direction;                     // unit light direction vector
    float cutoff;                       // cos(cutoff angle)
    float exponent;                     // sharpness exponent for spotlight
};

/* UNIFORMS */
uniform vec2 screenSize;                // screen size of the window
uniform SpotLight light;                // the light being rendered
uniform sampler2D gbCoordBuf;           // world-space coordinates + specular exponent
uniform sampler2D gbDiffuseBuf;         // diffuse-color
uniform sampler2D gbSpecularBuf;        // specular-color
uniform sampler2D gbNormalBuf;          // world-space normals
uniform vec3 cameraPos;                 // the world-space camera position

/* OUTPUTS */
layout (location = 0) out vec4 fragColor;

void main ()
{
  // convert window coordinates of [0..1] range
    vec2 fragXY = gl_FragCoord.xy / screenSize;

  // get the world-space position + specular exponent
    vec4 posPlus = texture(gbCoordBuf, fragXY);
    vec3 pos = posPlus.xyz;

  // compute vector toward light
    vec3 toLight = light.position - pos;
    float distance = length(toLight);

  // compute light intensity based on angle and distance; if the light is very close
  // to the fragment, then we just use its full value
    float intensity = 1.0;
    if (distance > 0.001) {
      // normalize the toLight vector
        toLight /= distance;
        float cosTheta = -dot(light.direction, toLight);
        if (cosTheta >= light.cutoff) {
            float atten = (light.constant + distance * (light.linear + distance * light.quadratic));
            intensity = pow(cosTheta, light.exponent) / atten;
        }
        else {
            discard;
        }
    }
    else {
        toLight = normalize(toLight);
    }

  // if there is sufficient light intensity, then compute the fragment color
    if (intensity >= M_1_256) {  // >= 1/256
	vec3 lightC = intensity * light.intensity;
      // diffuse lighting
	vec3 norm = texture(gbNormalBuf, fragXY).xyz;
	vec3 diffuseC = texture(gbDiffuseBuf, fragXY).xyz;
	vec3 color = max (dot(toLight, norm), 0.0) * lightC * diffuseC;
      // specular lighting
      // specular intensity is (h dot n)^sharpness, where h is the unit vector
      // halfway between toCamera and toLight.
	vec3 toCamera = normalize(cameraPos - pos);
	float cosPhi = dot (normalize(toLight + toCamera), norm);
	if (cosPhi > M_1_256) {
	    vec3 specularC = texture(gbSpecularBuf, fragXY).xyz;
	    float specI = pow(cosPhi, posPlus.w) * intensity;
	    color += specI * texture(gbSpecularBuf, fragXY).rgb * light.intensity;
	}
	fragColor = vec4(clamp(color, 0, 1), 1.0);
    }
    else {
	discard;
    }

}
