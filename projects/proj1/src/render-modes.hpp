/*! \file render-modes.hpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 1
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _RENDER_MODES_HPP_
#define _RENDER_MODES_HPP_

enum class RenderMode : int {
    eWireframe = 0,
    eFlatShading,
    eGouraudShading,
    ePhongShading,
    eNumModes
};

/* cast to integer type for array indexing */
constexpr int kWireframe = static_cast<int>(RenderMode::eWireframe);
constexpr int kFlatShading = static_cast<int>(RenderMode::eFlatShading);
constexpr int kGouraudShading = static_cast<int>(RenderMode::eGouraudShading);
constexpr int kPhongShading = static_cast<int>(RenderMode::ePhongShading);
constexpr int kNumRenderModes = static_cast<int>(RenderMode::eNumModes);

#endif // !_RENDER_MODES_HPP_
