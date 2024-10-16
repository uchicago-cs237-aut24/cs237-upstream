/*! \file vertex.hpp
 *
 * CS23740 Autumn 2024 Sample Code for Project 1
 *
 * The vertex representation for the vertex buffer used to represent
 * meshes.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (http://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _VERTEX_HPP_
#define _VERTEX_HPP_

#include "cs237/cs237.hpp"

/*! The locations of the standard mesh attributes.  The layout directives in the shaders
 * should match these values.
 */
constexpr int kCoordAttrLoc = 0;        //!< location of vertex coordinates attribute
constexpr int kNormAttrLoc = 1;         //!< location of normal-vector attribute
constexpr int kNumVertexAttrs = 2;      //!< number of vertex attributes

//! 3D mesh vertices with normals, texture coordinates, and bitangent vectors
//
struct Vertex {
    glm::vec3 pos;      //! vertex position
    glm::vec3 norm;     //! vertex normal

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
        std::vector<vk::VertexInputAttributeDescription> attrs(kNumVertexAttrs);

        // pos
        attrs[kCoordAttrLoc].binding = 0;
        attrs[kCoordAttrLoc].location = kCoordAttrLoc;
        attrs[kCoordAttrLoc].format = vk::Format::eR32G32B32Sfloat;
        attrs[kCoordAttrLoc].offset = offsetof(Vertex, pos);

        // norm
        attrs[kNormAttrLoc].binding = 0;
        attrs[kNormAttrLoc].location = kNormAttrLoc;
        attrs[kNormAttrLoc].format = vk::Format::eR32G32B32Sfloat;
        attrs[kNormAttrLoc].offset = offsetof(Vertex, norm);

        return attrs;
    }

    /// return a printable representation of the vertex
    std::string to_string () const
    {
        return "{p=" + glm::to_string(this->pos)
            + "; n=" + glm::to_string(this->norm) + "}";
    }

};

#endif // !_VERTEX_HPP_
