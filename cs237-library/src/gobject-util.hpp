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

inline OBJ::Group *allocGroup (
    std::string_view name,
    uint32_t nVerts,
    uint32_t nIndices,
    bool hasNorms,
    bool hasTCs)
{
    // allocate memory for the group
    OBJ::Group *grp = new OBJ::Group;

    grp->name = std::string(name);
    grp->material = -1;
    grp->nVerts = nVerts;
    grp->nIndices = nIndices;
    grp->verts = new glm::vec3[nVerts];
    grp->norms = (hasNorms ? new glm::vec3[nVerts] : nullptr);
    grp->txtCoords = (hasTCs ? new glm::vec2[nVerts] : nullptr);
    grp->indices = new uint32_t[nIndices];

    return grp;
}

// apply a uniform scaling and translation to the vertices of an object
inline void scaleAndMove (float scale, glm::vec3 center, OBJ::Group *grp)
{
    for (int i = 0;  i < grp->nVerts;  ++i) {
        grp->verts[i] = center + scale * grp->verts[i];
    }
}

// apply an linear transform plus a translation to the vertices and normals of an object
inline void transform (glm::mat3 M, glm::vec3 center, OBJ::Group *grp)
{
    for (int i = 0;  i < grp->nVerts;  ++i) {
        grp->verts[i] = center + M * grp->verts[i];
    }
    if (grp->norms != nullptr) {
        glm::mat3 normM = glm::inverseTranspose(M);
        for (int i = 0;  i < grp->nVerts;  ++i) {
            grp->norms[i] = normM * grp->norms[i];
        }
    }

}

} // namespace __detail
} // namespace gobj
} // namespace cs237

#endif // !_GOBJECT_UTIL_HPP_
