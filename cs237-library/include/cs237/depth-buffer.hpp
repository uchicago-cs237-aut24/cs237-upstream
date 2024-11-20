/*! \file depth-buffer.hpp
 *
 * Support code for CMSC 23740 Autumn 2024.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_DEPTH_BUFFER_HPP_
#define _CS237_DEPTH_BUFFER_HPP_

#ifndef _CS237_HPP_
#error "cs237/depth-buffer.hpp should not be included directly"
#endif

namespace cs237 {

/* TODO: perhaps this class should be replaced in favor of DepthAttachment? */

/// This class is a wrapper around the resources needed to implement a depth buffer
/// that can sampled
class DepthBuffer {
public:

    /// \brief construct and initialize a new depth buffer of the give size
    /// \param app  the owning application
    /// \param wid  the width of the buffer (should be power of 2)
    /// \param ht   the height of the buffer (should be power of 2)
    ///
    /// This function allocates and initializes a frame-buffer object
    /// and associated sampler.  The sampler is initialized with the
    /// following parameters:
    ///   - the filter parameters are set to `VK_FILTER_LINEAR`
    ///   - the mimap mode is `VK_SAMPLER_MIPMAP_MODE_NEAREST`
    ///   - the address modes are set to `VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE`
    //
    DepthBuffer (Application *app, uint32_t wid, uint32_t ht);

    /// destructor; this will free the underlying resources
    ~DepthBuffer ();

    /// get the width of the buffer
    uint32_t width() const { return this->_wid; }

    /// get the height of the buffer
    uint32_t height() const { return this->_ht; }

    /// get the depth-buffer image format
    vk::Format format() const { return this->_fmt; }

    /// get the sampler
    vk::Sampler sampler() const { return this->_sampler; }

    /// get the image view for the depth buffer
    vk::ImageView imageView() const { return this->_imageView; }

    /// get the image information for the depth buffer
    vk::DescriptorImageInfo imageInfo() const
    {
        return vk::DescriptorImageInfo(
            this->_sampler,
            this->_imageView,
            vk::ImageLayout::eDepthStencilReadOnlyOptimal);
    }

    /// create a framebuffer that writes to the depth-buffer image.
    /// \param rp  the render pass used to render to the framebuffer
    /// \return the framebuffer for the depth buffer
    vk::Framebuffer createFramebuffer (vk::RenderPass rp);

private:
    uint32_t _wid;
    uint32_t _ht;
    Application *_app;          ///< the application
    vk::Format _fmt;            ///< the format of the depth buffer
    vk::Image _image;
    vk::ImageView _imageView;
    vk::DeviceMemory _mem;      ///< the device memory object
    vk::Sampler _sampler;       ///< sampler for reading from the image

};

} // namespace cs237

#endif // !_CS237_DEPTH_BUFFER_HPP_
