/*! \file drawable.cpp
 *
 * CMSC 23740 Autumn 2024 Lab 6.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "drawable.hpp"
#include "uniforms.hpp"

Drawable::Drawable (cs237::Application *app, const Mesh *mesh)
: device(app->device()),
    vBuf(new cs237::VertexBuffer<Vertex>(app, mesh->verts)),
    iBuf(new cs237::IndexBuffer<uint16_t>(app, mesh->indices)),
    modelMat(mesh->toWorld),
    color(mesh->color)
{
    // for this lab, all the meshes should have textures
    assert (mesh->hasTexture());

    // initialize the texture
    this->tex = new cs237::Texture2D(app, mesh->image);
    // create the texture sampler
    cs237::Application::SamplerInfo samplerInfo(
        vk::Filter::eLinear,                    // magnification filter
        vk::Filter::eLinear,                    // minification filter
        vk::SamplerMipmapMode::eLinear,         // mipmap mode
        vk::SamplerAddressMode::eClampToEdge,   // addressing mode for U coordinates
        vk::SamplerAddressMode::eClampToEdge,   // addressing mode for V coordinates
        vk::BorderColor::eIntOpaqueBlack);      // border color
    this->sampler = app->createSampler (samplerInfo);

}

Drawable::~Drawable ()
{
    delete this->iBuf;
    delete this->vBuf;
    vkDestroySampler(this->device, this->sampler, nullptr);
    delete this->tex;
}

void Drawable::initDescriptor (
    vk::DescriptorPool dsPool,
    vk::DescriptorSetLayout dsLayout)
{
    // create the descriptor set
    vk::DescriptorSetAllocateInfo allocInfo(dsPool, dsLayout);
    this->descSet = (this->device.allocateDescriptorSets(allocInfo))[0];

    // initialize the color map descriptor
    vk::DescriptorImageInfo imageInfo(
        this->sampler,
        this->tex->view(),
        vk::ImageLayout::eShaderReadOnlyOptimal);
    vk::WriteDescriptorSet writeDS(
        this->descSet, /* descriptor set */
        kColorSamplerBind, /* binding */
        0, /* array element */
        vk::DescriptorType::eCombinedImageSampler, /* descriptor type */
        imageInfo, /* image info */
        nullptr, /* buffer info */
        nullptr); /* texel buffer view */

    device.updateDescriptorSets(writeDS, nullptr);

}

void Drawable::bindDescriptorSet (
    vk::CommandBuffer cmdBuf,
    vk::PipelineLayout pipeLayout)
{
    cmdBuf.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        pipeLayout,
        kColorSamplerSet,
        this->descSet,
        nullptr);
}
