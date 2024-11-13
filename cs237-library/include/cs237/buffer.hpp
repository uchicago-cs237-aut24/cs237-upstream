/*! \file buffer.hpp
 *
 * Support code for CMSC 23740 Autumn 2024.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_BUFFER_HPP_
#define _CS237_BUFFER_HPP_

#ifndef _CS237_HPP_
#error "cs237/buffer.hpp should not be included directly"
#endif

namespace cs237 {

/// A base class for buffer objects of all kinds
class Buffer {
public:
    /// get the Vulkan buffer object for this buffer
    vk::Buffer vkBuffer () const { return this->_buf; }

    /// get the memory object for this buffer
    MemoryObj *memory () const { return this->_mem; }

    /// get the memory requirements of this buffer
    vk::MemoryRequirements requirements ()
    {
        vk::MemoryRequirements reqs;
        this->_app->_device.getBufferMemoryRequirements(this->_buf, &reqs);
        return reqs;
    }

protected:
    Application *_app;          ///< the application
    vk::Buffer _buf;            ///< the Vulkan buffer object
    MemoryObj *_mem;            ///< the Vulkan memory object that holds the buffer

    /// constructor
    /// \param app    the owning application object
    /// \param usage  specify the purpose of the buffer object
    /// \param sz     the buffer's size in bytes
    Buffer (Application *app, vk::BufferUsageFlags usage, size_t sz)
      : _app(app)
    {
        vk::BufferCreateInfo info(
            {}, /* flags */
            sz,
            usage,
            vk::SharingMode::eExclusive, /* sharingMode */
            {}); /* queueFamilyIndices */

        this->_buf = app->_device.createBuffer (info);
        this->_mem = new MemoryObj(app, this->requirements());

        // bind the memory object to the buffer
        this->_app->_device.bindBufferMemory(this->_buf, this->_mem->_mem, 0);

    }

    /// destructor
    ~Buffer ()
    {
        delete this->_mem;
        this->_app->_device.destroyBuffer (this->_buf, nullptr);
    }

    /// copy data to a subrange of the device memory object
    /// \param src     address of data to copy
    /// \param offset  offset (in bytes) from the beginning of the buffer
    ///                to copy the data to
    /// \param sz      size in bytes of the data to copy
    void _copyTo (const void *src, size_t offset, size_t sz)
    {
        this->_mem->copyTo(src, offset, sz);
    }

    /// copy data to the device memory object
    /// \param src  address of data to copy
    void _copyTo (const void *src) { this->_mem->copyTo(src); }

};

/// Buffer class for vertex data; the type parameter `V` is the type of an
/// individual vertex.
template <typename V>
class VertexBuffer : public Buffer {
public:

    /// the type of vertices
    using VertexType = V;

    /// constructor
    /// \param app     the owning application object
    /// \param nVerts  the number of vertices in the buffer
    ///
    /// This constructor creates the vertex buffer and allocates GPU-side memory
    /// for it.
    VertexBuffer (Application *app, uint32_t nVerts)
      : Buffer (app, vk::BufferUsageFlagBits::eVertexBuffer, nVerts*sizeof(V))
    { }

    /// constructor with initialization
    /// \param app  the owning application object
    /// \param src  the array of vertices used to initialize the buffer
    ///
    /// This constructor creates the vertex buffer, allocates GPU-side memory
    /// for it, and then copies the data from `src` to the GPU.
    VertexBuffer (Application *app, vk::ArrayProxy<V> const &src)
      : VertexBuffer(app, src.size())
    {
        this->copyTo(src);
    }

    /// copy vertices to the device memory object
    /// \param src  proxy array of vertices
    void copyTo (vk::ArrayProxy<V> const &src)
    {
        assert ((src.size() * sizeof(V) <= this->_mem->size()) && "src is too large");
        this->_copyTo(src.data(), 0, src.size()*sizeof(V));
    }

    /// copy vertices to the device memory object
    /// \param src     proxy array of vertices
    /// \param offset  offset from the beginning of the buffer to copy the data to
    void copyTo (vk::ArrayProxy<V> const &src, uint32_t offset)
    {
        assert (((src.size()+offset) * sizeof(V) <= this->_mem->size())
            && "src is too large");
        this->_copyTo(src.data(), offset*sizeof(V), src.size()*sizeof(V));
    }

};

/// Buffer class for index data; the type parameter `I` is the index type.
template <typename I>
class IndexBuffer : public Buffer {
public:

    /// the type of indices
    using IndexType = I;

    /// constructor
    /// \param app       the owning application object
    /// \param nIndices  the number of indices in the buffer
    IndexBuffer (Application *app, uint32_t nIndices)
      : Buffer (app, vk::BufferUsageFlagBits::eIndexBuffer, nIndices*sizeof(I)),
        _nIndices(nIndices)
    { }

    /// constructor with initialization
    /// \param app  the owning application object
    /// \param src  the array of indices used to initialize the buffer
    IndexBuffer (Application *app, vk::ArrayProxy<I> const &src)
      : IndexBuffer(app, src.size())
    {
        this->copyTo(src);
    }

    /// get the number of indices in the buffer
    uint32_t nIndices () const { return this->_nIndices; }

    /// copy indices to the device memory object
    /// \param src  the array of indices that are copied to the buffer
    void copyTo (vk::ArrayProxy<I> const &src)
    {
        assert ((src.size() * sizeof(I) <= this->_mem->size()) && "src is too large");
        this->_copyTo(src.data(), 0, src.size()*sizeof(I));
    }

