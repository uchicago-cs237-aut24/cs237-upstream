/*! \file height-field.hpp
 *
 * \author John Reppy
 *
 * This file defines the HeightField class, which is a wrapper around the
 * height-field information in a scene.
 */

/* CMSC23740 Project 5 sample code (Autumn 2024)
 *
 * COPYRIGHT (c) 2024 John Reppy (http://www.cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _HEIGHT_FIELD_HPP_
#define _HEIGHT_FIELD_HPP_

#include "cs237/cs237.hpp"

class HeightField {
  public:

    /// construct a HeightField object
    /// \param file    the name of the height-field file
    /// \param width   the width (X dimension) covered by the ground in
    ///                world-space coordinates
    /// \param height  the height (Z dimension) covered by the ground in
    ///                world-space coordinates
    /// \param vScale  the vertical scaling (Y dimension) factor
    /// \param color   the color for non-texturing modes
    /// \param cmap    the color texture image for the ground
    /// \param nmap    the normal-map texture image for the ground
    HeightField (
        std::string const &file,
        float width, float height, float vScale,
        glm::vec3 const &color,
        cs237::Image2D *cmap,
        cs237::Image2D *nmap);

    /// the width of the ground object in world-space
    float width () const { return 2.0f * this->_halfWid; }

    /// the height of the ground object in world-space
    float height () const { return 2.0f * this->_halfHt; }

    /// the number of rows of data in the height-field
    uint32_t numRows () const { return this->_img->height(); }

    /// the number of rows of data in the height-field
    uint32_t numCols () const { return this->_img->width(); }

    /// the number of vertices in the mesh represented by the height field
    uint32_t numVerts () const { return this->numRows() * this->numCols(); }

    /// the number of triangles in a full triangulation of the ground (two per square)
    uint32_t numTris () const { return 2*(this->numRows()-1)*(this->numCols()-1); }

    /// return the vertex ID (i.e., the index into the image data) for the given
    /// row and column
    uint32_t indexOf (uint32_t row, uint32_t col) const
    {
        assert ((row < this->numRows()) && (col < this->numCols()));
        return row * this->numCols() + col;
    }

    /// return the height-field value at the given row and column
    uint16_t valueAt (uint32_t row, uint32_t col) const;

    /// return the world-space position for the given row and column of the heightfield
    glm::vec3 const posAt (int row, int col) const
    {
        return glm::vec3(
            this->_scaleX * float(col) - this->_halfWid,
            this->_scaleY * float(this->valueAt(row, col)),
            this->_scaleZ * float(row) - this->_halfHt);
    }

    /// return the world-space AABB for the height field
    cs237::AABBf_t bbox () const {
        return cs237::AABBf_t(
            glm::vec3(-this->_halfWid, this->_minHt, -this->_halfHt),
            glm::vec3( this->_halfWid, this->_maxHt,  this->_halfHt));
    }

    /// return the color for the ground in wireframe and flat-shading rendering modes
    glm::vec3 const &color () const { return this->_color; }

    /// return the color map image for the ground
    cs237::Image2D *colorMap () const { return this->_colorMap; }

    /// return the normal map image for the ground
    cs237::Image2D *normalMap () const { return this->_normMap; }

  private:
    const cs237::Image2D *_img; ///< the underlying image data
    const float _halfWid;       ///< half the width in world space
    const float _halfHt;        ///< half the height in world space
    float _minHt;               ///< the minimum elevation
    float _maxHt;               ///< the maximum elevation
    const float _scaleX;        ///< horizontal scaling factor in X dimension
    const float _scaleY;        ///< vertical scaling factor (Y dimension)
    const float _scaleZ;        ///< horizontal scaling factor in Z dimension
    const glm::vec3 _color;     ///< the color of the ground in wireframe, flat-shading,
                                ///  or diffuse mode
    cs237::Image2D *_colorMap;	///< the color texture image for the ground in
                                ///  texturing and normal-mapping modes
    cs237::Image2D *_normMap;	///< the normal-map texture image for the ground in
                                ///  normal-mapping mode.

};

#endif // !_HEIGHT_FIELD_HPP_
