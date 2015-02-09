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

NORI_NAMESPACE_BEGIN

/* ===================================================================
    This file contains a few templates and specializations, which
    provide 2/3D points, vectors, and normals over different
    underlying data types. Points, vectors, and normals are distinct
    in Nori, because they transform differently under homogeneous
    coordinate transformations.
 * =================================================================== */

/**
 * \brief Generic N-dimensional vector data structure based on Eigen::Matrix
 */
template <typename _Scalar, int _Dimension> struct TVector : public Eigen::Matrix<_Scalar, _Dimension, 1> {
public:
    enum {
        Dimension = _Dimension
    };

    typedef _Scalar                             Scalar;
    typedef Eigen::Matrix<Scalar, Dimension, 1> Base;
    typedef TVector<Scalar, Dimension>          VectorType;
    typedef TPoint<Scalar, Dimension>           PointType;

    /// Create a new vector with constant component vlaues
    TVector(Scalar value = (Scalar) 0) { Base::setConstant(value); }

    /// Create a new 2D vector (type error if \c Dimension != 2)
    TVector(Scalar x, Scalar y) : Base(x, y) { }

    /// Create a new 3D vector (type error if \c Dimension != 3)
    TVector(Scalar x, Scalar y, Scalar z) : Base(x, y, z) { }

    /// Create a new 4D vector (type error if \c Dimension != 4)
    TVector(Scalar x, Scalar y, Scalar z, Scalar w) : Base(x, y, z, w) { }

    /// Construct a vector from MatrixBase (needed to play nice with Eigen)
    template <typename Derived> TVector(const Eigen::MatrixBase<Derived>& p)
        : Base(p) { }

    /// Assign a vector from MatrixBase (needed to play nice with Eigen)
    template <typename Derived> TVector &operator=(const Eigen::MatrixBase<Derived>& p) {
        this->Base::operator=(p);
        return *this;
    }

    /// Return a human-readable string summary
    std::string toString() const {
        std::string result;
        for (size_t i=0; i<Dimension; ++i) {
            result += std::to_string(this->coeff(i));
            if (i+1 < Dimension)
                result += ", ";
        }
        return "[" + result + "]";
    }
};

/**
 * \brief Generic N-dimensional point data structure based on Eigen::Matrix
 */
template <typename _Scalar, int _Dimension> struct TPoint : public Eigen::Matrix<_Scalar, _Dimension, 1> {
public:
    enum {
        Dimension = _Dimension
    };

    typedef _Scalar                             Scalar;
    typedef Eigen::Matrix<Scalar, Dimension, 1> Base;
    typedef TVector<Scalar, Dimension>          VectorType;
    typedef TPoint<Scalar, Dimension>           PointType;

    /// Create a new point with constant component vlaues
    TPoint(Scalar value = (Scalar) 0) { Base::setConstant(value); }

    /// Create a new 2D point (type error if \c Dimension != 2)
    TPoint(Scalar x, Scalar y) : Base(x, y) { }

    /// Create a new 3D point (type error if \c Dimension != 3)
    TPoint(Scalar x, Scalar y, Scalar z) : Base(x, y, z) { }

    /// Create a new 4D point (type error if \c Dimension != 4)
    TPoint(Scalar x, Scalar y, Scalar z, Scalar w) : Base(x, y, z, w) { }

    /// Construct a point from MatrixBase (needed to play nice with Eigen)
    template <typename Derived> TPoint(const Eigen::MatrixBase<Derived>& p)
        : Base(p) { }

    /// Assign a point from MatrixBase (needed to play nice with Eigen)
    template <typename Derived> TPoint &operator=(const Eigen::MatrixBase<Derived>& p) {
        this->Base::operator=(p);
        return *this;
    }

    /// Return a human-readable string summary
    std::string toString() const {
        std::string result;
        for (size_t i=0; i<Dimension; ++i) {
            result += std::to_string(this->coeff(i));
            if (i+1 < Dimension)
                result += ", ";
        }
        return "[" + result + "]";
    }
};

/**
 * \brief 3-dimensional surface normal representation
 */
struct Normal3f : public Eigen::Matrix<float, 3, 1> {
public:
    enum {
        Dimension = 3
    };

    typedef float                               Scalar;
    typedef Eigen::Matrix<Scalar, Dimension, 1> Base;
    typedef TVector<Scalar, Dimension>          VectorType;
    typedef TPoint<Scalar, Dimension>           PointType;


    /// Create a new normal with constant component vlaues
    Normal3f(Scalar value = 0.0f) { Base::setConstant(value); }

    /// Create a new 3D normal
    Normal3f(Scalar x, Scalar y, Scalar z) : Base(x, y, z) { }

    /// Construct a normal from MatrixBase (needed to play nice with Eigen)
    template <typename Derived> Normal3f(const Eigen::MatrixBase<Derived>& p)
        : Base(p) { }

    /// Assign a normal from MatrixBase (needed to play nice with Eigen)
    template <typename Derived> Normal3f &operator=(const Eigen::MatrixBase<Derived>& p) {
        this->Base::operator=(p);
        return *this;
    }

    /// Return a human-readable string summary
    std::string toString() const {
        return tfm::format("[%f, %f, %f]", coeff(0), coeff(1), coeff(2));
    }
};

/// Complete the set {a} to an orthonormal base
extern void coordinateSystem(const Vector3f &a, Vector3f &b, Vector3f &c);

NORI_NAMESPACE_END
