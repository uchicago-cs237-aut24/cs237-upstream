/*! \file spot-light.hxx
 *
 * \author John Reppy
 */

/* CMSC23700 Project 4 sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _SPOT_LIGHT_HXX_
#define _SPOT_LIGHT_HXX_

#include  "cs237.hxx"
#include "cone-mesh.hxx"

struct SpotLight {
    std::string         name;           //!< the light's name
    cs237::vec3f        pos;            //!< the light's world-space position
    cs237::vec3f        dir;            //!< unit-length direction vector
    float               cutoff;         //!< the cutoff angle for the light (in degrees)
    float               exponent;       //!< the focus exponent
    cs237::color3f      intensity;      //!< the light's intensity
    float               atten[3];       //!< attenuation coefficients (constant, linear, quadratic)
  /* additional fields that are useful for rendering the light volumes */
    float               maxDist;        //!< the maximum distance at which the light has non-zero
                                        //!< intensity
    float               cosCutoff;      //!< cos(cutoff)
    Instance            *cone;          //!< mesh for the light volume

  //! initialize the additional fields of the spot light from scene (e.g., maxDist, etc.)
    void Init ();

};

#endif // !_SPOT_LIGHT_HXX_
