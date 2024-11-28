/*! \file gobject-util.hpp
 *
 * Support code for CMSC 23740 Autumn 2024.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _GOBJECT_UTIL_HPP_
#define _GOBJECT_UTIL_HPP_

#include "cs237/cs237.hpp"
#include "obj.hpp"

namespace cs237 {
namespace gobj {
namespace __detail {

constexpr double kPi = 3.14159265358979323846;

inline Obj *allocGroup (
    std::string_view name,
    uint32_t nVerts,
    uint32_t nIndices,
    bool hasNorms,
    bool hasTCs)
{
    // allocate memory for the group
    Obj *obj = new Obj(name, nVerts, nIndices);

    obj->norms = (hasNorms ? new glm::vec3[nVerts] : nullptr);
    obj->txtCoords = (hasTCs ? new glm::vec2[nVerts] : nullptr);

    return obj;
}

// apply a uniform scaling and translation to the vertices of an object
inline void scaleAndMove (float scale, glm::vec3 center, Obj *obj)
{
    for (int i = 0;  i < obj->nVerts;  ++i) {
        obj->verts[i] = center + scale * obj->verts[i];
    }
}

// apply an linear transform plus a translation to the vertices and normals of an object
inline void transform (glm::mat3 M, glm::vec3 center, Obj *obj)
{
    for (int i = 0;  i < obj->nVerts;  ++i) {
        obj->verts[i] = center + M * obj->verts[i];
    }
    if (obj->norms != nullptr) {
        glm::mat3 normM = glm::inverseTranspose(M);
        for (int i = 0;  i < obj->nVerts;  ++i) {
            obj->norms[i] = normM * obj->norms[i];
        }
    }

}

} // namespace __detail
} // namespace gobj
} // namespace cs237

#endif // !_GOBJECT_UTIL_HPP_
