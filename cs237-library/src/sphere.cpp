/*! \file sphere.cpp
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

/* Generate a "UV" mesh for a sphere; see
 * https://danielsieger.com/blog/2021/03/27/generating-spheres.html
 * for a discussion of this method (and others).
 */

OBJ::Group *sphere (
    VertexAttrs attrs,
    glm::vec3 center,
    float radius,
    uint32_t slices,
    uint32_t stacks)
{
    if (radius <= 0.0) {
        return nullptr;
    }

    bool hasNorms = hasNormals (attrs);
    bool hasTCs = hasTextureCoords(attrs);

    slices = (slices < 5 ? 5 : slices);
    stacks = (stacks < 5 ? 5 : stacks);

    // If the sphere has texture coordinates, then we replicate the polar vertices
    // for each triangle in the top-most and bottom-most slices so that they can
    // have different texture coordinates.  The normal vectors, however are
    // the same for any instance of a vertex.
    uint32_t nPolarVerts = (hasTCs ? slices : 1);  // number of vertices at the pole

    // the number of vertices in the mesh is computed as follows
    //  - polar vertices (see above)
    //  - one vertex per slice per stack excluding the bottom one
    uint32_t nVerts = 2*nPolarVerts + (stacks - 1) * slices;
    uint32_t nTriFaces = 2*slices;  // the triangular faces at the poles
    uint32_t nQuadFaces = (stacks - 2) * slices;
    uint32_t nTris = nTriFaces + 2 * nQuadFaces;

    auto grp = __detail::allocGroup("sphere", nVerts, 3*nTris, hasNorms, hasTCs);

    // angle around the Y axis between the start and end of a slice
    double sliceAngle = (2.0 * __detail::kPi) / double(slices);

    // angle between bottom and top of stack
    double stackAngle = __detail::kPi / double(stacks);

    // we first construct a unit sphere centered at the origin
    int idx = 0;
    // start at the south pole
    for (int i = 0;  i < nPolarVerts;  ++i) {
        grp->verts[idx++] = glm::vec3(0, -radius, 0);
    }
    for (int stkIdx = 1;  stkIdx < stacks;  ++stkIdx) {
        double phi = __detail::kPi - double(stkIdx) * stackAngle;
        double sinPhi = std::sin(phi);
        double cosPhi = std::cos(phi);
        for (int sliceIdx = 0;  sliceIdx < slices;  ++sliceIdx) {
            // we are enumerating the vertices in CW order when looking in the
            // -Y direction.
            double theta = double(sliceIdx) * sliceAngle;
            auto x = float(sinPhi * std::cos(theta));
            auto y = float(cosPhi);
            auto z = float(sinPhi * std::sin(theta));
            grp->verts[idx++] = glm::vec3(x, y, z);
        }
    }
    // finish at the north pole
    for (int i = 0;  i < nPolarVerts;  ++i) {
        grp->verts[idx++] = glm::vec3(0, radius, 0);
    }
    assert (idx == nVerts && "incorrect number of vertices");

    if (hasNorms) {
        // the normal vectors are just the vertices of the unit sphere
        float scale = 1.0 / radius;
        for (int i = 0;  i < nVerts;  ++i) {
            grp->norms[i] = glm::normalize(grp->verts[i]);
        }
    }

    // scale the sphere to the specifed width and move it
    __detail::scaleAndMove (radius, center, grp);

    if (hasTCs) {
        idx = 0;
        // instances of the south pole
        for (int i = 0;  i < slices;  ++i) {
            grp->txtCoords[idx++] = glm::vec2(float(i+1) / float(slices), 0.0);
        }
        // texture coordinates for intermediate vertices
        for (int stkIdx = 1;  stkIdx < stacks;  ++stkIdx) {
            float t = float(stkIdx) / float(stacks);
            for (int i = 0;  i < slices;  ++i) {
                grp->txtCoords[idx++] = glm::vec2(float(i+1) / float(slices), t);
            }
        }
        // instances of the north pole
        for (int i = 0;  i < slices;  ++i) {
            grp->txtCoords[idx++] = glm::vec2(float(i+1) / float(slices), 1.0);
        }
        assert (idx == nVerts && "incorrect number of vertices");
    }

    // generate the indices for the triangles
    idx = 0;

    // the triangles around the bottom stack
    for (int i = 0; i < slices; ++i) {
        uint32_t i0, i1, i2;
        if (hasTCs) {
            // the pole vertices are at indices [0 .. slices-1] and the first stack
            // vertices are at indices [slices .. 2*slices-1]
            i0 = i;  // south-pole vertex
            i1 = slices + i;
            i2 = slices + ((i + 1) % slices);
        } else {
            i0 = 0;
            i1 = i + 1;
            i2 = (i + 1) % slices + 1;
        }
        grp->indices[idx++] = i0;
        grp->indices[idx++] = i1;
        grp->indices[idx++] = i2;
    }

    // triangulate the quads for each stack from bottom to top
    for (int stkIdx = 1;  stkIdx < stacks - 1;  ++stkIdx) {
        uint32_t j0 = (stkIdx - 1) * slices + nPolarVerts;
        uint32_t j1 = stkIdx * slices + nPolarVerts;
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

    // the triangles around the top stack
    for (int i = 0; i < slices; ++i) {
        uint32_t i0, i1, i2;
        if (hasTCs) {
            // the pole vertices are at indices [nVerts-slices .. nVerts-1]
            // and the first stack vertices are at indices
            // [nverts-2*slices .. nVerts-slices-1]
            i0 = nVerts - slices + i; // north-pole vertex
            i1 = nVerts - 2*slices + i;
            i2 = nVerts - 2*slices + ((i + 1) % slices);
        } else {
            i0 = grp->nVerts - 1;
            i1 = i + slices * (stacks - 2) + 1;
            i2 = (i + 1) % slices + slices * (stacks - 2) + 1;
        }
        grp->indices[idx++] = i0;
        grp->indices[idx++] = i2;
        grp->indices[idx++] = i1;
    }

    return grp;

}

} // namespace cs237
} // namespace gobj
