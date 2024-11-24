/*! \file gobjects.hpp
 *
 * Support code for CMSC 23740 Autumn 2024.
 *
 * This file defines various "graphical objects" that can be used to represent
 * light volumes and other things.  The objects are represented as OBJ::Group
 * structures.
 *
 * \author John Reppy
 */

/*
 * COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_GOBJECTS_HPP_
#define _CS237_GOBJECTS_HPP_

#ifndef _CS237_HPP_
#error "cs237/gobjects.hpp should not be included directly"
#endif

namespace OBJ {
    struct Group;
}

namespace cs237 {
namespace gobj {

/// bit mask for the various vertex attributes
constexpr uint32_t kVAttrPosBit = (1 << 0);     ///< vertex position (required)
constexpr uint32_t kVAttrNormBit = (1 << 1);    ///< normal vector attribute
constexpr uint32_t kVAttrTCBit = (1 << 2);      ///< texture coordinate attribute
constexpr uint32_t kVAttrTanBit = (1 << 3);     ///< tangent vector attribute


/// specifying the attributes of the generated object
//
enum class VertexAttrs : uint32_t {
    ePos = kVAttrPosBit,                                ///< position
    ePosNorm = kVAttrPosBit | kVAttrNormBit,            ///< position+normal
    ePosTex = kVAttrPosBit | kVAttrTCBit,               ///< position+texture coord
    ePosNormTex = kVAttrPosBit | kVAttrNormBit | kVAttrTCBit ///< all attributes
};

/// Does a vertex attribute specification include normals?
inline bool hasNormals (VertexAttrs attrs)
{
    return ((static_cast<uint32_t>(attrs) & kVAttrNormBit) != 0);
}

/// Does a vertex attribute specification include texture coordinates?
inline bool hasTextureCoords (VertexAttrs attrs)
{
    return ((static_cast<uint32_t>(attrs) & kVAttrTCBit) != 0);
}

/// construct an axis-aligned cube centered at the origin
/// \param center  the cube's center
/// \param width   the width of a side
/// \return a `OBJ::Group` object that holds the vertex data for the cube.  Returns
///         `nullptr` if the width is not greater than zero.
OBJ::Group *cube (VertexAttrs attrs, float center, float width);

/// create a mesh  to represent a sphere centered at the origin
/// \param attrs   specify the attributes of the sphere's vertices
/// \param center  the sphere's center
/// \param radius  the sphere's radius
/// \param slices  the number of horizontal triangles around a band; must be at
///                least 5.
/// \param stacks  the number of horizontal bands around the sphere; must be at
///                least 5.
/// \return a `OBJ::Group` object that holds the vertex data for the sphere.  Returns
///         `nullptr` if the radius is not greater than zero.
///
/// Note that the vertices of the mesh lie on the surface of the sphere.
OBJ::Group *sphere (
    VertexAttrs attrs,
    glm::vec3 center,
    float radius,
    uint32_t slices,
    uint32_t stacks);

/// creates a cone shaped triange mesh
/// \param attrs   specify the attributes of the cones's vertices
/// \param pos     the apex of the cone
/// \param dir     the direction in which the cone is pointing (from the apex
///                to the center of the base)
/// \param radius  the radius of the cone's base
/// \param height  the height of the cone (distance from base to apex)
/// \param slices  the number slices around the sides of the cone; must be at least 6
/// \param stacks  the number of segments between the base and the apex of the cone
/// \return a `OBJ::Group` object that holds the vertex data for the cone.  Returns
///         `nullptr` if the radius or height is not greater than zero.
///
/// Note that the vertices of the mesh lie on the surface of the cone.  To produce
/// a mesh that contains the cone, you need to scale the radius by
/// \f$\frac{1}{\cos(\frac{2\pi}{\mathit{slices}})}\f$.
OBJ::Group *cone (
    VertexAttrs attrs,
    glm::vec3 pos,
    glm::vec3 dir,
    float radius,
    float height,
    uint32_t slices,
    uint32_t stacks);

} // namespace gobj
} // namespace cs237

#endif // !_CS237_GOBJECTS_HPP_