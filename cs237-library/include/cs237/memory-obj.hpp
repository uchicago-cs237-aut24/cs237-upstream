/*! \file memory-obj.hpp
 *
 * Support code for CMSC 23740 Autumn 2024.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_MEMORY_OBJ_HPP_
#define _CS237_MEMORY_OBJ_HPP_

#ifndef _CS237_HPP_
#error "cs237-memory.hpp should not be included directly"
#endif

namespace cs237 {

/// wrapper around Vulkan memory objects
class MemoryObj {
    friend class Buffer;

public:
    MemoryObj (Application *app, vk::MemoryRequirements const &reqs);
    ~MemoryObj ();

    /// copy data to a subrange of the device memory object
    /// \param src     address of data to copy
    /// \param offset  offset from the beginning of the memory object to copy the data to
    /// \param sz      size in bytes of the data to copy
    void copyTo (const void *src, size_t offset, size_t sz)
    {
        assert (offset + sz <= this->_sz);

        auto dev = this->_app->_device;

        // first we need to map the object into our address space
        auto dst = dev.mapMemory(this->_mem, offset, this->_sz, {});
        // copy the data
        memcpy(dst, src, sz);
        // unmap the object
        this->_app->_device.unmapMemory (this->_mem);
    }

    /// copy data to the device memory object
    /// \param src  address of data to copy
    void copyTo (const void *src) { this->copyTo(src, 0, this->_sz); }

    /// the size of the memory object in bytes
    size_t size () const { return this->_sz; }

    vk::DeviceMemory getDeviceMemory() { return this->_mem; }

protected:
    Application *_app;          ///< the application
    vk::DeviceMemory _mem;      ///< the device memory object
    size_t _sz;                 ///< the size of the memory object

};

} // namespace cs237

#endif // !_CS237_MEMORY_OBJ_HPP_
