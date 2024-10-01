/*! \file texture.cpp
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

namespace __detail {

TextureBase::TextureBase (
    Application *app,
    uint32_t wid, uint32_t ht, uint32_t mipLvls,
    cs237::__detail::ImageBase const *img)
  : _app(app), _wid(wid), _ht(ht), _nMipLevels(mipLvls), _fmt(img->format())
{
    vk::ImageUsageFlags usage = (mipLvls > 1)
        ? vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
            | vk::ImageUsageFlagBits::eTransferSrc
        : vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    this->_img = app->_createImage (
        wid, ht, this->_fmt,
        vk::ImageTiling::eOptimal,
        usage,
        mipLvls);
    this->_mem = app->_allocImageMemory(
        this->_img,
        vk::MemoryPropertyFlagBits::eDeviceLocal);
    this->_view = app->_createImageView(
        this->_img, this->_fmt,
        vk::ImageAspectFlagBits::eColor);

}

TextureBase::~TextureBase ()
{
    this->_app->_device.destroyImageView(this->_view);
    this->_app->_device.destroyImage(this->_img);
    this->_app->_device.freeMemory(this->_mem);

}

void TextureBase::_init (cs237::__detail::ImageBase const *img)
{
    void *data = img->data();
    size_t nBytes = img->nBytes();
    auto device = this->_app->_device;

    // create a staging buffer for copying the image
    vk::Buffer stagingBuf = this->_createBuffer (
        nBytes, vk::BufferUsageFlagBits::eTransferSrc);
    vk::DeviceMemory stagingBufMem = this->_allocBufferMemory(
        stagingBuf,
        vk::MemoryPropertyFlagBits::eHostVisible
            | vk::MemoryPropertyFlagBits::eHostCoherent);

    // copy the image data to the staging buffer
    void* stagingData;
    stagingData = device.mapMemory(stagingBufMem, 0, nBytes, {});
    ::memcpy(stagingData, data, nBytes);
    device.unmapMemory(stagingBufMem);

    this->_app->_transitionImageLayout(
        this->_img, this->_fmt,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal);
    this->_app->_copyBufferToImage(this->_img, stagingBuf, nBytes, this->_wid, this->_ht);
    this->_app->_transitionImageLayout(
        this->_img, this->_fmt,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal);

    // free up the staging buffer
    device.freeMemory(stagingBufMem);
    device.destroyBuffer(stagingBuf);

}

} // namespce __detail

/******************** class Texture1D methods ********************/

Texture1D::Texture1D (Application *app, Image1D const *img)
  : __detail::TextureBase(app, img->width(), 1, 1, img)
{
    this->_init(img);
}

/******************** class Texture2D methods ********************/

// return the integer log2 of n; if n is not a power of 2, then return -1.
static int32_t ilog2 (uint32_t n)
{
    uint32_t k = 0, two_k = 1;
    while (two_k < n) {
        k++;
        two_k *= 2;
        if (two_k == n) {
            return k;
        }
    }
    return -1;

}

// compute the number of mipmap levels for an image.  This value is log2 of
// the larger dimension plus one for the base level image.  We require that
// both dimensions be a power of 2.
static uint32_t mipLevels (Image2D const *img, bool mipmap)
{
    if (mipmap) {
        int32_t log2Wid = ilog2(img->width());
        int32_t log2Ht = ilog2(img->height());
        if ((log2Wid < 0) || (log2Ht < 0)) {
            ERROR("texture size not a power of 2");
        }
        return std::max(log2Wid, log2Ht) + 1;
    }
    else {
        return 1;
    }
}

Texture2D::Texture2D (Application *app, Image2D const *img, bool mipmap)
  : __detail::TextureBase(app, img->width(), img->height(), mipLevels(img, mipmap), img)
{
    if (mipmap) {
        this->_generateMipMaps (img);
    } else {
        this->_init (img);
    }
}

