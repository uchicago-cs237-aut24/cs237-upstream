/*! \file crate.cpp
 *
 * CMSC 23740 Autumn 2024 Lab 5.  This file defines the vertex data for
 * the "crate".
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
/// location of the textures for Lab 5
const std::string kDataDir = CS237_SOURCE_DIR "/labs/lab5/data/";
#else
# error CS237_SOURCE_DIR not defined
#endif

constexpr glm::vec3 kOrange(1.0f, 0.52f, 0.0f);

/* The vertices for the crate in CCW order */
static Quad sides[6] = {
    { // left
        glm::vec3{ -1.0f,  0.0f,  0.0f },       // norm
        {   glm::vec3{ -1.5f,  1.5f, -1.5f },   // corner[0]
            glm::vec3{ -1.5f, -1.5f, -1.5f },
            glm::vec3{ -1.5f, -1.5f,  1.5f },
            glm::vec3{ -1.5f,  1.5f,  1.5f },
        },
        {   glm::vec2{  0.0f,  1.0f},
            glm::vec2{  0.0f,  0.0f},
            glm::vec2{  1.0f,  0.0f},
            glm::vec2{  1.0f,  1.0f}
        }
    },
    { // right
        glm::vec3{  1.0f,  0.0f,  0.0f },
        {   glm::vec3{  1.5f,  1.5f,  1.5f },
            glm::vec3{  1.5f, -1.5f,  1.5f },
            glm::vec3{  1.5f, -1.5f, -1.5f },
            glm::vec3{  1.5f,  1.5f, -1.5f },
        },
        {   glm::vec2{  0.0f,  1.0f},
            glm::vec2{  0.0f,  0.0f},
            glm::vec2{  1.0f,  0.0f},
            glm::vec2{  1.0f,  1.0f}
        }
    },
    { // top
        glm::vec3{  0.0f,  1.0f,  0.0f },
        {   glm::vec3{ -1.5f,  1.5f, -1.5f },
            glm::vec3{ -1.5f,  1.5f,  1.5f },
            glm::vec3{  1.5f,  1.5f,  1.5f },
            glm::vec3{  1.5f,  1.5f, -1.5f },
        },
        {   glm::vec2{  0.0f,  0.5f},
            glm::vec2{  0.5f,  0.0f},
            glm::vec2{  1.0f,  0.5f},
            glm::vec2{  0.5f,  1.0f}
        }
    },
    { // bottom
        glm::vec3{  0.0f, -1.0f,  0.0f },
        {   glm::vec3{  1.5f, -1.5f, -1.5f },
            glm::vec3{  1.5f, -1.5f,  1.5f },
            glm::vec3{ -1.5f, -1.5f,  1.5f },
            glm::vec3{ -1.5f, -1.5f, -1.5f },
        },
        {   glm::vec2{  0.0f,  0.5f},
            glm::vec2{  0.5f,  0.0f},
            glm::vec2{  1.0f,  0.5f},
            glm::vec2{  0.5f,  1.0f}
        }
    },
    { // back
        glm::vec3{  0.0f,  0.0f, -1.0f },
        {   glm::vec3{  1.5f,  1.5f, -1.5f },
            glm::vec3{  1.5f, -1.5f, -1.5f },
            glm::vec3{ -1.5f, -1.5f, -1.5f },
            glm::vec3{ -1.5f,  1.5f, -1.5f },
        },
        {   glm::vec2{  0.0f,  1.0f},
            glm::vec2{  0.0f,  0.0f},
            glm::vec2{  1.0f,  0.0f},
            glm::vec2{  1.0f,  1.0f}
        }
    },
    { // front
        glm::vec3{  0.0f,  0.0f,  1.0f },
        {   glm::vec3{ -1.5f,  1.5f,  1.5f },
            glm::vec3{ -1.5f, -1.5f,  1.5f },
            glm::vec3{  1.5f, -1.5f,  1.5f },
            glm::vec3{  1.5f,  1.5f,  1.5f },
        },
        {   glm::vec2{  0.0f,  1.0f},
            glm::vec2{  0.0f,  0.0f},
            glm::vec2{  1.0f,  0.0f},
            glm::vec2{  1.0f,  1.0f}
        }
    }
};

//! return the mesh object for the crate.
Mesh *Mesh::crate ()
{
    Mesh *mesh = new Mesh;

    mesh->color = kOrange;
    mesh->image = new cs237::Image2D(kDataDir + "crate-tex.png");
    mesh->toWorld = glm::translate(glm::vec3(0.0, 0.0, 0.0));

    // initialize the vertices
    mesh->verts.reserve (6*4);
    for (int w = 0;  w < 6;  ++w) {
        for (int i = 0;  i < 4;  ++i) {
            mesh->verts.push_back(
                Vertex(sides[w].corner[i], sides[w].norm, sides[w].texCoord[i]));
        }
    }

    // initialize the indices
    mesh->indices.reserve(6*6);
    for (int w = 0;  w < 6;  ++w) {
        for (auto i : Quad::indices) {
            mesh->indices.push_back(4*w + i);
        }
    }

    return mesh;

}
