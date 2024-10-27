/*! \file renderer.cpp
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

#include "shader-uniforms.hpp"
#include "vertex.hpp"
#include "renderer.hpp"
#include "app.hpp"
#include "instance.hpp"
#include <string_view>

/// path to directory that holds the shaders
constexpr std::string_view kShaderDir = CS237_BINARY_DIR "/projects/proj2/shaders/";

Renderer::~Renderer ()
{
    this->_device().destroyPipeline(this->_pipeline);
    this->_device().destroyPipelineLayout(this->_pipelineLayout);
}

void Renderer::_initPipeline (
    RenderMode mode,
    std::vector<vk::DescriptorSetLayout> const &dsLayouts)
{
  /** HINT: here you need to create the pipeline for the renderer based on its mode.
   ** We recommend that you define a table of properties (e.g., shader names etc.)
   ** that is indexed by the render mode.
   **/
}


/******************** class WireframeRenderer ********************/

WireframeRenderer::~WireframeRenderer () { }

void WireframeRenderer::bindFrameDescriptorSets (
    vk::CommandBuffer cmdBuf,
    vk::DescriptorSet sceneDS)
{
    cmdBuf.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        this->_pipelineLayout,
        0, /* first set */
        sceneDS, /* descriptor sets */
        nullptr);

}

void WireframeRenderer::bindMeshDescriptorSets (vk::CommandBuffer cmdBuf, Instance *inst)
{
    /* no texturing in wireframe mode, so nothing to do */
}


/******************** class FlatRenderer ********************/

FlatRenderer::~FlatRenderer () { }

void FlatRenderer::bindFrameDescriptorSets (
    vk::CommandBuffer cmdBuf,
    vk::DescriptorSet sceneDS)
{
    cmdBuf.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        this->_pipelineLayout,
        0, /* first set */
        sceneDS, /* descriptor sets */
        nullptr);

}

void FlatRenderer::bindMeshDescriptorSets (vk::CommandBuffer cmdBuf, Instance *inst)
{
    /* no texturing in flat mode, so nothing to do */
}


/******************** class TextureRenderer ********************/

TextureRenderer::~TextureRenderer () { }

void TextureRenderer::bindFrameDescriptorSets (
    vk::CommandBuffer cmdBuf,
    vk::DescriptorSet sceneDS)
{
    cmdBuf.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        this->_pipelineLayout,
        0, /* first set */
        sceneDS, /* descriptor sets */
        nullptr);

}

void TextureRenderer::bindMeshDescriptorSets (vk::CommandBuffer cmdBuf, Instance *inst)
{
    /** HINT: bind the sampler descriptor sets */
}

/******************** class NormalMapRenderer ********************/

NormalMapRenderer::~NormalMapRenderer () { }

void NormalMapRenderer::bindFrameDescriptorSets (
    vk::CommandBuffer cmdBuf,
    vk::DescriptorSet sceneDS)
{
    cmdBuf.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        this->_pipelineLayout,
        0, /* first set */
        sceneDS, /* descriptor sets */
        nullptr);

}

void NormalMapRenderer::bindMeshDescriptorSets (vk::CommandBuffer cmdBuf, Instance *inst)
{
    /** HINT: bind the sampler descriptor sets */
}
