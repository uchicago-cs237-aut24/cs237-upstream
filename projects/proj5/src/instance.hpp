/*! \file instance.hpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 5
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _INSTANCE_HPP_
#define _INSTANCE_HPP_

#include "mesh.hpp"
#include "shader-uniforms.hpp"

//! An instance of a graphical object in the scene
struct Instance {
    std::vector<Mesh *> meshes; //!< the meshs representing the object.  Note that
                                //!< mesh is freed by the Proj5Window class destructor.
    glm::mat4 toWorld;          //!< affine transform from object space to world space
    glm::mat3 normToWorld;      //!< linear transform that maps object-space normals
                                //!  to world-space normals

    // constructor
    Instance (glm::mat4 modelM, glm::mat3 normM)
    : meshes(), toWorld(modelM), normToWorld(normM)
    { }

    // constructor
    Instance (Mesh *m, glm::mat4 modelM, glm::mat3 normM)
    : meshes{m}, toWorld(modelM), normToWorld(normM)
    { }

    // add a mesh to the instance
    void pushMesh (Mesh *m) { this->meshes.push_back(m); }

};

#endif /*! _INSTANCE_HPP_ */
