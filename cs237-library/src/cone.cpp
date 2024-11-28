/*! \file cone.cpp
 *
 * Support code for CMSC 23740 Autumn 2024.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "gobject-util.hpp"

namespace cs237 {
namespace gobj {

Obj *cone (
    VertexAttrs attrs,
    glm::vec3 pos,
    glm::vec3 dir,
    float radius,
    float height,
    uint32_t slices,
    uint32_t stacks)
{
    if ((radius <= 0.0) || (height <= 0.0)) {
        return nullptr;
    }

    bool hasNorms = hasNormals (attrs);
    bool hasTCs = hasTextureCoords(attrs);

    // set minimums
    slices = (slices < 6 ? 6 : slices);
    stacks = (stacks < 1 ? 1 : stacks);

    // if there are texture coordinates or normals, then we replicate the apex
    // vertices (one per slice).
    uint32_t nApexVerts = ((hasNorms || hasTCs) ? slices : 1);

    // if there are normal vectors, then we replicate the base-edge vertices to
    // make the base a "hard" edge
    uint32_t nBaseEdgeVerts = (hasNorms ? slices : 0);

    // if there are texture coordinates, then we replicate the base-center
    // vertices (one per slice).
    uint32_t nBaseCenterVerts = (hasTCs ? slices : 1);

    // the number of vertices in the mesh is computed as follows
    //  - apex vertices (see above)
    //  - one vertex per slice per stack
    //  - center vertex for base
    uint32_t nVerts = nApexVerts + stacks * slices + nBaseEdgeVerts + nBaseCenterVerts;
    uint32_t nTriFaces = 2*slices;  // the triangular faces at the apex and base
    uint32_t nQuadFaces = (stacks - 1) * slices;
    uint32_t nTris = nTriFaces + 2 * nQuadFaces;

    auto grp = __detail::allocGroup("cone", nVerts, 3*nTris, hasNorms, hasTCs);

    // angle around the Z axis between the start and end of a slice
    double sliceAngle = (2.0 * __detail::kPi) / double(slices);

  // construct a rotation matrix to orient the cone
    dir = glm::normalize(dir);
    glm::vec3 xAxis, zAxis;
    float magX = fabsf(dir.x), magY = fabsf(dir.y), magZ = fabsf(dir.z);
    if ((magX <= magY) && (magX <= magZ)) {
        zAxis = glm::normalize(cross(glm::vec3(1.0f, 0.0f, 0.0f), dir));
        xAxis = glm::normalize(cross(dir, zAxis));
    }
    else if ((magY <= magX) && (magY <= magZ)) {
        zAxis = glm::normalize(cross(glm::vec3(0.0f, 1.0f, 0.0f), dir));
        xAxis = glm::normalize(cross(dir, zAxis));
    }
    else {
        assert ((magZ <= magX) && (magZ <= magY));
        zAxis = glm::normalize(cross(glm::vec3(0.0f, 0.0f, 1.0f), dir));
        xAxis = glm::normalize(cross(dir, zAxis));
    }
    glm::mat3 R(xAxis, dir, zAxis);

    /********** Vertices **********/

    // construct a mesh with the apex at the origin and the center of the
    // base at <0, height, 0>.
    int idx = 0;
    // start at the apex
    for (int i = 0;  i < nApexVerts;  ++i) {
        grp->verts[idx++] = pos;
    }
    for (int stkIdx = 0;  stkIdx < stacks;  ++stkIdx) {
        auto s = float(stkIdx + 1) / float(stacks);
        auto y = s * float(height);
        auto r = s * float(radius);
        for (int sliceIdx = 0;  sliceIdx < slices;  ++sliceIdx) {
            auto theta = double(sliceIdx) * sliceAngle;
            auto x = r * float(std::cos(theta));
            auto z = r * float(std::sin(theta));
            grp->verts[idx++] = R * glm::vec3(x, y, z) + pos;
        }
    }
    // if we have normals, then we have two copies of each base-edge vertex
    if (hasNorms) {
        for (int sliceIdx = 0;  sliceIdx < slices;  ++sliceIdx) {
            grp->verts[idx] = grp->verts[idx - slices];
            idx++;
        }
    }
    // the base-center vertex
    for (int i = 0;  i < nBaseCenterVerts;  ++i) {
        grp->verts[idx++] = R * glm::vec3(0, height, 0) + pos;
    }
    assert (idx == nVerts && "incorrect number of vertices");

    /********** Normals **********/

    if (hasNorms) {
        // the normals along a slice edge are all the same, so the outer loop
        // is for the slices and the inner loop is for the stacks
        for (int sliceIdx = 0;  sliceIdx < slices;  ++sliceIdx) {
            auto theta = double(sliceIdx) * sliceAngle;
            auto x = radius * float(std::cos(theta));
            auto z = radius * float(std::sin(theta));
            glm::vec3 norm = glm::normalize(R * glm::vec3(x, -float(height), z));
            for (int stkIdx = 0;  stkIdx <= stacks;  ++stkIdx) {
                grp->norms[stkIdx * slices + sliceIdx] = norm;
            }
        }
        // normals for base
        glm::vec3 norm = dir;
        for (int i = nApexVerts + stacks * slices; i < nVerts;  ++i) {
            grp->norms[i] = norm;
        }
    }

    /********** Texture Coordinates **********/

    if (hasTCs) {
        /* TODO */
    }

    /********** Indices **********/

    idx = 0;

    // the triangles around the apex
    for (int i = 0; i < slices; ++i) {
        uint32_t i0, i1, i2;
        if (hasTCs) {
            // the pole vertices are at indices [0 .. slices-1] and the first stack
            // vertices are at indices [slices .. 2*slices-1]
            i0 = i;  // apex vertex
            i1 = slices + i;
            i2 = slices + ((i + 1) % slices);
        } else {
            i0 = 0;
            i1 = i + 1;
            i2 = (i + 1) % slices + 1;
        }
        grp->indices[idx++] = i0;
        grp->indices[idx++] = i2;
        grp->indices[idx++] = i1;
    }

    // the triangulated quads
    for (int stkIdx = 1;  stkIdx < stacks;  ++stkIdx) {
        uint32_t j0 = (stkIdx - 1) * slices + nApexVerts;
        uint32_t j1 = stkIdx * slices + nApexVerts;
        for (int sliceIdx = 0;  sliceIdx < slices;  ++sliceIdx) {
            // the indices of the quad
            uint32_t i0 = j0 + sliceIdx;
            uint32_t i1 = j0 + (sliceIdx + 1) % slices;
            uint32_t i2 = j1 + (sliceIdx + 1) % slices;
            uint32_t i3 = j1 + sliceIdx;
            // first triangle
            grp->indices[idx++] = i0;
            grp->indices[idx++] = i3;
            grp->indices[idx++] = i2;
            // second triangle
            grp->indices[idx++] = i0;
            grp->indices[idx++] = i2;
            grp->indices[idx++] = i1;
        }
    }

    // the base triangles
    for (int sliceIdx = 0;  sliceIdx < slices;  ++sliceIdx) {
        uint32_t i0, i1, i2;
        if (hasTCs) {
            // the base-center vertices are at indices [nVerts-slices .. nVerts-1]
            i0 = nVerts - slices + sliceIdx;
            i1 = nVerts - 2*slices + sliceIdx;
            i2 = nVerts - 2*slices + (sliceIdx + 1) % slices;
        } else {
            i0 = nVerts - 1;
            i1 = nVerts - slices + sliceIdx;
            i2 = nVerts - slices + (sliceIdx + 1) % slices + 1;
        }
        grp->indices[idx++] = i0;
        grp->indices[idx++] = i2;
        grp->indices[idx++] = i1;
    }

    return grp;

}

} // namespace cs237
} // namespace gobj
