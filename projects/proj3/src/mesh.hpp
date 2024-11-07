/*! \file mesh.hpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 3
 *
 * \author John Reppy
 */

/* CMSC23700 Project 3 sample code (Autumn 2022)
 *
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _MESH_HPP_
#define _MESH_HPP_

#include "cs237/cs237.hpp"
#include "obj.hpp"
#include "app.hpp"
#include "vertex.hpp"

//! the information needed to render a mesh
struct Mesh {
    vk::Device device;                  //!< the Vulkan device
    cs237::VertexBuffer<Vertex> *vBuf;  //!< vertex-array for this mesh
    cs237::IndexBuffer<uint32_t> *iBuf; //!< the index array
    vk::PrimitiveTopology prim;         //!< the primitive type for rendering the mesh
    cs237::Texture2D *cMap;             //!< the color-map texture for the object
    cs237::Texture2D *nMap;             //!< the normal-map texture for the object
    vk::Sampler cMapSampler;            //!< the color-map sampler
    vk::Sampler nMapSampler;            //!< the normal-map sampler
    vk::DescriptorSet descSet;          //!< the descriptor set for the samplers

    //! create a Mesh object by allocating and loading buffers for it.
    //! \param app    the owning app
    //! \param p      the topology of the vertices; for Project 1, it should
    //!               be vk::PrimitiveTopology::eTriangleList
    //! \param model  a `Model` that this mesh represents; we assume that the model
    //!               has only one group
    Mesh (Proj3 *app, vk::PrimitiveTopology p, OBJ::Model const *model);

    //! Mesh destuctor
    ~Mesh ();

    /// return the number of indices in the mesh
    uint32_t nIndices() const { return this->iBuf->nIndices(); }

    //! record commands in the command buffer to draw the mesh using
    //! `vkCmdDrawIndexed`.
    void draw (vk::CommandBuffer cmdBuf);

};

#endif // !_MESH_HPP_
