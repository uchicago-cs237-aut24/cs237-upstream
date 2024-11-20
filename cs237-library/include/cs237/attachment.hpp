/*! \file attachment.hpp
 *
 * Support code for CMSC 23740 Autumn 2024.  Attachments for off-screen rendering.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_ATTACHMENT_HPP_
#define _CS237_ATTACHMENT_HPP_

#ifndef _CS237_HPP_
#error "cs237/attachment.hpp should not be included directly"
#endif

namespace cs237 {

/// Frame-buffer attachments for off-screen rendering support
class Attachment {
public:

    /// \brief Construct a frame-buffer attachment
    /// \param app    the owning application
    /// \param wid    the width of the attachment
    /// \param ht     the height of the attachment
    /// \param fmt    the pixel format of the attachment
    /// \param usage  the way that this attachment is going to be used
    Attachment (
        cs237::Application *app,
        uint32_t wid,
        uint32_t ht,
        vk::Format fmt,
        vk::ImageUsageFlags usage);

    /// destructor
    ~Attachment ();

    /// get the width of the buffer
    uint32_t width() const { return this->_wid; }

    /// get the height of the buffer
    uint32_t height() const { return this->_ht; }

    /// get the pixel format
    vk::Format format() const { return this->_fmt; }

    /// return the image view for the attachment
    vk::ImageView imageView () const { return this->_view; }

protected:
    cs237::Application *_app;   ///< the owning application
    vk::Image _img;             ///< Vulkan image to hold the attachment
    vk::DeviceMemory _mem;      ///< device memory for the attachment image
    vk::ImageView _view;        ///< image view for attachment image
    uint32_t _wid;              ///< attachment width
    uint32_t _ht;               ///< attachment height
    vk::Format _fmt;            ///< the pixel format

};

/// Depth-buffer attachment for off-screen rendering
class DepthAttachment : public Attachment {
public:

    /// \brief Construct a depth-buffer attachment using the "best" format for
    ///        a depth buffer
    /// \param app    the owning application
    /// \param wid    the width of the attachment
    /// \param ht     the height of the attachment
    /// \param fmt    the pixel format of the attachment
    /// \param usage  the way that this attachment is going to be used
    DepthAttachment (cs237::Application *app, uint32_t wid, uint32_t ht)
    : Attachment (app, wid, ht,
        app->_depthStencilBufferFormat(true, false),
        vk::ImageUsageFlagBits::eDepthStencilAttachment)
    { }

    /// destructor
    ~DepthAttachment () {};

};

} // namespace cs237

#endif // !_CS237_ATTACHMENT_HPP_