    /// copy vertices to the device memory object
    /// \param src     the array of indices that are copied to the buffer
    /// \param offset  offset from the beginning of the buffer to copy the data to
    void copyTo (vk::ArrayProxy<I> const &src, uint32_t offset)
    {
        assert (((src.size()+offset) * sizeof(I) <= this->_mem->size())
            && "src is too large");
        this->_copyTo(src.data(), offset*sizeof(I), src.size()*sizeof(I));
    }

private:
    uint32_t _nIndices;

};

/// Buffer class for uniform data; the type parameter `UB` is the C++
/// struct type of the buffer contents
template <typename UB>
class UniformBuffer : public Buffer {
public:

    /// the type of the buffer's contents
    using BufferType = UB;

    /// constructor
    /// \param app  the owning application object
    UniformBuffer (Application *app)
      : Buffer (app, vk::BufferUsageFlagBits::eUniformBuffer, sizeof(UB))
    { }

    /// constructor with initialization
    /// \param app      the owning application object
    /// \param[in] src  the buffer contents to copy to the Vulkan memory buffer
    UniformBuffer (Application *app, UB const &src)
      : UniformBuffer(app)
    {
        this->copyTo(src);
    }

    /// copy the buffer data to the device memory object
    /// \param[in] src  the buffer contents to copy to the Vulkan memory buffer
    void copyTo (UB const &src)
    {
        this->_copyTo(&src, 0, sizeof(UB));
    }

    /// get the default buffer-descriptor info for this buffer
    vk::DescriptorBufferInfo descInfo ()
    {
        return vk::DescriptorBufferInfo(this->_buf, 0, sizeof(UB));
    }

};

/// A `StorageVertexBuffer<SV>` is both a storage buffer and a vertex
/// buffer that can be read by the graphics pipeline, where `SV` is the
/// vertex type
template <typename SV>
class StorageVertexBuffer : public Buffer {
public:

    /// the type of the buffer's contents
    using BufferType = SV;

    /// constructor
    /// \param app     the owning application object
    /// \param nVerts  the number of vertices that can be stored in the buffer
    StorageVertexBuffer (Application *app, uint32_t nVerts)
    : Buffer (app, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer, nVerts*sizeof(SV))
    { }

    /// constructor with initialization
    /// \param app  the owning application object
    /// \param src  the array of vertices used to initialize the buffer
    StorageVertexBuffer (Application *app, vk::ArrayProxy<SV> const &src)
    : StorageVertexBuffer(app, src.size())
    {
        this->copyTo(src);
    }

    /// copy vertices to the device memory object
    /// \param src  proxy array of vertices
    void copyTo (vk::ArrayProxy<SV> const &src)
    {
        assert ((src.size() * sizeof(SV) <= this->_mem->size()) && "src is too large");
        this->_copyTo(src.data(), 0, src.size()*sizeof(SV));
    }

    /// copy vertices to the device memory object
    /// \param src     proxy array of vertices
    /// \param offset  offset from the beginning of the buffer to copy the data to
    void copyTo (vk::ArrayProxy<SV> const &src, uint32_t offset)
    {
        assert (((src.size()+offset) * sizeof(SV) <= this->_mem->size())
            && "src is too large");
        this->_copyTo(src.data(), offset*sizeof(SV), src.size()*sizeof(SV));
    }

    /// get the default buffer-descriptor info for this buffer
    vk::DescriptorBufferInfo descInfo ()
    {
        return vk::DescriptorBufferInfo(this->_buf, 0, sizeof(SV));
    }

};

/// A `StorageBuffer<S>` is both a wrapper around a Vulkan storage buffer
/// that can be read by the graphics pipeline.
template <typename S>
class StorageBuffer : public Buffer {
public:

    /// the type of the buffer's contents
    using BufferType = S;

    /// constructor
    /// \param app  the owning application object
    StorageBuffer (Application *app)
    : Buffer (app, vk::BufferUsageFlagBits::eStorageBuffer, sizeof(S))
    { }

    /// constructor with initialization
    /// \param app      the owning application object
    /// \param[in] src  the buffer contents to copy to the Vulkan memory buffer
    StorageBuffer (Application *app, S const &src)
    : StorageBuffer(app)
    {
        this->copyTo(src);
    }

    /// constructor
    /// \param app  the owning application object
    StorageBuffer (Application *app, vk::ArrayProxy<S> const &src, uint32_t nValues)
    : Buffer (app, vk::BufferUsageFlagBits::eStorageBuffer, nValues*sizeof(S))
    {
        this->copyTo(src);
    }

    /// copy the buffer data to the device memory object
    /// \param[in] src  the buffer contents to copy to the Vulkan memory buffer
    /** TODO: Will have to change this potentially. SSBO having an identity crisis */
    void copyTo (S const &src)
    {
        this->_copyTo(&src, 0, sizeof(S));
    }

    /// get the default buffer-descriptor info for this buffer
    vk::DescriptorBufferInfo descInfo ()
    {
        return vk::DescriptorBufferInfo(this->_buf, 0, sizeof(S));
    }

    /// copy vertices to the device memory object
    /// \param src  proxy array of vertices
    void copyTo (vk::ArrayProxy<S> const &src)
    {
        assert ((src.size() * sizeof(S) <= this->_mem->size()) && "src is too large");
        this->_copyTo(src.data(), 0, src.size()*sizeof(S));
    }

    /// copy vertices to the device memory object
    /// \param src     proxy array of vertices
    /// \param offset  offset from the beginning of the buffer to copy the data to
    void copyTo (vk::ArrayProxy<S> const &src, uint32_t offset)
    {
        assert (((src.size()+offset) * sizeof(S) <= this->_mem->size())
            && "src is too large");
        this->_copyTo(src.data(), offset*sizeof(S), src.size()*sizeof(S));
    }

};

} // namespace cs237

#endif // !_CS237_BUFFER_HPP_
