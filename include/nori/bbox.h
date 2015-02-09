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

#include <nori/ray.h>

NORI_NAMESPACE_BEGIN

/**
 * \brief Generic n-dimensional bounding box data structure
 *
 * Maintains a minimum and maximum position along each dimension and provides
 * various convenience functions for querying and modifying them.
 *
 * This class is parameterized by the underlying point data structure,
 * which permits the use of different scalar types and dimensionalities, e.g.
 * \code
 * TBoundingBox<Vector3i> integerBBox(Point3i(0, 1, 3), Point3i(4, 5, 6));
 * TBoundingBox<Vector2d> doubleBBox(Point2d(0.0, 1.0), Point2d(4.0, 5.0));
 * \endcode
 *
 * \tparam T The underlying point data type (e.g. \c Point2d)
 * \ingroup libcore
 */
template <typename _PointType> struct TBoundingBox {
    enum {
        Dimension = _PointType::Dimension
    };

    typedef _PointType                             PointType;
    typedef typename PointType::Scalar             Scalar;
    typedef typename PointType::VectorType         VectorType;

    /** 
     * \brief Create a new invalid bounding box
     * 
     * Initializes the components of the minimum 
     * and maximum position to \f$\infty\f$ and \f$-\infty\f$,
     * respectively.
     */
    TBoundingBox() {
        reset();
    }

    /// Create a collapsed bounding box from a single point
    TBoundingBox(const PointType &p) 
        : min(p), max(p) { }

    /// Create a bounding box from two positions
    TBoundingBox(const PointType &min, const PointType &max)
        : min(min), max(max) {
    }

    /// Test for equality against another bounding box
    bool operator==(const TBoundingBox &bbox) const {
        return min == bbox.min && max == bbox.max;
    }

    /// Test for inequality against another bounding box
    bool operator!=(const TBoundingBox &bbox) const {
        return min != bbox.min || max != bbox.max;
    }

    /// Calculate the n-dimensional volume of the bounding box
    Scalar getVolume() const {
        return (max - min).prod();
    }

    /// Calculate the n-1 dimensional volume of the boundary
    float getSurfaceArea() const {
        VectorType d = max - min;
        float result = 0.0f;
        for (int i=0; i<Dimension; ++i) {
            float term = 1.0f;
            for (int j=0; j<Dimension; ++j) {
                if (i == j)
                    continue;
                term *= d[j];
            }
            result += term;
        }
        return 2.0f * result;
    }

    /// Return the center point
    PointType getCenter() const {
        return (max + min) * (Scalar) 0.5f;
    }

    /**
     * \brief Check whether a point lies \a on or \a inside the bounding box
     *
     * \param p The point to be tested
     *
     * \param strict Set this parameter to \c true if the bounding
     *               box boundary should be excluded in the test
     */
    bool contains(const PointType &p, bool strict = false) const {
        if (strict) {
            return (p.array() > min.array()).all() 
                && (p.array() < max.array()).all();
        } else {
            return (p.array() >= min.array()).all() 
                && (p.array() <= max.array()).all();
        }
    }

    /**
     * \brief Check whether a specified bounding box lies \a on or \a within 
     * the current bounding box
     *
     * Note that by definition, an 'invalid' bounding box (where min=\f$\infty\f$
     * and max=\f$-\infty\f$) does not cover any space. Hence, this method will always 
     * return \a true when given such an argument.
     *
     * \param strict Set this parameter to \c true if the bounding
     *               box boundary should be excluded in the test
     */
    bool contains(const TBoundingBox &bbox, bool strict = false) const {
        if (strict) {
            return (bbox.min.array() > min.array()).all() 
                && (bbox.max.array() < max.array()).all();
        } else {
            return (bbox.min.array() >= min.array()).all() 
                && (bbox.max.array() <= max.array()).all();
        }
    }

    /**
     * \brief Check two axis-aligned bounding boxes for possible overlap.
     *
     * \param strict Set this parameter to \c true if the bounding
     *               box boundary should be excluded in the test
     *
     * \return \c true If overlap was detected.
     */
    bool overlaps(const TBoundingBox &bbox, bool strict = false) const {
        if (strict) {
            return (bbox.min.array() < max.array()).all() 
                && (bbox.max.array() > min.array()).all();
        } else {
            return (bbox.min.array() <= max.array()).all() 
                && (bbox.max.array() >= min.array()).all();
        }
    }

    /**
     * \brief Calculate the smallest squared distance between
     * the axis-aligned bounding box and the point \c p.
     */
    Scalar squaredDistanceTo(const PointType &p) const {
        Scalar result = 0;

        for (int i=0; i<Dimension; ++i) {
            Scalar value = 0;
            if (p[i] < min[i])
                value = min[i] - p[i];
            else if (p[i] > max[i])
                value = p[i] - max[i];
            result += value*value;
        }

        return result;
    }

    /**
     * \brief Calculate the smallest distance between
     * the axis-aligned bounding box and the point \c p.
     */
    Scalar distanceTo(const PointType &p) const {
        return std::sqrt(squaredDistanceTo(p));
    }

    /**
     * \brief Calculate the smallest square distance between
     * the axis-aligned bounding box and \c bbox.
     */
    Scalar squaredDistanceTo(const TBoundingBox &bbox) const {
        Scalar result = 0;

        for (int i=0; i<Dimension; ++i) {
            Scalar value = 0;
            if (bbox.max[i] < min[i])
                value = min[i] - bbox.max[i];
            else if (bbox.min[i] > max[i])
                value = bbox.min[i] - max[i];
            result += value*value;
        }

        return result;
    }

    /**
     * \brief Calculate the smallest distance between
     * the axis-aligned bounding box and \c bbox.
     */
    Scalar distanceTo(const TBoundingBox &bbox) const {
        return std::sqrt(squaredDistanceTo(bbox));
    }

    /**
     * \brief Check whether this is a valid bounding box
     *
     * A bounding box \c bbox is valid when
     * \code
     * bbox.min[dim] <= bbox.max[dim]
     * \endcode
     * holds along each dimension \c dim.
     */
    bool isValid() const {
        return (max.array() >= min.array()).all();
    }

    /// Check whether this bounding box has collapsed to a single point
    bool isPoint() const {
        return (max.array() == min.array()).all();
    }

    /// Check whether this bounding box has any associated volume
    bool hasVolume() const {
        return (max.array() > min.array()).all();
    }

    /// Return the dimension index with the largest associated side length
    int getMajorAxis() const {
        VectorType d = max - min;
        int largest = 0;
        for (int i=1; i<Dimension; ++i)
            if (d[i] > d[largest])
                largest = i;
        return largest;
    }

    /// Return the dimension index with the shortest associated side length
    int getMinorAxis() const {
        VectorType d = max - min;
        int shortest = 0;
        for (int i=1; i<Dimension; ++i)
            if (d[i] < d[shortest])
                shortest = i;
        return shortest;
    }

    /**
     * \brief Calculate the bounding box extents
     * \return max-min
     */
    VectorType getExtents() const {
        return max - min;
    }

    /// Clip to another bounding box
    void clip(const TBoundingBox &bbox) {
        min = min.cwiseMax(bbox.min);
        max = max.cwiseMin(bbox.max);
    }

    /** 
     * \brief Mark the bounding box as invalid.
     * 
     * This operation sets the components of the minimum 
     * and maximum position to \f$\infty\f$ and \f$-\infty\f$,
     * respectively.
     */
    void reset() {
        min.setConstant( std::numeric_limits<Scalar>::infinity());
        max.setConstant(-std::numeric_limits<Scalar>::infinity());
    }

    /// Expand the bounding box to contain another point
    void expandBy(const PointType &p) {
        min = min.cwiseMin(p);
        max = max.cwiseMax(p);
    }

    /// Expand the bounding box to contain another bounding box
    void expandBy(const TBoundingBox &bbox) {
        min = min.cwiseMin(bbox.min);
        max = max.cwiseMax(bbox.max);
    }

    /// Merge two bounding boxes
    static TBoundingBox merge(const TBoundingBox &bbox1, const TBoundingBox &bbox2) {
        return TBoundingBox(
            bbox1.min.cwiseMin(bbox2.min),
            bbox1.max.cwiseMax(bbox2.max)
        );
    }

    /// Return the index of the largest axis
    int getLargestAxis() const {
        VectorType extents = max-min;

        if (extents[0] >= extents[1] && extents[0] >= extents[2])
            return 0;
        else if (extents[1] >= extents[0] && extents[1] >= extents[2])
            return 1;
        else
            return 2;
    }

    /// Return the position of a bounding box corner
    PointType getCorner(int index) const {
        PointType result;
        for (int i=0; i<Dimension; ++i)
            result[i] = (index & (1 << i)) ? max[i] : min[i];
        return result;
    }

    /// Return a string representation of the bounding box
    std::string toString() const {
        if (!isValid())
            return "BoundingBox[invalid]";
        else
            return tfm::format("BoundingBox[min=%s, max=%s]", min.toString(), max.toString());
    }

    /// Check if a ray intersects a bounding box
    bool rayIntersect(const Ray3f &ray) const {
        float nearT = -std::numeric_limits<float>::infinity();
        float farT = std::numeric_limits<float>::infinity();

        for (int i=0; i<3; i++) {
            float origin = ray.o[i];
            float minVal = min[i], maxVal = max[i];

            if (ray.d[i] == 0) {
                if (origin < minVal || origin > maxVal)
                    return false;
            } else {
                float t1 = (minVal - origin) * ray.dRcp[i];
                float t2 = (maxVal - origin) * ray.dRcp[i];

                if (t1 > t2)
                    std::swap(t1, t2);

                nearT = std::max(t1, nearT);
                farT = std::min(t2, farT);

                if (!(nearT <= farT))
                    return false;
            }
        }

        return ray.mint <= farT && nearT <= ray.maxt;
    }

    /// Return the overlapping region of the bounding box and an unbounded ray
    bool rayIntersect(const Ray3f &ray, float &nearT, float &farT) const {
        nearT = -std::numeric_limits<float>::infinity();
        farT = std::numeric_limits<float>::infinity();

        for (int i=0; i<3; i++) {
            float origin = ray.o[i];
            float minVal = min[i], maxVal = max[i];

            if (ray.d[i] == 0) {
                if (origin < minVal || origin > maxVal)
                    return false;
            } else {
                float t1 = (minVal - origin) * ray.dRcp[i];
                float t2 = (maxVal - origin) * ray.dRcp[i];

                if (t1 > t2)
                    std::swap(t1, t2);

                nearT = std::max(t1, nearT);
                farT = std::min(t2, farT);

                if (!(nearT <= farT))
                    return false;
            }
        }

        return true;
    }

    PointType min; ///< Component-wise minimum 
    PointType max; ///< Component-wise maximum 
};


NORI_NAMESPACE_END
