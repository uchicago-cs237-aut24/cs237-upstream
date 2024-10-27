/*! \file renderer.hpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 2
 *
 * A Renderer bundles together the Vulkan renderpass and pipeline
 * objects for a particular shading mode.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */


#ifndef _RENDERER_HPP_
#define _RENDERER_HPP_

#include "cs237/cs237.hpp"
#include "app.hpp"
#include "render-modes.hpp"
#include "shader-uniforms.hpp"

struct Instance;

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

    /// bind the descriptor sets for rendering a frame
    /// \param cmdBuf   the command buffer to store the bind command in
    /// \param sceneDS  the descriptor set for the scene UBO for
    ///                 the frame being rendered
    virtual void bindFrameDescriptorSets (
        vk::CommandBuffer cmdBuf,
        vk::DescriptorSet sceneDS) = 0;

    /// bind the descriptor sets for rendering a given object
    /// \param cmdBuf   the command buffer to store the bind command in
    /// \param vertUBO  the vertex-shader uniform-buffer-object information for
    ///                 the frame being rendered
    /// \param inst     the instance to be rendered using the UBO
    virtual void bindMeshDescriptorSets (
        vk::CommandBuffer cmdBuf,
        Instance *inst) = 0;

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
    Proj2 *_app;                        ///< the owning application
    vk::RenderPass _renderPass;         ///< render pass, which is shared across all
                                        ///  renderers
    vk::PipelineLayout _pipelineLayout; ///< the pipeline layout for this renderer's
                                        ///  graphics pipeline
    vk::Pipeline _pipeline;             ///< the graphics pipeline for this renderer

    Renderer (Proj2 *app, vk::RenderPass rp)
      : _app(app), _renderPass(rp), _pipelineLayout(nullptr), _pipeline(nullptr)
    { }

    /// initialize the renderer's pipeline
    void _initPipeline (
        RenderMode mode,
        std::vector<vk::DescriptorSetLayout> const &dsLayouts);

    /// get the device handle
    vk::Device _device() const { return this->_app->device(); }

};

/// A Renderer for drawing the scene in wireframe mode
class WireframeRenderer : public Renderer {
public:
    /// constructor
    /// \param app      pointer to the application object
    /// \param rp       pointer to the renderpass
    /// \param sceneDS  descriptor-set layout for the scene UBO (set 0)
    WireframeRenderer (Proj2 *app, vk::RenderPass rp, vk::DescriptorSetLayout sceneDS)
      : Renderer (app, rp)
    {
        std::vector<vk::DescriptorSetLayout> layouts = {sceneDS};
        this->_initPipeline(RenderMode::eWireframe, layouts);
    }

    ~WireframeRenderer () override;

    /// bind the descriptor sets for rendering a frame
    /// \param cmdBuf   the command buffer to store the bind command in
    /// \param sceneDS  the descriptor set for the scene UBO for
    ///                 the frame being rendered
    void bindFrameDescriptorSets (
        vk::CommandBuffer cmdBuf,
        vk::DescriptorSet sceneDS) override;

    /// bind the descriptor sets for rendering a given object
    /// \param cmdBuf   the command buffer to store the bind command in
    /// \param vertUBO  the vertex-shader uniform-buffer-object information for
    ///                 the frame being rendered
    /// \param inst     the instance to be rendered using the UBO
    void bindMeshDescriptorSets (
        vk::CommandBuffer cmdBuf,
        Instance *inst) override;

};

/// A Renderer for drawing the scene in flat-shading mode
class FlatRenderer : public Renderer {
public:
    /// constructor
    /// \param app       pointer to the application object
    /// \param rp        pointer to the renderpass
    /// \param sceneDS  descriptor-set layout for the scene UBO (set 0)
    FlatRenderer (Proj2 *app, vk::RenderPass rp, vk::DescriptorSetLayout sceneDS)
      : Renderer (app, rp)
    {
        std::vector<vk::DescriptorSetLayout> layouts = {sceneDS};
        this->_initPipeline(RenderMode::eFlatShading, layouts);
    }

