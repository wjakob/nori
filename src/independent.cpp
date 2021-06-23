/*
    This file is part of Nori, a simple educational ray tracer

    Copyright (c) 2015 by Wenzel Jakob
*/

#include <nori/sampler.h>
#include <nori/block.h>
#include <pcg32.h>

NORI_NAMESPACE_BEGIN

/**
 * Independent sampling - returns independent uniformly distributed
 * random numbers on <tt>[0, 1)x[0, 1)</tt>.
 *
 * This class is essentially just a wrapper around the pcg32 pseudorandom
 * number generator. For more details on what sample generators do in
 * general, refer to the \ref Sampler class.
 */
class Independent : public Sampler {
public:
    Independent(const PropertyList &propList) {
        m_sampleCount = (size_t) propList.getInteger("sampleCount", 1);
    }

    virtual ~Independent() { }

    std::unique_ptr<Sampler> clone() const {
        std::unique_ptr<Independent> cloned(new Independent());
        cloned->m_sampleCount = m_sampleCount;
        cloned->m_random = m_random;
        return std::move(cloned);
    }

    void prepare(const ImageBlock &block) {
        m_random.seed(
            block.getOffset().x(),
            block.getOffset().y()
        );
    }

    void generate() { /* No-op for this sampler */ }
    void advance()  { /* No-op for this sampler */ }

    float next1D() {
        return m_random.nextFloat();
    }
    
    Point2f next2D() {
        return Point2f(
            m_random.nextFloat(),
            m_random.nextFloat()
        );
    }

    std::string toString() const {
        return tfm::format("Independent[sampleCount=%i]", m_sampleCount);
    }
protected:
    Independent() { }

private:
    pcg32 m_random;
};

NORI_REGISTER_CLASS(Independent, "independent");
NORI_NAMESPACE_END
