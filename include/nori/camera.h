/*
    This file is part of Nori, a simple educational ray tracer

    Copyright (c) 2015 by Wenzel Jakob
*/

#pragma once

#include <nori/object.h>

NORI_NAMESPACE_BEGIN

/**
 * \brief Generic camera interface
 * 
 * This class provides an abstract interface to cameras in Nori and
 * exposes the ability to sample their response function. By default, only
 * a perspective camera implementation exists, but you may choose to
 * implement other types (e.g. an environment camera, or a physically-based 
 * camera model that simulates the behavior actual lenses)
 */
class Camera : public NoriObject {
public:
    /**
     * \brief Importance sample a ray according to the camera's response function
     *
     * \param ray
     *    A ray data structure to be filled with a position 
     *    and direction value
     *
     * \param samplePosition
     *    Denotes the desired sample position on the film
     *    expressed in fractional pixel coordinates
     *
     * \param apertureSample
     *    A uniformly distributed 2D vector that is used to sample
     *    a position on the aperture of the sensor if necessary.
     *
     * \return
     *    An importance weight associated with the sampled ray.
     *    This accounts for the difference in the camera response
     *    function and the sampling density.
     */
    virtual Color3f sampleRay(Ray3f &ray,
        const Point2f &samplePosition,
        const Point2f &apertureSample) const = 0;

    /// Return the size of the output image in pixels
    const Vector2i &getOutputSize() const { return m_outputSize; }

    /// Return the camera's reconstruction filter in image space
    const ReconstructionFilter *getReconstructionFilter() const { return m_rfilter; }

    /**
     * \brief Return the type of object (i.e. Mesh/Camera/etc.) 
     * provided by this instance
     * */
    EClassType getClassType() const { return ECamera; }
protected:
    Vector2i m_outputSize;
    ReconstructionFilter *m_rfilter;
};

NORI_NAMESPACE_END
