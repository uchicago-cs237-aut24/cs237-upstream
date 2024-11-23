/*! \file cube.cpp
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

constexpr int kNumFaces = 6;
constexpr int kNumVerts = 8;
constexpr int kNumTris = 2 * kNumFaces;
constexpr int kNumIndices = 3 * kNumTris;

// helper function for initializing a face of the cube mesh
static void _initFace (
    OBJ::Group *grp,
    int face,
    glm::vec3 v1,
    glm::vec3 v2)
{
    // starting index into _verts vector
    int vert = 4 * face;

    // initialize the indices for drawing two triangles
    int ix = 2 * 3 * face;
    // triangle 1
    grp->indices[ix++] = vert;
    grp->indices[ix++] = vert+1;
    grp->indices[ix++] = vert+2;
    // triangle 2
    grp->indices[ix++] = vert;
    grp->indices[ix++] = vert+2;
    grp->indices[ix++] = vert+3;

    // face normal vector
    glm::vec3 v3 = cross(v1, v2);
    // lower-left corner of face
    glm::vec3 origin = -0.5 * (v1 + v2 - v3);

    // vertex positions in CCW order
    grp->verts[vert+0] = origin; // lower-left corner
    grp->verts[vert+1] = origin + v1; // lower-right
    grp->verts[vert+2] = origin + (v1 + v2); // upper-right
    grp->verts[vert+3] = origin + v2; // upper-left

    // normal vectors
    if (grp->norms != nullptr) {
        for (int i = 0;  i < 4;  ++i) {
            grp->norms[vert+i] = v3;
        }
    }

    // texture coordinates
    if (grp->txtCoords != nullptr) {
        grp->txtCoords[vert+0] = glm::vec2(0.0f, 0.0f); // lower-left corner
        grp->txtCoords[vert+1] = glm::vec2(1.0f, 0.0f); // lower-right
        grp->txtCoords[vert+2] = glm::vec2(1.0f, 1.0f); // upper-right
        grp->txtCoords[vert+3] = glm::vec2(0.0f, 1.0f); // upper-left
    }

}


OBJ::Group *cube (VertexAttrs attrs, glm::vec3 center, float width)
{
    if (width <= 0.0) {
        return nullptr;
    }

    bool hasNorms = hasNormals (attrs);
    bool hasTCs = hasTextureCoords(attrs);

    // allocate memory for the group
    OBJ::Group *grp = __detail::allocGroup (
        "cube",
        // if we have normals or texture coordinates,
        // then there are three vertices per corner
        ((hasNorms || hasTCs) ? 3 * kNumVerts : kNumVerts),
        kNumIndices,
        hasNorms,
        hasTCs);

    // first we construct a unit cube at the origin
    if (!hasNorms && !hasTCs) {
        // initialize the vertices
        grp->verts[0] = center + glm::vec3(-0.5, -0.5, -0.5);
        grp->verts[1] = center + glm::vec3(-0.5, -0.5,  0.5);
        grp->verts[2] = center + glm::vec3(-0.5,  0.5, -0.5);
        grp->verts[3] = center + glm::vec3(-0.5,  0.5,  0.5);
        grp->verts[4] = center + glm::vec3( 0.5, -0.5, -0.5);
        grp->verts[5] = center + glm::vec3( 0.5, -0.5,  0.5);
        grp->verts[6] = center + glm::vec3( 0.5,  0.5, -0.5);
        grp->verts[7] = center + glm::vec3( 0.5,  0.5,  0.5);

        // initialize the indices for the faces
        // bottom face: Y = -hw; verts = {0, 4, 5, 1}
        grp->indices[ 0] = 0; grp->indices[ 1] = 4; grp->indices[ 2] = 5;
        grp->indices[ 3] = 0; grp->indices[ 4] = 5; grp->indices[ 5] = 1;
        // top face: Y = hw; verts = {}
        grp->indices[ 6] = 0; grp->indices[ 7] = 4; grp->indices[ 8] = 5;
        grp->indices[ 9] = 0; grp->indices[10] = 5; grp->indices[11] = 1;
        // left face: X = -hw; verts = {}
        grp->indices[12] = 0; grp->indices[13] = 4; grp->indices[14] = 5;
        grp->indices[15] = 0; grp->indices[16] = 5; grp->indices[17] = 1;
        // right face: X = hw; verts = {}
        grp->indices[18] = 0; grp->indices[19] = 4; grp->indices[20] = 5;
        grp->indices[21] = 0; grp->indices[22] = 5; grp->indices[23] = 1;
        // back face: Z = -hw; verts = {}
        grp->indices[24] = 0; grp->indices[25] = 4; grp->indices[26] = 5;
        grp->indices[27] = 0; grp->indices[28] = 5; grp->indices[29] = 1;
        // front face: Z = hw; verts = {}
        grp->indices[30] = 0; grp->indices[31] = 4; grp->indices[32] = 5;
        grp->indices[33] = 0; grp->indices[34] = 5; grp->indices[35] = 1;
    } else {
        // vertices have face-dependent attributes
        _initFace (
            grp, 0,
            glm::vec3( 0.5f,  0.0f,  0.0f),
            glm::vec3( 0.0f,  0.5f,  0.0f));
        _initFace (
            grp, 1,
            glm::vec3(-0.5f,  0.0f,  0.0f),
            glm::vec3( 0.0f,  0.5f,  0.0f));
        _initFace (
            grp, 2,
            glm::vec3( 0.0f,  0.0f,  0.5f),
            glm::vec3( 0.0f,  0.5f,  0.0f));
        _initFace (
            grp, 3,
            glm::vec3( 0.0f,  0.0f, -0.5f),
            glm::vec3( 0.0f,  0.5f,  0.0f));
        _initFace (
            grp, 4,
            glm::vec3( 0.5f,  0.0f,  0.0f),
            glm::vec3( 0.0f,  0.0f, -0.5f));
        _initFace (
            grp, 5,
            glm::vec3( 0.5f,  0.0f,  0.0f),
            glm::vec3( 0.0f,  0.0f,  0.5f));
    }

    // scale the cube to the specifed width and move it
    __detail::scaleAndMove (width, center, grp);

    return grp;

}

} // namespace gobj
} // namespace cs237
