/*! \file render-info.hpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 3
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _RENDER_INFO_HPP_
#define _RENDER_INFO_HPP_

#include "cs237/cs237.hpp"
#include "shader-uniforms.hpp"

/// struct to collect the information needed for a render pass
struct RenderInfo {
    vk::RenderPass renderPass;          ///< render pass for depth texture
    vk::PipelineLayout pipelineLayout;  ///< pipeline layout for depth pass
    vk::Pipeline pipeline;              ///< pipeline for depth pass

    virtual void destroy (vk::Device device)
    {
        device.destroyPipeline(this->pipeline);
        device.destroyRenderPass(this->renderPass);
        device.destroyPipelineLayout(this->pipelineLayout);
    }

    /// issue a command to bind this renderer's pipeline
    void bindPipelineCmd (vk::CommandBuffer cmdBuf)
    {
        cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, this->pipeline);
    }

    // issue a command to add push constants to the command buffer
    void pushConstants (vk::CommandBuffer cmdBuf, PushConsts const &pc)
    {
        cmdBuf.pushConstants(
            this->pipelineLayout,
            vk::ShaderStageFlagBits::eVertex, /* just used in vertex shader */
            0,
            sizeof(PushConsts),
            &pc);
    }

};

/// render-pass information with additional depth-pass information
struct DepthRenderInfo : public RenderInfo {
    cs237::DepthBuffer *depthBuf;       ///< depth-buffer
    vk::Framebuffer frameBuffer;        ///< output buffer for depth pass

    virtual void destroy (vk::Device device) override
    {
        device.destroyFramebuffer(this->frameBuffer);
        delete this->depthBuf;
        device.destroyPipeline(this->pipeline);
        device.destroyRenderPass(this->renderPass);
        device.destroyPipelineLayout(this->pipelineLayout);
    }
};

#endif /* !_RENDER_INFO_HPP_ */
