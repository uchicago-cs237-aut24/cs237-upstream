/*! \file mesh.cxx
 *
 * CS23740 Autumn 2024 Sample Code for Project 1
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "vertex.hpp"
#include "mesh.hpp"
#include <vector>

Mesh::Mesh (cs237::Application *app, vk::PrimitiveTopology p, OBJ::Group const &grp)
  : vBuf(nullptr), iBuf(nullptr), prim(p)
{
    if (grp.norms == nullptr) {
        ERROR("missing normals in model mesh");
    }

    /** HINT: allocate and initialize the vertex and index buffers here; you will
     ** have to convert from the SOA representation in the group to a AOS
     ** representation to initialize the vertex buffer.
     */

}

Mesh::~Mesh ()
{
    // delete Vulkan resources
    delete this->vBuf;
    delete this->iBuf;
}

void Mesh::draw (vk::CommandBuffer cmdBuf)
{
    /** HINT: record index-mode drawing commands here */
}
