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
    static Vector3f sampleUniformHemisphere(Sampler * sampler, const Normal3f & northPole);


    /* TBD: add more in programming assignment 2 */
};

NORI_NAMESPACE_END

#endif /* __NORI_WARP_H */
