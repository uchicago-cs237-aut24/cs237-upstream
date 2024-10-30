/*! \file mesh.hpp
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _MESH_HPP_
#define _MESH_HPP_

#include "cs237/cs237.hpp"

/// 3D vertices with normals and texture coordinates
struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 texCoord;

    static std::vector<vk::VertexInputBindingDescription> getBindingDescriptions()
    {
        std::vector<vk::VertexInputBindingDescription> bindings(1);
        bindings[0].binding = 0;
        bindings[0].stride = sizeof(Vertex);
        bindings[0].inputRate = vk::VertexInputRate::eVertex;

        return bindings;
    }

    /// get the attribute descriptions for standard rendering a mesh
    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions()
    {
        std::vector<vk::VertexInputAttributeDescription> attrs(3);

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

        return attrs;
    }

    /// get the attribute description for when we are just fetching the position
    /// (i.e., in the shadow rendering pass)
    static std::vector<vk::VertexInputAttributeDescription> getVertexAttribute()
    {
        std::vector<vk::VertexInputAttributeDescription> attrs;
        attrs.push_back(
            vk::VertexInputAttributeDescription (
                0, /* location */
                0, /* binding */
                vk::Format::eR32G32B32Sfloat, /* format */
                offsetof(Vertex, pos))); /* offset */
        return attrs;
    }

    Vertex (glm::vec3 pt, glm::vec3 n, glm::vec2 tc)
      : pos(pt), norm(n), texCoord(tc)
    { }
};

/// the raw mesh data
struct Mesh {
    std::vector<Vertex> verts;          ///< vertices
    std::vector<uint16_t> indices;      ///< indices to render triangle list
    glm::vec3 color;                    ///< color for when there is no texture
    cs237::Image2D *image;              ///< texture image for mesh
    glm::mat4 toWorld;                  ///< model-view transform for the mesh
                                        ///  Note: for this lab, the `toWorld` transform
                                        ///  is assumed to be orthogonal, so we do not
                                        ///  need a separate transform for normals.

    explicit Mesh () : image(nullptr) { }
    ~Mesh ()
    {
        delete this->image;
    }

    /// does the mesh have an associated texture image?
    bool hasTexture () const { return (this->image != nullptr); }

    /// compute the world-space axis-aligned bounding box for the mesh
    cs237::AABBf_t bbox () const
    {
        cs237::AABBf_t bb;
        for (auto v : this->verts) {
            bb.addPt (glm::vec3(this->toWorld * glm::vec4(v.pos, 1.0f)));
        }
        return bb;
    }

    /// return the mesh object for the ground/floor
    static Mesh *floor ();

    /// return the mesh object for the crate.
    static Mesh *crate ();

};

#endif // !_MESH_HPP_
