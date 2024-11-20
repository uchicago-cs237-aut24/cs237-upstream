/*! \file drawable.hpp
 *
 * CMSC 23740 Autumn 2024 Lab 6.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _DRAWABLE_HXX_
#define _DRAWABLE_HXX_

#include "cs237/cs237.hpp"
#include "mesh.hpp"
#include "uniforms.hpp"

/// the information that we need to draw stuff
struct Drawable {
    vk::Device device;                  ///< the owning device (needed for cleanup)
    cs237::VertexBuffer<Vertex> *vBuf;  ///< vertex buffer for mesh vertices
    cs237::IndexBuffer<uint16_t> *iBuf; ///< index buffer for mesh indices
    glm::mat4 modelMat;                 ///< drawable's model matrix
    glm::vec3 color;                    ///< drawable's color
    cs237::Texture2D *tex;              ///< drawable's texture
    vk::Sampler sampler;                ///< the texture sampler
    vk::DescriptorSet descSet;          ///< descriptor set for the texture sampler

    Drawable (cs237::Application *app, const Mesh *mesh);
    ~Drawable ();

    /// allocate the descriptor set for the drawable's sampler
    /// \param dsPool        the descriptor pool used for allocation
    /// \param dsLayout      the layout of the descriptor sets
    void initDescriptor (vk::DescriptorPool dsPool, vk::DescriptorSetLayout dsLayout);

    /// bind the sampler descriptor set for the drawable as Set 0
    void bindDescriptorSet (
        vk::CommandBuffer cmdBuf,
        vk::PipelineLayout pipeLayout);

    /// emit the push constants for this drawable into the command buffer
    void emitPushConstants (
        vk::CommandBuffer cmdBuf,
        vk::PipelineLayout pipeLayout,
        vk::ShaderStageFlags stages)
    {
        // NOTE: we are assuming that the model-to-world space transform is
        // a rigid-body transform, so that we can use it on normals too
        PC pc = { this->modelMat, this->modelMat };
        cmdBuf.pushConstants(
            pipeLayout,
            stages,
            0,
            sizeof(pc),
            &pc);
    }

    /// emit the drawing commands to render the object
    /// \param cmdBuf  the command buffer for the drawing commands
    void draw (vk::CommandBuffer cmdBuf)
    {
        // bind the vertex buffer
        vk::Buffer vertBuffers[] = {this->vBuf->vkBuffer()};
        vk::DeviceSize offsets[] = {0};
        cmdBuf.bindVertexBuffers(0, vertBuffers, offsets);

        // bind the index buffer
        cmdBuf.bindIndexBuffer(this->iBuf->vkBuffer(), 0, vk::IndexType::eUint16);

        cmdBuf.drawIndexed(this->iBuf->nIndices(), 1, 0, 0, 0);
    }

};

#endif /* _DRAWABLE_HXX_ */
