/*! \file memory-obj.cpp
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

MemoryObj::MemoryObj (Application *app, vk::MemoryRequirements const &reqs)
  : _app(app), _sz(reqs.size)
{
    vk::MemoryAllocateInfo allocInfo(
        // the allocated memory size must be a multiple of the alignment
        (reqs.size + reqs.alignment - 1) & ~(reqs.alignment - 1),
        app->_findMemory(
            reqs.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eHostVisible
            | vk::MemoryPropertyFlagBits::eHostCoherent));

    this->_mem = app->_device.allocateMemory(allocInfo);
}

MemoryObj::~MemoryObj ()
{
    this->_app->_device.freeMemory (this->_mem);
}

} // namespace cs237
