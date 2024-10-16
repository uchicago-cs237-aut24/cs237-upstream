/*! \file renderer.cpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 1
 *
 * A Renderer bundles together the Vulkan renderpass and pipeline
 * objects for a particular shading mode.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "vertex.hpp"
#include "renderer.hpp"
#include "app.hpp"
#include <string_view>

/// path to directory that holds the shaders
constexpr std::string_view kShaderDir = CS237_BINARY_DIR "/projects/proj1/shaders/";

Renderer::Renderer (
    Proj1 *app,
    RenderMode mode,
    vk::RenderPass rp,
    vk::DescriptorSetLayout dsLayout)
  : _app(app), _renderPass(rp), _pipelineLayout(nullptr), _pipeline(nullptr)
{
  /** HINT: here you need to create the pipeline for the renderer based on its mode.
   ** We recommend that you define a table of properties (e.g., shader names etc.)
   ** that is indexed by the render mode.
   **/
}

Renderer::~Renderer ()
{
    this->_device().destroyPipeline(this->_pipeline);
    this->_device().destroyPipelineLayout(this->_pipelineLayout);
}

WireframeRenderer::~WireframeRenderer () { }
FlatRenderer::~FlatRenderer () { }
GouraudRenderer::~GouraudRenderer () { }
PhongRenderer::~PhongRenderer () { }
