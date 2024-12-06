/*! \file app.hpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 5
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _APP_HPP_
#define _APP_HPP_

#include "cs237/cs237.hpp"
#include "scene.hpp"

/// The Project 5 Application class
class Proj5 : public cs237::Application {
public:
    Proj5 (std::vector<std::string> const &args);
    ~Proj5 ();

    /// run the application
    void run () override;

    /// access function for the scene
    const Scene *scene () const { return &this->_scene; }

    /// a descriptor-set layout for the per-mesh samplers
    vk::DescriptorSetLayout meshDSLayout () const { return this->_meshDSLayout; }

    /// allocate a descriptor set for a mesh.  This method is used to allocate the
    /// per-frame descriptor sets
    vk::DescriptorSet allocMeshDS ();

    /// print the interface help message to standard out
    void controlsHelpMessage ();

    /// toggle the state of the rain simulation
    void toggleRain ()
    {
        if (this->_enableRain) {
            this->_enableRain = false;
        } else {
            this->_enableRain = true;
            this->_lastT = glfwGetTime();
        }
    }

    /// is rain enabled?
    bool isRainEnabled () const { return this->_enableRain; }

    /// update the internal time of the simulation and return the delta since the
    /// last update
    double rainTime ()
    {
        if (this->_enableRain) {
            double now = glfwGetTime();
            double deltaT = now - this->_lastT;
            this->_lastT = now;
            return deltaT;
        } else {
            return 0.0;
        }
    }

protected:
    Scene _scene;                       //!< the scene to be rendered
    vk::DescriptorPool _meshDSPool;     //!< the pool for allocating the per-mesh
                                        //!  descriptor sets

    /// the descriptor-set layout for the per-mesh sampler descriptor sets.
    /// We put this in the application object, so that it can be shared
    /// across meshes
    vk::DescriptorSetLayout _meshDSLayout;

    /// rain simulation stuff
    bool _enableRain;                   ///< when true, simulate and render the
                                        ///  rain particles
    double _lastT;                      ///< time of last simulation step

};

#endif // !_APP_HPP_
