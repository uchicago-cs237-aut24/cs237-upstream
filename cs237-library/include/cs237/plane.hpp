/*! \file plane.hpp
 *
 * Representation of a plane in 3D space.
 *
 * \author John Reppy
 */

/* COPYRIGHT (c) 2024 John Reppy (https://cs.uchicago.edu/~jhr)
 * All rights reserved.
 */

#ifndef _CS237_PLANE_HPP_
#define _CS237_PLANE_HPP_

#ifndef _CS237_HPP_
#error "cs237-plane.hpp should not be included directly"
#endif

namespace cs237 {

namespace __detail {

/// representation of a oriented 3D plane
template <typename T>
struct Plane {
    using vec3 = glm::vec<3, T, glm::defaultp>;
    using vec4 = glm::vec<4, T, glm::defaultp>;

    vec4 _nd;     /// unit normal and signed distance from origin

    Plane () { }

    /// \brief specify a plane as a unit-length normal vector and
    ///        signed distance from origin
    /// \param n unit-length plane normal vector
    /// \param d signed distance from origin to the plane
    Plane (vec3 n, T d) : _nd(vec4(n, T(d))) { }

    /// \brief specify a plane as a normal vector and point on the plane
    /// \param n plane-normal vector (does not have to be unit length)
    /// \param p a point on the plane
    Plane (vec3 n, vec3 p)
    {
        vec3 norm = glm::normalize(n);
        double d = -glm::dot(norm, p);
        this->_nd = vec4(norm, d);
    }

    /// \brief get the plane normal vector
    vec3 norm () const { return vec3(this->_nd); }

    /// \brief signed distance from origin to plane
    T dist () const { return this->_nd.w; }

    /// \brief signed distance from a point to plane
    /// \param p a point in space
    /// \return the signed distance from `p` to the plane
    T distanceToPt (vec3 const &p) const
    {
	return glm::dot(this->_nd, vec4(p, 1.0));
    }

    /// project a point onto the plane
    /// \param p a point in space
    /// \return the projection of the point `p` onto the plane.
    vec3 project (vec3 const &p) const
    {
        return p - this->distanceToPt(p) * this->norm();
    }

    /// the string representation of a plane
    std::string toString () const
    {
        return "Plane(" + glm::to_string(this->norm()) + ", "
            + to_string(this->dist()) + ")";
    }

};

/***** Output *****/

template <typename T>
inline std::ostream& operator<< (std::ostream& s, Plane<T> const &plane)
{
    return (s << plane.toString());
}

} // namespace __detail

/// Single-precision planes
using Planef_t = __detail::Plane<float>;
/// Double-precision planes
using Planed_t = __detail::Plane<double>;

} // namespace cs237

#endif // !_CS237_PLANE_HPP_
