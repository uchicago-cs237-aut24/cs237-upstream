/*! \file render-modes.hpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 5
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _RENDER_MODES_HPP_
#define _RENDER_MODES_HPP_

enum class RenderMode : int {
    eWireFrame = 0,
    eTextured,
    eDeferred
};

constexpr int kWireFrame = static_cast<int>(RenderMode::eWireFrame);
constexpr int kTextured = static_cast<int>(RenderMode::eTextured);
constexpr int kDeferred = static_cast<int>(RenderMode::eDeferred);

/// A structure that collects together all of the rendering options in one place
struct RenderFlags {
    RenderMode mode;            //< the current rendering mode
    bool dirLight;              //< enable directional lighting for Deferred rendering
    bool spotLights;            //< enable spot lights for Deferred rendering
    bool emissiveLighting;      //< enable emissive lighting for Deferred rendering
    bool shadows;               //< enable shadows for Deferred rendering (extra credit)

/** HINT: you may want to add additional components to specify which buffer gets
 ** displayed or to enable wire-frame rendering of the light volumes.
 **/

    /// default constructor
    RenderFlags ()
    : mode(RenderMode::eTextured),
      dirLight(true), spotLights(true), emissiveLighting(true),
      shadows(false) // extra credit
    { }

    /// toggle the directional-lighting state
    void toggleDirLight () { this->dirLight = !this->dirLight; }

    /// toggle the spot-light state
    void toggleSpotLights () { this->spotLights = !this->spotLights; }

    /// toggle emissive lighting
    void toggleEmissiveLighting () { this->emissiveLighting = ! this->emissiveLighting; }

    /// toggle the shadow state (extra credit)
    void toggleShadows () { this->shadows = !this->shadows; }

};

#endif // !_RENDER_MODES_HPP_
