/*! \file height-field.cpp
 *
 * \author John Reppy
 *
 * This file defines the HeightField class, which is a wrapper around the
 * height-field information in a scene.
 */

/* CMSC23740 Project 4 sample code (Autumn 2024)
 *
 * COPYRIGHT (c) 2024 John Reppy (http://www.cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "height-field.hpp"

// construct a HeightField object
HeightField::HeightField (
    std::string const &file,
    float width, float height, float vScale,
    glm::vec3 const &color,
    cs237::Image2D *cmap,
    cs237::Image2D *nmap)
  : _img(new cs237::Image2D(file, false)),
    _halfWid(0.5*width), _halfHt(0.5*height),
    _minHt(0), _maxHt(0),
    _scaleX(width / float(this->numCols() - 1)),
    _scaleY(vScale),
    _scaleZ(height / float(this->numRows() -1 )),
    _color(color), _colorMap(cmap), _normMap(nmap)
{
  // validate the arguments
    if ((this->_img->type() != cs237::ChannelTy::U8)
    && (this->_img->type() != cs237::ChannelTy::U16)) {
        ERROR("HeightField::HeightField: invalid image type");
    }
    if ((width <= 0.0) || (height <= 0.0) || (vScale <= 0.0)) {
        ERROR("HeightField::HeightField: invalid scaling");
    }
  // find the maximum height of the height-field
    int minHt, maxHt;
    if (this->_img->type() == cs237::ChannelTy::U8) {
        const uint8_t *p = static_cast<const uint8_t *>(this->_img->data());
        minHt = maxHt = *p++;
        for (int i = 1;  i < this->numVerts();  i++, p++) {
            if (*p > maxHt) maxHt = *p;
            else if (*p < minHt) minHt = *p;
        }
    }
    else {
        const uint16_t *p = static_cast<const uint16_t *>(this->_img->data());
        minHt = maxHt = *p++;
        for (int i = 1;  i < this->numVerts();  i++, p++) {
            if (*p > maxHt) maxHt = *p;
            else if (*p < minHt) minHt = *p;
        }
    }
    this->_minHt = this->_scaleY * float(minHt);
    this->_maxHt = this->_scaleY * float(maxHt);

}

// return the height-field value at the given row and column
uint16_t HeightField::valueAt (uint32_t row, uint32_t col) const
{
    uint32_t idx = this->indexOf (row, col);
    if (this->_img->type() == cs237::ChannelTy::U8) {
        return static_cast<uint16_t>(static_cast<uint8_t *>(this->_img->data())[idx]);
    }
    else {
        return static_cast<uint16_t *>(this->_img->data())[idx];
    }
}
