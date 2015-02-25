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

#if !defined(__NORI_WARP_H)
#define __NORI_WARP_H

#include <nori/common.h>
#include <nori/sampler.h>

NORI_NAMESPACE_BEGIN

/// A collection of useful warping functions for importance sampling
class Warp {
public:
    /// Uniformly sample a vector on the unit hemisphere with respect to solid angles (naive implementation)
    static Vector3f sampleUniformHemisphere(Sampler *sampler, const Normal3f &northPole);

    /// Dummy warping function: takes uniformly distributed points in a square and just returns them
    static Point2f squareToUniformSquare(const Point2f &sample);

    /// Probability density of \ref squareToUniformSquare()
    static float squareToUniformSquarePdf(const Point2f &p);

    /// Uniformly sample a vector on a 2D disk with radius 1, centered around the origin
    static Point2f squareToUniformDisk(const Point2f &sample);

    /// Probability density of \ref squareToUniformDisk()
    static float squareToUniformDiskPdf(const Point2f &p);

    /// Uniformly sample a vector on the unit sphere with respect to solid angles
    static Vector3f squareToUniformSphere(const Point2f &sample);

    /// Probability density of \ref squareToUniformSphere()
    static float squareToUniformSpherePdf(const Vector3f &v);

    /**
     * \brief Uniformly sample a vector on a spherical cap around (0, 0, 1)
     *
     * A spherical cap is the subset of a unit sphere whose directions
     * make an angle of less than 'theta' with the north pole. This function
     * expects the cosine of 'theta' as a parameter.
     */
    static Vector3f squareToUniformSphereCap(const Point2f &sample, float cosThetaMax);

    /// Probability density of \ref squareToUniformSphereCap()
    static float squareToUniformSphereCapPdf(const Vector3f &v, float cosThetaMax);

    /// Uniformly sample a vector on the unit hemisphere around the pole (0,0,1) with respect to solid angles
    static Vector3f squareToUniformHemisphere(const Point2f &sample);

    /// Probability density of \ref squareToUniformHemisphere()
    static float squareToUniformHemispherePdf(const Vector3f &v);

    /// Uniformly sample a vector on the unit hemisphere around the pole (0,0,1) with respect to projected solid angles
    static Vector3f squareToCosineHemisphere(const Point2f &sample);

    /// Probability density of \ref squareToCosineHemisphere()
    static float squareToCosineHemispherePdf(const Vector3f &v);

    /// Warp a uniformly distributed square sample to a Beckmann distribution * cosine for the given 'alpha' parameter
    static Vector3f squareToBeckmann(const Point2f &sample, float alpha);

    /// Probability density of \ref squareToBeckmann()
    static float squareToBeckmannPdf(const Vector3f &m, float alpha);
};

NORI_NAMESPACE_END

#endif /* __NORI_WARP_H */
