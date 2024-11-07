/*! \file render-modes.hpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 3
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _RENDER_MODES_HPP_
#define _RENDER_MODES_HPP_

constexpr int kNormalMapBit = 1;
constexpr int kShadowsBit = 2;

/// The supported render modes
//
enum class RenderMode : int {
    eTextureWOShadows = 0,
    eNormalMapWOShadows = kNormalMapBit,
    eTextureWShadows = kShadowsBit,
    eNormalMapWShadows = kNormalMapBit|kShadowsBit
};

/// is normal-mapping enabled in the given render mode?
inline bool normalMapEnabled (RenderMode m)
{
    return (static_cast<int>(m) & kNormalMapBit) == kNormalMapBit;
}

/// are shadows enabled in the given render mode?
inline bool shadowsEnabled (RenderMode m)
{
    return (static_cast<int>(m) & kShadowsBit) == kShadowsBit;
}

/// enable normal mapping in the render mode
/// \param m  the current render mode
/// \return   the mode `m` with normal mapping enabled
inline RenderMode enableNormalMap (RenderMode m)
{
    return static_cast<RenderMode>(static_cast<int>(m) | kNormalMapBit);
}

/// disable normal mapping in the render mode
/// \param m  the current render mode
/// \return   the mode `m` with normal mapping disabled
inline RenderMode disableNormalMap (RenderMode m)
{
    return static_cast<RenderMode>(static_cast<int>(m) & ~kNormalMapBit);
}

/// toggle shadows in the render mode
/// \param m  the current render mode
/// \return   the mode `m` with the shadows bit flipped
inline RenderMode toggleShadows (RenderMode m)
{
    return static_cast<RenderMode>(static_cast<int>(m) ^ kShadowsBit);
}

#endif // !_RENDER_MODES_HPP_
