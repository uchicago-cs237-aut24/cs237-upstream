/*! \file mesh.hpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 1
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _MESH_HPP_
#define _MESH_HPP_

#include "cs237/cs237.hpp"
#include "obj.hpp"
#include "vertex.hpp"

//! the information needed to render a mesh
struct Mesh {
    vk::Device device;                  //!< the Vulkan device
    cs237::VertexBuffer<Vertex> *vBuf;  //!< vertex-array for this mesh
    cs237::IndexBuffer<uint32_t> *iBuf; //!< the index array
    vk::PrimitiveTopology prim;         //!< the primitive type for rendering the mesh

    //! create a Mesh object by allocating and loading buffers for it.
    //! \param app  the owning app
    //! \param p    the topology of the vertices; for Project 1, it should
    //!             be vk::PrimitiveTopology::eTriangleList
    //! \param grp  a `Group` from an object that this mesh defines
    Mesh (cs237::Application *app, vk::PrimitiveTopology p, OBJ::Group const &grp);

    //! Mesh destuctor
    ~Mesh ();

    /// return the number of indices in the mesh
    uint32_t nIndices() const { return this->iBuf->nIndices(); }

    //! record commands in the command buffer to draw the mesh using
    //! `vkCmdDrawIndexed`.
    void draw (vk::CommandBuffer cmdBuf);

};

#endif // !_MESH_HPP_
