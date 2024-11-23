/*! \file spot-light.cxx
 *
 * \author John Reppy
 */

/* CMSC23700 Project 4 sample code (Autumn 2017)
 *
 * COPYRIGHT (c) 2017 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "spot-light.hxx"

//! initialize the additional fields of the spot light from scene
void SpotLight::Init ()
{
    float a = this->atten[2];   // quadratic
    float b = this->atten[1];   // linear
    float c = this->atten[0];   // constant

    float scaledI = 255.0f * cs237::max(cs237::max(this->intensity.r, this->intensity.g), this->intensity.b);
    if (a < 0.00001f) {
      // linear attenuation
        this->maxDist = (1.0f - scaledI*c) / (b * scaledI);
    }
    else {
      // quadratic attenuation
        float desc2 = b*b - 4.0 * a * (c - scaledI);
        this->maxDist = (sqrt(desc2) - b) / (2.0 * a);
    }

std::cerr << "Light " << this->name << " range = " << this->maxDist << std::endl;

    this->cosCutoff = cosf(cs237::radians(this->cutoff));
    this->cone = MakeCone (this->pos, this->dir, cutoff, maxDist);
}