    ~FlatRenderer () override;

    /// bind the descriptor sets for rendering a frame
    /// \param cmdBuf   the command buffer to store the bind command in
    /// \param sceneDS  the descriptor set for the scene UBO for
    ///                 the frame being rendered
    void bindFrameDescriptorSets (
        vk::CommandBuffer cmdBuf,
        vk::DescriptorSet sceneDS) override;

    /// bind the descriptor sets for rendering a given object
    /// \param cmdBuf   the command buffer to store the bind command in
    /// \param vertUBO  the vertex-shader uniform-buffer-object information for
    ///                 the frame being rendered
    /// \param inst     the instance to be rendered using the UBO
    void bindMeshDescriptorSets (
        vk::CommandBuffer cmdBuf,
        Instance *inst) override;

};

/// A Renderer for drawing the scene using per-pixel texturing
class TextureRenderer : public Renderer {
public:
    /// constructor
    /// \param app        pointer to the application object
    /// \param rp         pointer to the renderpass
    /// \param sceneDS    descriptor-set layout for the scene UBO (set 0)
    /// \param samplerDS  descriptor-set layout for the texture samplers (set 1)
    TextureRenderer (
        Proj2 *app,
        vk::RenderPass rp,
        vk::DescriptorSetLayout sceneDS,
        vk::DescriptorSetLayout samplerDS)
      : Renderer (app, rp)
    {
        std::vector<vk::DescriptorSetLayout> layouts = {
                sceneDS, samplerDS, app->meshDSLayout()
            };
        this->_initPipeline(RenderMode::eTextureShading, layouts);

    }

    ~TextureRenderer () override;


    /// bind the descriptor sets for rendering a frame
    /// \param cmdBuf   the command buffer to store the bind command in
    /// \param sceneDS  the descriptor set for the scene UBO for
    ///                 the frame being rendered
    void bindFrameDescriptorSets (
        vk::CommandBuffer cmdBuf,
        vk::DescriptorSet sceneDS) override;

    /// bind the descriptor sets for rendering a given object
    /// \param cmdBuf   the command buffer to store the bind command in
    /// \param vertUBO  the vertex-shader uniform-buffer-object information for
    ///                 the frame being rendered
    /// \param inst     the instance to be rendered using the UBO
    void bindMeshDescriptorSets (
        vk::CommandBuffer cmdBuf,
        Instance *inst) override;

};

/// A Renderer for drawing the scene using per-pixel texturing and a normal map
class NormalMapRenderer : public Renderer {
public:
    /// constructor
    /// \param app        pointer to the application object
    /// \param rp         pointer to the renderpass
    /// \param sceneDS    descriptor-set layout for the scene UBO (set 0)
    /// \param samplerDS  descriptor-set layout for the texture samplers (set 1)
    NormalMapRenderer (
        Proj2 *app,
        vk::RenderPass rp,
        vk::DescriptorSetLayout sceneDS,
        vk::DescriptorSetLayout samplerDS)
      : Renderer (app, rp)
    {
        std::vector<vk::DescriptorSetLayout> layouts = {
                sceneDS, samplerDS, app->meshDSLayout()
            };
        this->_initPipeline(RenderMode::eNormalMapShading, layouts);
    }

    ~NormalMapRenderer () override;


    /// bind the descriptor sets for rendering a frame
    /// \param cmdBuf   the command buffer to store the bind command in
    /// \param sceneDS  the descriptor set for the scene UBO for
    ///                 the frame being rendered
    void bindFrameDescriptorSets (
        vk::CommandBuffer cmdBuf,
        vk::DescriptorSet sceneDS) override;

    /// bind the descriptor sets for rendering a given object
    /// \param cmdBuf   the command buffer to store the bind command in
    /// \param vertUBO  the vertex-shader uniform-buffer-object information for
    ///                 the frame being rendered
    /// \param inst     the instance to be rendered using the UBO
    void bindMeshDescriptorSets (
        vk::CommandBuffer cmdBuf,
        Instance *inst) override;

};

#endif // !_RENDERER_HPP_
