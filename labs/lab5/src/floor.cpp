/*! \file floor.cpp
 *
 * CMSC 23740 Autumn 2024 Lab 5.  This file defines the vertex data for
 * the floor.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "quad.hpp"
#include "mesh.hpp"

#ifdef CS237_SOURCE_DIR
//!< the absolute path to the directory containing the compiled shaders
const std::string kDataDir = CS237_SOURCE_DIR "/labs/lab5/data/";
#else
# error CS237_SOURCE_DIR not defined
#endif

constexpr glm::vec3 kLightGreen(0.5f, 0.5f, 0.8f);

constexpr float kTileWid = 7.0;
constexpr int kGridWid = 3; // width in tiles (4x4 grid)
constexpr int kNTiles = kGridWid*kGridWid;
constexpr float kWidth = kTileWid * float(kGridWid);

// vertices and indices per tile
constexpr int kNVertsPerTile = 4;
constexpr int kNIndicesPerTile = 6;

// 1x1 square with texture coords in CCW order
static Quad tile = {
        glm::vec3{  0.0f, 1.0f,  0.0f },
        {   glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 0.0f, 0.0f, 1.0f },
            glm::vec3{ 1.0f, 0.0f, 1.0f },
            glm::vec3{ 1.0f, 0.0f, 0.0f },
        },
        {   glm::vec2{  0.0f,  0.0f },
            glm::vec2{  0.0f,  1.0f },
            glm::vec2{  1.0f,  1.0f },
            glm::vec2{  1.0f,  0.0f },
        }
    };

// add the vertices and indices for a tile with upper-left corner at <x, y>
static void addVerts (Mesh *m, int tileIdx, float x, float z)
{
    for (int i = 0;  i < kNVertsPerTile;  ++i) {
        glm::vec3 pos = glm::vec3(x, 0.0, z) + kTileWid*tile.corner[i];
        // we rotate the texture coordinates to reduce the obvious
        // patterns
        glm::vec2 tc = tile.texCoord[(i + tileIdx) % kNVertsPerTile];
        m->verts.push_back(Vertex(pos, tile.norm, tc));
    }

    // initialize the indices
    for (auto i : Quad::indices) {
        m->indices.push_back(kNVertsPerTile*tileIdx + i);
    }
}

//! return the mesh object for the ground/floor
Mesh *Mesh::floor ()
{
    Mesh *mesh = new Mesh;

    mesh->color = kLightGreen;
    mesh->image = new cs237::Image2D(kDataDir + "floor-tex.png");
    mesh->toWorld = glm::translate(glm::vec3(0.0,-1.0,0.0));

    // reserve space
    mesh->verts.reserve (kNVertsPerTile * kNTiles);
    mesh->indices.reserve(kNIndicesPerTile*kNTiles);

    // initialize the vertices and indices
    float y = -0.5 * kWidth;
    for (int r = 0;  r < kGridWid;  ++r) {
        float x = -0.5 * kWidth;
        for (int c = 0;  c < kGridWid;  ++c) {
            // add the vertices for the (r, c) tile
            addVerts (mesh, r*kGridWid+c, x, y);
            x += kTileWid;
        }
        y += kTileWid;
    }

    return mesh;

}
