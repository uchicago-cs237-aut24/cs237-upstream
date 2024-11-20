/*! \file depth-buffer.cpp
 *
 * Support code for CMSC 23740 Autumn 2024.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "cs237/cs237.hpp"

namespace cs237 {

DepthBuffer::DepthBuffer (Application *app, uint32_t wid, uint32_t ht)
: _app(app), _wid(wid), _ht(ht)
{
    // create the sampler
    cs237::Application::SamplerInfo samplerInfo(
        vk::Filter::eLinear,                    // magnification filter
        vk::Filter::eLinear,                    // minification filter
        vk::SamplerMipmapMode::eLinear,         // mipmap mode
        vk::SamplerAddressMode::eClampToEdge,   // addressing mode for U coordinates
        vk::SamplerAddressMode::eClampToEdge,   // addressing mode for V coordinates
        vk::BorderColor::eFloatOpaqueWhite);    // border color (white is max depth)
    this->_sampler = this->_app->createDepthSampler (samplerInfo);

    this->_fmt = app->_depthStencilBufferFormat(true, false);

    // create the image
    this->_image = app->_createImage (
        wid, ht,
        this->_fmt,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment
            | vk::ImageUsageFlagBits::eSampled);

    // allocate and bind the memory object
    this->_mem = app->_allocImageMemory (
        this->_image,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    // create the image view
    this->_imageView = app->_createImageView(
        this->_image,
        this->_fmt,
        vk::ImageAspectFlagBits::eDepth);

}

DepthBuffer::~DepthBuffer ()
{
    this->_app->device().destroyImageView (this->_imageView);
    this->_app->device().freeMemory (this->_mem);
    this->_app->device().destroyImage (this->_image);
    this->_app->device().destroySampler (this->_sampler);
}

vk::Framebuffer DepthBuffer::createFramebuffer (vk::RenderPass rp)
{
    vk::FramebufferCreateInfo fbInfo(
        {}, /* flags */
        rp, /* render pass */
        this->_imageView, /* attachments */
        this->_wid,
        this->_ht,
        1); /* layers */

    return this->_app->device().createFramebuffer(fbInfo);

}

} // namespace cs237