// helper function for generating the mipmaps for a texture
void Texture2D::_generateMipMaps (Image2D const *img)
{
    // Check if image format supports linear blitting
    vk::FormatProperties props = this->_app->formatProps(this->_fmt);
    if (!(props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        ERROR("texture-image format does not support linear blitting!");
    }

/** FIXME: we should really use a single set of commands for both copying the data
 ** to the image buffer and for generating the mipmaps.
 **/
    void *data = img->data();
    size_t nBytes = img->nBytes();
    auto device = this->_app->_device;

    // create a staging buffer for copying the image
    vk::Buffer stagingBuf = this->_createBuffer (
        nBytes,
        vk::BufferUsageFlagBits::eTransferSrc);
    vk::DeviceMemory stagingBufMem = this->_allocBufferMemory(
        stagingBuf,
        vk::MemoryPropertyFlagBits::eHostVisible
            | vk::MemoryPropertyFlagBits::eHostCoherent);

    // copy the image data to the staging buffer
    void *stagingData = device.mapMemory(stagingBufMem, 0, nBytes, {});
    memcpy(stagingData, data, nBytes);
    device.unmapMemory(stagingBufMem);

    this->_app->_transitionImageLayout(
        this->_img, this->_fmt,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal);
    this->_app->_copyBufferToImage(this->_img, stagingBuf, nBytes, this->_wid, this->_ht);

    // free up the staging buffer
    device.freeMemory(stagingBufMem);
    device.destroyBuffer(stagingBuf);

    vk::CommandBuffer cmdBuf = this->_app->newCommandBuf();

    this->_app->beginCommands(cmdBuf, true);

    vk::ImageMemoryBarrier barrier(
        vk::AccessFlagBits::eTransferWrite, /* src access mask */
        vk::AccessFlagBits::eTransferRead, /* dst access mask */
        vk::ImageLayout::eTransferDstOptimal, /* old layout */
        vk::ImageLayout::eTransferSrcOptimal, /* new layout */
        VK_QUEUE_FAMILY_IGNORED, /* src queue family index */
        VK_QUEUE_FAMILY_IGNORED, /* dst queue family index */
        this->_img, /* image */
        vk::ImageSubresourceRange(
            vk::ImageAspectFlagBits::eColor, /* aspect mask */
            0, /* base mip level */
            1, /* level count */
            0, /* base array layer */
            1)); /* layer count */

    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer, /* src stage */
        vk::PipelineStageFlagBits::eTransfer, /* dst stage */
        {}, /* dependency flags */
        nullptr, /* memory barriers */
        nullptr, /* buffer-memory barriers */
        barrier); /* image barriers */

    int32_t mipWid = this->_wid;
    int32_t mipHt = this->_ht;

    // compute the mipmap levels; note that level 0 is the base image
    for (uint32_t i = 1; i < this->_nMipLevels; i++) {
        barrier
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setSrcAccessMask({})
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
        barrier.subresourceRange.setBaseMipLevel(i);

        cmdBuf.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer, /* src stage */
            vk::PipelineStageFlagBits::eTransfer, /* dst stage */
            {}, /* dependency flags */
            nullptr, /* memory barriers */
            nullptr, /* buffer-memory barriers */
            barrier); /* image barriers */

        int32_t nextWid = (mipWid > 1) ? (mipWid >> 1) : 1;
        int32_t nextHt = (mipHt > 1) ? (mipHt >> 1) : 1;

        vk::ImageBlit blit(
            vk::ImageSubresourceLayers( /* src subresource */
                vk::ImageAspectFlagBits::eColor, /* aspect mask */
                i - 1, /* mip level */
                0, /* base array level */
                1), /* layer count */
            { vk::Offset3D(0, 0, 0), vk::Offset3D(mipWid, mipHt, 1) },
            vk::ImageSubresourceLayers( /* dst subresource */
                vk::ImageAspectFlagBits::eColor, /* aspect mask */
                i, /* mip level */
                0, /* base array level */
                1), /* layer count */
            { vk::Offset3D(0, 0, 0), vk::Offset3D(nextWid, nextHt, 1) });

        cmdBuf.blitImage(
            this->_img,
            vk::ImageLayout::eTransferSrcOptimal,
            this->_img,
            vk::ImageLayout::eTransferDstOptimal,
            blit,
            vk::Filter::eLinear);

        barrier
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        cmdBuf.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer, /* src stage */
            vk::PipelineStageFlagBits::eFragmentShader, /* dst stage */
            {}, /* dependency flags */
            nullptr, /* memory barriers */
            nullptr, /* buffer-memory barriers */
            barrier); /* image barriers */

        mipWid = nextWid;
        mipHt = nextHt;
    }

    barrier.subresourceRange
        .setBaseMipLevel(0)
        .setLevelCount(this->_nMipLevels);
    barrier
        .setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
        .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead);

    cmdBuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer, /* src stage */
        vk::PipelineStageFlagBits::eFragmentShader, /* dst stage */
        {}, /* dependency flags */
        nullptr, /* memory barriers */
        nullptr, /* buffer-memory barriers */
        barrier); /* image barriers */

    this->_app->endCommands(cmdBuf);
    this->_app->submitCommands(cmdBuf);
    this->_app->freeCommandBuf(cmdBuf);

}

} // namespace cs237
