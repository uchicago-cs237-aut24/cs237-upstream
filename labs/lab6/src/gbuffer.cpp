/*! \file gbuffer.cpp
 *
 * CMSC 23740 Autumn 2024 Lab 6.  This file is the main program
 * for Lab 5.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "gbuffer.hpp"
#include "uniforms.hpp"

/********** class GBuffer methods **********/

GBuffer::GBuffer (cs237::Application *app, uint32_t wid, uint32_t ht)
: _app(app), _wid(wid), _ht(ht)
{
    vk::Device dev = app->device();

    // create the attachments
    this->_albedo = new cs237::Attachment (
        app, wid, ht,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);
    this->_normal = new cs237::Attachment (
        app, wid, ht,
        vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eColorAttachment);

    // create the sampler for reading the attachments.  We can use the same CPU-side
    // value for all of the attachments, since they all have the same settings
    vk::SamplerCreateInfo samplerInfo(
        {}, /* flags */
        vk::Filter::eLinear,                    // magnification filter
        vk::Filter::eLinear,                    // minification filter
        vk::SamplerMipmapMode::eLinear,         // mipmap mode
        vk::SamplerAddressMode::eClampToEdge,   // addressing mode for U coordinates
        vk::SamplerAddressMode::eClampToEdge,   // addressing mode for V coordinates
        vk::SamplerAddressMode::eClampToEdge,   // addressing mode for W coordinates
        0.0, /* mip LOD bias */
        VK_FALSE, /* anisotropy enable */
        1.0,
        VK_FALSE, /* compare enable */
        vk::CompareOp::eNever, /* compare op */
        0.0f, /* min LOD */
        1.0f, /* max LOD */
        vk::BorderColor::eFloatOpaqueWhite, /* borderColor */
        VK_FALSE); /* unnormalized coordinates */

    this->_sampler = dev.createSampler(samplerInfo);

}

GBuffer::~GBuffer ()
{
    vk::Device dev = this->_app->device();

    delete this->_albedo;
    delete this->_normal;

    dev.destroySampler(this->_sampler);
    dev.destroyDescriptorSetLayout(this->_dsLayout);
    dev.destroyDescriptorPool(this->_descPool);
}

void GBuffer::initAttachments (
    std::vector<vk::AttachmentDescription> &descs,
    std::vector<vk::AttachmentReference> &refs)
{
    // the G-buffer has two attachments
    descs.resize(2);
    refs.resize(2);

    // the output albedo (color) buffer
    descs[0].format = this->_albedo->format();
    descs[0].samples = vk::SampleCountFlagBits::e1;
    descs[0].loadOp = vk::AttachmentLoadOp::eClear;
    descs[0].storeOp = vk::AttachmentStoreOp::eStore;
    descs[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    descs[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    descs[0].initialLayout = vk::ImageLayout::eUndefined;
    descs[0].finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    refs[0].attachment = 0;
    refs[0].layout = vk::ImageLayout::eColorAttachmentOptimal;

    // the output normal buffer
    descs[1].format = this->_normal->format();
    descs[1].samples = vk::SampleCountFlagBits::e1;
    descs[1].loadOp = vk::AttachmentLoadOp::eClear;
    descs[1].storeOp = vk::AttachmentStoreOp::eStore;
    descs[1].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    descs[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    descs[1].initialLayout = vk::ImageLayout::eUndefined;
    descs[1].finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    refs[1].attachment = 1;
    refs[1].layout = vk::ImageLayout::eColorAttachmentOptimal;
}

void GBuffer::initDescriptorSetLayout ()
{
    // initialize the descriptor-set pool for the G-buffer samplers
    vk::DescriptorPoolSize poolSize = vk::DescriptorPoolSize(
            vk::DescriptorType::eCombinedImageSampler,
            GBuffer::kNumBuffers);
    vk::DescriptorPoolCreateInfo poolInfo(
        {}, /* flags */
        1, /* max sets */
        poolSize); /* pool sizes */
    this->_descPool = this->_app->device().createDescriptorPool(poolInfo);

    // create the descriptor-set layout for the G-buffer samplers
    vk::DescriptorSetLayoutBinding albedoBinding(
        0, /* binding */
        vk::DescriptorType::eCombinedImageSampler, /* descriptor type */
        1, /* descriptor count */
        vk::ShaderStageFlagBits::eFragment, /* stages */
        nullptr); /* immutable samplers */
    vk::DescriptorSetLayoutBinding normalBinding(
        1, /* binding */
        vk::DescriptorType::eCombinedImageSampler, /* descriptor type */
        1, /* descriptor count */
        vk::ShaderStageFlagBits::eFragment, /* stages */
        nullptr); /* immutable samplers */
    std::array<vk::DescriptorSetLayoutBinding, 2> layoutBindings = {
            albedoBinding, normalBinding
        };
    vk::DescriptorSetLayoutCreateInfo layoutInfo({}, layoutBindings);
    this->_dsLayout = this->_app->device().createDescriptorSetLayout(layoutInfo);

}

void GBuffer::initDescriptorSet ()
{
    // create the descriptor set for the samplers
    vk::DescriptorSetAllocateInfo allocInfo(
        this->_descPool,
        this->_dsLayout);
    this->_descSet = (this->_app->device().allocateDescriptorSets(allocInfo))[0];

    std::array<vk::WriteDescriptorSet,kNumBuffers> descWrites;

    // write the albedo descriptor set
    auto albedoImageInfo = vk::DescriptorImageInfo(
        this->_sampler,
        this->_albedo->imageView(),
        vk::ImageLayout::eShaderReadOnlyOptimal);
    descWrites[0] = vk::WriteDescriptorSet(
        this->_descSet, /* descriptor set */
        kGBufAlbedoBind, /* binding */
        0, /* array element */
        vk::DescriptorType::eCombinedImageSampler, /* descriptor type */
        albedoImageInfo, /* image info */
        nullptr, /* buffer info */
        nullptr); /* texel buffer view */

    // write the normal descriptor set
    auto normalImageInfo = vk::DescriptorImageInfo(
        this->_sampler,
        this->_normal->imageView(),
        vk::ImageLayout::eShaderReadOnlyOptimal);
    descWrites[1] = vk::WriteDescriptorSet(
        this->_descSet, /* descriptor set */
        kGBufNormalBind, /* binding */
        0, /* array element */
        vk::DescriptorType::eCombinedImageSampler, /* descriptor type */
        normalImageInfo, /* image info */
        nullptr, /* buffer info */
        nullptr); /* texel buffer view */

    this->_app->device().updateDescriptorSets(descWrites, nullptr);

}

void GBuffer::resize (uint32_t wid, uint32_t ht)
{
    if (this->_wid == wid && this->_ht == ht) {
        return; // no size change
    }

    // delete the existing attachments and then create new ones
    delete this->_albedo;
    delete this->_normal;

    this->_albedo = new cs237::Attachment (
        this->_app, wid, ht,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eColorAttachment);
    this->_normal = new cs237::Attachment (
        this->_app, wid, ht,
        vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eColorAttachment);
}

std::vector<vk::ClearValue> GBuffer::clearValues () const
{
    std::vector<vk::ClearValue> clearV = {
            /* clear color for the albedo buffer; we use 0 for the alpha
             * channel so that we can detect pixels that have not been
             * rendered during the geometry pass.
             */
            vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f),
            /* clear color for the normal buffer */
            vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f)
        };

    return clearV;

}
