/*! \file renderer.hpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 1
 *
 * A Renderer bundles together the Vulkan renderpass and pipeline
 * objects for a particular shading mode.  We specialize the `Renderer`
 * base class for each of the rendering modes.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */


#ifndef _RENDERER_HPP_
#define _RENDERER_HPP_

#include "cs237/cs237.hpp"
#include "app.hpp"
#include "render-modes.hpp"
#include "shader-uniforms.hpp"

//! An abstract container for the information needed to support a rendering
//! mode.  It is specialized to specific rendering modes by subclasses.
class Renderer {
public:

    virtual ~Renderer ();

    /// issue a command to bind this renderer's pipeline
    void bindPipelineCmd (vk::CommandBuffer cmdBuf)
    {
        cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, this->_pipeline);
    }

    void bindDescriptorSets (vk::CommandBuffer cmdBuf,
        vk::ArrayProxy<const vk::DescriptorSet> const &descriptorSets,
        vk::ArrayProxy<const uint32_t> const &dynamicOffsets)
    {
        cmdBuf.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            this->_pipelineLayout,
            0, /* first set */
            descriptorSets, /* descriptor sets */
            dynamicOffsets); /* dynamic offsets */
    }

    void pushConstants (vk::CommandBuffer cmdBuf, PushConsts const &pc)
    {
        cmdBuf.pushConstants(
            this->_pipelineLayout,
            vk::ShaderStageFlagBits::eVertex, /* just used in vertex shader */
            0,
            sizeof(PushConsts),
            &pc);
    }

protected:
    Proj1 *_app;                        ///< the owning application
    vk::RenderPass _renderPass;         ///< render pass, which is shared across all renderers
    vk::PipelineLayout _pipelineLayout; ///< the pipeline layout for this renderer's
                                        ///  graphics pipeline
    vk::Pipeline _pipeline;             ///< the graphics pipeline for this renderer

    Renderer (
        Proj1 *app,
        RenderMode renderMode,
        vk::RenderPass rp,
        vk::DescriptorSetLayout dsLayout);

    /// get the device handle
    vk::Device _device() const { return this->_app->device(); }

};

/// A Renderer for drawing the scene in wireframe mode
class WireframeRenderer : public Renderer {
public:
    WireframeRenderer (Proj1 *app, vk::RenderPass rp, vk::DescriptorSetLayout dsLayout)
      : Renderer (
            app,
            RenderMode::eWireframe,
            rp,
            dsLayout)
    { }

    virtual ~WireframeRenderer () override;
};

/// A Renderer for drawing the scene in flat-shading mode
class FlatRenderer : public Renderer {
public:
    FlatRenderer (Proj1 *app, vk::RenderPass rp, vk::DescriptorSetLayout dsLayout)
      : Renderer (
            app,
            RenderMode::eFlatShading,
            rp,
            dsLayout)
    { }

    virtual ~FlatRenderer () override;
};

/// A Renderer for drawing the scene in Gouraud-shading (per-vertex lighting) mode
class GouraudRenderer : public Renderer {
public:
    GouraudRenderer (Proj1 *app, vk::RenderPass rp, vk::DescriptorSetLayout dsLayout)
      : Renderer (
            app,
            RenderMode::eGouraudShading,
            rp,
            dsLayout)
    { }

    virtual ~GouraudRenderer () override;
};

/// A Renderer for drawing the scene in Phong-shading (per-pixel lighting) mode
class PhongRenderer : public Renderer {
public:
    PhongRenderer (Proj1 *app, vk::RenderPass rp, vk::DescriptorSetLayout dsLayout)
      : Renderer (
            app,
            RenderMode::ePhongShading,
            rp,
            dsLayout)
    { }

    virtual ~PhongRenderer () override;
};

#endif // !_RENDERER_HPP_
