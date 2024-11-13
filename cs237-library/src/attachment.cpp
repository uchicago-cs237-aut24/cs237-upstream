/*! \file attachment.cpp
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
#include "vulkan/vulkan_enums.hpp"

namespace cs237 {

Attachment::Attachment (
    cs237::Application *app,
    uint32_t wid,
    uint32_t ht,
    vk::Format fmt,
    vk::ImageUsageFlags usage)
: _app(app), _wid(wid), _ht(ht), _fmt(fmt)
{
    auto device = app->device();

    vk::ImageAspectFlagBits aspect = vk::ImageAspectFlagBits::eColor;
    vk::ImageLayout layout = vk::ImageLayout::eColorAttachmentOptimal;

    if (usage & vk::ImageUsageFlagBits::eDepthStencilAttachment) {
        // depth-buffer attachment
        layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        assert (false && "TODO: depth/stencil-buffer attachment support (aspect)");
    }

    vk::ImageCreateInfo imageInfo(
        {}, /* flags */
        vk::ImageType::e2D,
        fmt,
        { wid, ht, 1 }, /* extend: wid, ht, depth */
        1, /* mip levels */
        1, /* array layers */
        vk::SampleCountFlagBits::e1, /* samples */
        vk::ImageTiling::eOptimal,
        usage | vk::ImageUsageFlagBits::eSampled, /* usage should include "sampled" */
        vk::SharingMode::eExclusive, /* sharing mode */
        {},
        layout); /* layout */

    this->_img = device.createImage(imageInfo);

    this->_mem = app->_allocImageMemory(
        this->_img,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    this->_view = app->_createImageView(this->_img, this->_fmt, aspect);
}

Attachment::~Attachment ()
{
    auto device = this->_app->device();

    device.destroyImageView(this->_view);
    device.destroyImage(this->_img);
    device.freeMemory(this->_mem);
}

} // namespace cs237
