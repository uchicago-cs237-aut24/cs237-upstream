/*! \file instance.hpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 3
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _INSTANCE_HPP_
#define _INSTANCE_HPP_

#include "cs237/cs237.hpp"
#include "mesh.hpp"
#include <memory>

//! An instance of a graphical object in the scene
struct Instance {
    Mesh *mesh;                 //!< the mesh representing the object.  Note that
                                //!< mesh is freed by the Proj3Window class destructor.
    glm::vec3 color;            //!< the color of the object
    glm::mat4 toWorld;          //!< affine transform from object space to world space
    glm::mat3 normToWorld;      //!< linear transform that maps object-space normals
                                //!  to world-space normals
};

#endif /*! _INSTANCE_HPP_ */
