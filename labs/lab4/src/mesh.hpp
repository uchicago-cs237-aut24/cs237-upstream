/*! \file mesh.hpp
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _MESH_HPP_
#define _MESH_HPP_

#include "cs237/cs237.hpp"

//! 3D vertices with normals and texture coordinates
struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 texCoord;
    glm::vec4 color;

    static std::vector<vk::VertexInputBindingDescription> getBindingDescriptions()
    {
        std::vector<vk::VertexInputBindingDescription> bindings(1);
        bindings[0].binding = 0;
        bindings[0].stride = sizeof(Vertex);
        bindings[0].inputRate = vk::VertexInputRate::eVertex;

        return bindings;
    }

    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions()
    {
        std::vector<vk::VertexInputAttributeDescription> attrs(4);

        // pos
        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = vk::Format::eR32G32B32Sfloat;
        attrs[0].offset = offsetof(Vertex, pos);
        // norm
        attrs[1].binding = 0;
        attrs[1].location = 1;
        attrs[1].format = vk::Format::eR32G32B32Sfloat;
        attrs[1].offset = offsetof(Vertex, norm);
        // texCoord
        attrs[2].binding = 0;
        attrs[2].location = 2;
        attrs[2].format = vk::Format::eR32G32Sfloat;
        attrs[2].offset = offsetof(Vertex, texCoord);
        // color
        attrs[3].binding = 0;
        attrs[3].location = 3;
        attrs[3].format = vk::Format::eR32G32B32A32Sfloat;
        attrs[3].offset = offsetof(Vertex, color);

        return attrs;
    }

    Vertex ()
      : pos(glm::vec3(0.0f)), norm(glm::vec3(0.0f)),
        texCoord(glm::vec2(0.0f)), color(glm::vec4(0.0f))
    { }
    Vertex (Vertex const &v)
      : pos(v.pos), norm(v.norm), texCoord(v.texCoord), color(v.color)
    { }
    Vertex (glm::vec3 p, glm::vec3 n, glm::vec2 tc, glm::vec4 c)
      : pos(p), norm(n), texCoord(tc), color(c)
    { }
};

//! the raw mesh data
struct Mesh {
    std::vector<Vertex> verts;
    std::vector<uint16_t> indices;
    cs237::Image2D *image;

    explicit Mesh ();
    ~Mesh () { delete this->image; }

};

#endif // !_MESH_HPP_
