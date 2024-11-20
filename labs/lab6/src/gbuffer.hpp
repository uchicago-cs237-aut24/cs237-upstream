/*! \file gbuffer.hpp
 *
 * CMSC 23740 Autumn 2024 Lab 6.
 *
 * A minimal GBuffer implementation.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _GBUFFER_HPP_
#define _GBUFFER_HPP_

#include "cs237/cs237.hpp"

/// This class is a container for the various bits of Vulkan state needed to
/// implement the GBuffer
//
class GBuffer {
public:

    /// the number of buffer/attachments in the geometry buffer
    static constexpr uint32_t kNumBuffers = 2;

    /// constructor
    /// \param app    the owning application
    /// \param wid    the width of the buffer
    /// \param ht     the height of the buffer
    GBuffer (cs237::Application *app, uint32_t wid, uint32_t ht);
    ~GBuffer ();

    /// get the attachment descriptors and references for the G-buffer
    /// \param[out] descs  vector that will contain the attachment descriptors
    /// \param[out] refs   vector that will contain the attachment references
    void initAttachments (
        std::vector<vk::AttachmentDescription> &descs,
        std::vector<vk::AttachmentReference> &refs);

    /// initialize the descriptor-set pool and layout for the G-buffer
    void initDescriptorSetLayout ();

    /// create and initialize the descriptor-set for the G-buffer
    void initDescriptorSet ();

    /// resize the attachments
    /// \param wid    the new width of the buffer
    /// \param ht     the new height of the buffer
    void resize (uint32_t wid, uint32_t ht);

    /// get the descriptor-set layout for the G-buffer samplers
    vk::DescriptorSetLayout descriptorSetLayout () { return this->_dsLayout; }

    /// get the descriptor set for the G-buffer samplers
    vk::DescriptorSet descriptorSet () { return this->_descSet; }

    /// get the sampler
    vk::Sampler sampler() const { return this->_sampler; }

    /// get the frame-buffer attachments for the buffers
    std::vector<vk::ImageView> attachments () const
    {
        return std::vector<vk::ImageView>{
                this->_albedo->imageView(),
                this->_normal->imageView()
            };
    }

    /// get the clear values for the buffers
    std::vector<vk::ClearValue> clearValues () const;

private:
    cs237::Application *_app;           ///< the owning application
    uint32_t _wid, _ht;                 ///< dimensions of frame buffer
    cs237::Attachment *_albedo;         ///< offscreen color buffer
    cs237::Attachment *_normal;         ///< offscreen normal buffer
    vk::Sampler _sampler;               ///< sampler for reading the attachments; note
                                        ///  that we can reuse this for each attachment
    vk::DescriptorPool _descPool;       ///< descriptor-set pool for the geometry buffer
    vk::DescriptorSetLayout _dsLayout;  ///< the descriptor-set layout for the samplers
    vk::DescriptorSet _descSet;         ///< the descriptors for the samplers
};

#endif // !_GBUFFER_HPP_
