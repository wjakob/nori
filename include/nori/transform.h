/*
    This file is part of Nori, a simple educational ray tracer

    Copyright (c) 2015 by Wenzel Jakob

    Nori is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License Version 3
    as published by the Free Software Foundation.

    Nori is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <nori/common.h>
#include <nori/ray.h>

NORI_NAMESPACE_BEGIN

/**
 * \brief Homogeneous coordinate transformation
 *
 * This class stores a general homogeneous coordinate tranformation, such as
 * rotation, translation, uniform or non-uniform scaling, and perspective
 * transformations. The inverse of this transformation is also recorded
 * here, since it is required when transforming normal vectors.
 */
struct Transform {
public:
    /// Create the identity transform
    Transform() : 
        m_transform(Eigen::Matrix4f::Identity()),
        m_inverse(Eigen::Matrix4f::Identity()) { }

    /// Create a new transform instance for the given matrix 
    Transform(const Eigen::Matrix4f &trafo);

    /// Create a new transform instance for the given matrix and its inverse
    Transform(const Eigen::Matrix4f &trafo, const Eigen::Matrix4f &inv) 
        : m_transform(trafo), m_inverse(inv) { }

    /// Return the underlying matrix
    const Eigen::Matrix4f &getMatrix() const {
        return m_transform;
    }

    /// Return the inverse of the underlying matrix
    const Eigen::Matrix4f &getInverseMatrix() const {
        return m_inverse;
    }

    /// Return the inverse transformation
    Transform inverse() const {
        return Transform(m_inverse, m_transform);
    }

    /// Concatenate with another transform
    Transform operator*(const Transform &t) const;

    /// Apply the homogeneous transformation to a 3D vector
    Vector3f operator*(const Vector3f &v) const {
        return m_transform.topLeftCorner<3,3>() * v;
    }

    /// Apply the homogeneous transformation to a 3D normal
    Normal3f operator*(const Normal3f &n) const {
        return m_inverse.topLeftCorner<3, 3>().transpose() * n;
    }

    /// Transform a point by an arbitrary matrix in homogeneous coordinates
    Point3f operator*(const Point3f &p) const {
        Vector4f result = m_transform * Vector4f(p[0], p[1], p[2], 1.0f);
        return result.head<3>() / result.w();
    }

    /// Apply the homogeneous transformation to a ray
    Ray3f operator*(const Ray3f &r) const {
        return Ray3f(
            operator*(r.o), 
            operator*(r.d), 
            r.mint, r.maxt
        );
    }

    /// Return a string representation
    std::string toString() const;
private:
    Eigen::Matrix4f m_transform;
    Eigen::Matrix4f m_inverse;
};

NORI_NAMESPACE_END
