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

#include <nori/rfilter.h>

NORI_NAMESPACE_BEGIN

/**
 * Windowed Gaussian filter with configurable extent
 * and standard deviation. Often produces pleasing 
 * results, but may introduce too much blurring.
 */
class GaussianFilter : public ReconstructionFilter {
public:
    GaussianFilter(const PropertyList &propList) {
        /* Half filter size */
        m_radius = propList.getFloat("radius", 2.0f);
        /* Standard deviation of the Gaussian */
        m_stddev = propList.getFloat("stddev", 0.5f);
    }

    float eval(float x) const {
        float alpha = -1.0f / (2.0f * m_stddev*m_stddev);
        return std::max(0.0f, 
            std::exp(alpha * x * x) - 
            std::exp(alpha * m_radius * m_radius));
    }

    std::string toString() const {
        return tfm::format("GaussianFilter[radius=%f, stddev=%f]", m_radius, m_stddev);
    }
protected:
    float m_stddev;
};

/**
 * Separable reconstruction filter by Mitchell and Netravali
 * 
 * D. Mitchell, A. Netravali, Reconstruction filters for computer graphics, 
 * Proceedings of SIGGRAPH 88, Computer Graphics 22(4), pp. 221-228, 1988.
 */
class MitchellNetravaliFilter : public ReconstructionFilter {
public:
    MitchellNetravaliFilter(const PropertyList &propList) {
        /* Filter size in pixels */
        m_radius = propList.getFloat("radius", 2.0f);
        /* B parameter from the paper */
        m_B = propList.getFloat("B", 1.0f / 3.0f);
        /* C parameter from the paper */
        m_C = propList.getFloat("C", 1.0f / 3.0f);
    }

    float eval(float x) const {
        x = std::abs(2.0f * x / m_radius);
        float x2 = x*x, x3 = x2*x;

        if (x < 1) {
            return 1.0f/6.0f * ((12-9*m_B-6*m_C)*x3 
                    + (-18+12*m_B+6*m_C) * x2 + (6-2*m_B));
        } else if (x < 2) {
            return 1.0f/6.0f * ((-m_B-6*m_C)*x3 + (6*m_B+30*m_C) * x2
                    + (-12*m_B-48*m_C)*x + (8*m_B + 24*m_C));
        } else {
            return 0.0f;
        }
    }

    std::string toString() const {
        return tfm::format("MitchellNetravaliFilter[radius=%f, B=%f, C=%f]", m_radius, m_B, m_C);
    }
protected:
    float m_B, m_C;
};

/// Tent filter 
class TentFilter : public ReconstructionFilter {
public:
    TentFilter(const PropertyList &) {
        m_radius = 1.0f;
    }

    float eval(float x) const {
        return std::max(0.0f, 1.0f - std::abs(x));
    }
    
    std::string toString() const {
        return "TentFilter[]";
    }
};

/// Box filter -- fastest, but prone to aliasing
class BoxFilter : public ReconstructionFilter {
public:
    BoxFilter(const PropertyList &) {
        m_radius = 0.5f;
    }

    float eval(float) const {
        return 1.0f;
    }
    
    std::string toString() const {
        return "BoxFilter[]";
    }
};

NORI_REGISTER_CLASS(GaussianFilter, "gaussian");
NORI_REGISTER_CLASS(MitchellNetravaliFilter, "mitchell");
NORI_REGISTER_CLASS(TentFilter, "tent");
NORI_REGISTER_CLASS(BoxFilter, "box");

NORI_NAMESPACE_END
