/*! \file app.hpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 4
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

/// The Project 4 Application class
class Proj4 : public cs237::Application {
public:
    Proj4 (std::vector<std::string> const &args);
    ~Proj4 ();

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

protected:
    Scene _scene;                       //!< the scene to be rendered
    vk::DescriptorPool _meshDSPool;     //!< the pool for allocating the per-mesh
                                        //!  descriptor sets

    /// the descriptor-set layout for the per-mesh sampler descriptor sets.
    /// We put this in the application object, so that it can be shared
    /// across meshes
    vk::DescriptorSetLayout _meshDSLayout;

};

#endif // !_APP_HPP_
