/*! \file mesh-data.cpp
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#include "mesh.hpp"

#ifdef CS237_SOURCE_DIR
//!< the path to the directory containing the texture data
const std::string kDataDir = CS237_SOURCE_DIR "/labs/lab4/data/";
#else
# error CS237_SOURCE_DIR  not defined
#endif

constexpr glm::vec4 kRed(1.0f, 0.0f, 0.0f, 0.0f);       ///< Red color
constexpr glm::vec4 kGreen(0.0f, 1.0f, 0.0f, 1.0f);     ///< Green color
constexpr glm::vec4 kYellow(1.0f, 1.0f, 0.0f, 1.0f);    ///< Yellow color
constexpr glm::vec4 kBlue(0.0f, 0.0f, 1.0f, 1.0f);      ///< Blue color
constexpr glm::vec4 kSkyBlue(0.0f, 1.0f, 1.0f, 1.0f);   ///< Sky Blue color
constexpr glm::vec4 kPink(1.0f, 0.0f, 0.5f, 1.0f);      ///< Pink color

//! a face of the box
struct Face {
  glm::vec4 color;              //!< uniform color for face
  glm::vec3 norm;               //!< normal vector for face
  glm::vec3 corners[4];         //!< the corners of the quad that defines the face
  glm::vec2 texCoords[4];       //!< texture coordinates for the corners
};

//! the five visible faces of the box; note that the faces
//! are oriented toward the inside of the box; i.e., the
//! normals point into the box.
Face  faces[6] = {
    { /* left (X = -1) */
        kRed,                              // face color
        glm::vec3{  1.0f,  0.0f,  0.0f },       // norm
        {   glm::vec3{ -1.0f,  1.0f, -1.0f },   // corner[0]
            glm::vec3{ -1.0f,  1.0f,  1.0f },
            glm::vec3{ -1.0f, -1.0f,  1.0f },
            glm::vec3{ -1.0f, -1.0f, -1.0f }
        },
        {   glm::vec2{  0.665f,  0.665f },      // face 5 texture coordinates
            glm::vec2{  0.334f,  0.665f },
            glm::vec2{  0.334f,  0.334f },
            glm::vec2{  0.665f,  0.334f }
        }
    },
    { /* right (X = 1) */
        kGreen,
        glm::vec3{ -1.0f,  0.0f,  0.0f },
        {   glm::vec3{  1.0f,  1.0f,  1.0f },
            glm::vec3{  1.0f,  1.0f, -1.0f },
            glm::vec3{  1.0f, -1.0f, -1.0f },
            glm::vec3{  1.0f, -1.0f,  1.0f }
        },
        {   glm::vec2{  1.0f,   0.665f },       // face 6
            glm::vec2{  0.667f, 0.665f },
            glm::vec2{  0.667f, 0.334f },
            glm::vec2{  1.0f,   0.334f }
        }
    },
    { /* top (Y = 1) */
        kYellow,
        glm::vec3{  0.0f, -1.0f,  0.0f },
        {   glm::vec3{ -1.0f,  1.0f,  1.0f },
            glm::vec3{ -1.0f,  1.0f, -1.0f },
            glm::vec3{  1.0f,  1.0f, -1.0f },
            glm::vec3{  1.0f,  1.0f,  1.0f }
        },
        {   glm::vec2{  0.0f,    0.665f },      // face 4
            glm::vec2{  0.0f,    0.334f },
            glm::vec2{  0.332f,  0.334f },
            glm::vec2{  0.332f,  0.665f }
        }
    },
    { /* bottom (Y = -1) */
        kBlue,
        glm::vec3{  0.0f,  1.0f,  0.0f },
        {   glm::vec3{ -1.0f, -1.0f, -1.0f },
            glm::vec3{ -1.0f, -1.0f,  1.0f },
            glm::vec3{  1.0f, -1.0f,  1.0f },
            glm::vec3{  1.0f, -1.0f, -1.0f }
        },
        {   glm::vec2{  0.334f,  0.332f },      // face 2
            glm::vec2{  0.334f,  0.0f },
            glm::vec2{  0.665f,  0.0f },
            glm::vec2{  0.665f,  0.332f }
        }
    },
    { /* back (Z = -1) */
        kSkyBlue,
        glm::vec3{  0.0f,  0.0f,  1.0f },
        {   glm::vec3{  1.0f,  1.0f, -1.0f },
            glm::vec3{ -1.0f,  1.0f, -1.0f },
            glm::vec3{ -1.0f, -1.0f, -1.0f },
            glm::vec3{  1.0f, -1.0f, -1.0f }
        },
        {   glm::vec2{  0.332f, 0.332f },         // face 1
            glm::vec2{  0.0f,   0.332f },
            glm::vec2{  0.0f,   0.0f,  },
            glm::vec2{  0.332f, 0.0f }
        }

    },
    { /* front (Z = 1) */
        kPink,
        glm::vec3{  0.0f,  0.0f, -1.0f },
        {   glm::vec3{ -1.0f,  1.0f,  1.0f },
            glm::vec3{  1.0f,  1.0f,  1.0f },
            glm::vec3{  1.0f, -1.0f,  1.0f },
            glm::vec3{ -1.0f, -1.0f,  1.0f }
        },
        {   glm::vec2{  0.667f,  0.332f },      // face 3
            glm::vec2{  1.0f,    0.332f },
            glm::vec2{  1.0f,    0.0f },
            glm::vec2{  0.667f,  0.0f }
        }
    }
};

Mesh::Mesh ()
{
    // the vertices
    this->verts.resize(4 * 6);
    for (int i = 0; i < 6; i++) {
        for (int j = 0;  j < 4;  j++) {
            this->verts[4*i + j] = Vertex(
                faces[i].corners[j],
                faces[i].norm,
                faces[i].texCoords[j],
                faces[i].color);
        }
    }

    // the indices are computed to draw each side as two separate
    // triangles (i.e., 6 indices per side)
    this->indices.resize(6 * 6);
    const uint32_t offsets[6] = {0, 1, 2, 2, 3, 0};
    for (int i = 0; i < 6; i++) {
        for (int j = 0;  j < 6;  j++) {
            this->indices[6*i + j] = 4*i + offsets[j];
        }
    }

    // load the texture image
    this->image = new cs237::Image2D (kDataDir + "cubetex.png", true);

}
