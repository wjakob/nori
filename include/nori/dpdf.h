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

/**
 * \brief Discrete probability distribution
 * 
 * This data structure can be used to transform uniformly distributed
 * samples to a stored discrete probability distribution.
 * 
 * \ingroup libcore
 */
struct DiscretePDF {
public:
    /// Allocate memory for a distribution with the given number of entries
    explicit DiscretePDF(size_t nEntries = 0) {
        reserve(nEntries);
        clear();
    }

    /// Clear all entries
    void clear() {
        m_cdf.clear();
        m_cdf.push_back(0.0f);
        m_normalized = false;
    }

    /// Reserve memory for a certain number of entries
    void reserve(size_t nEntries) {
        m_cdf.reserve(nEntries+1);
    }

    /// Append an entry with the specified discrete probability
    void append(float pdfValue) {
        m_cdf.push_back(m_cdf[m_cdf.size()-1] + pdfValue);
    }

    /// Return the number of entries so far
    size_t size() const {
        return m_cdf.size()-1;
    }

    /// Access an entry by its index
    float operator[](size_t entry) const {
        return m_cdf[entry+1] - m_cdf[entry];
    }

    /// Have the probability densities been normalized?
    bool isNormalized() const {
        return m_normalized;
    }

    /**
     * \brief Return the original (unnormalized) sum of all PDF entries
     *
     * This assumes that \ref normalize() has previously been called
     */
    float getSum() const {
        return m_sum;
    }

    /**
     * \brief Return the normalization factor (i.e. the inverse of \ref getSum())
     *
     * This assumes that \ref normalize() has previously been called
     */
    float getNormalization() const {
        return m_normalization;
    }

    /**
     * \brief Normalize the distribution
     *
     * \return Sum of the (previously unnormalized) entries
     */
    float normalize() {
        m_sum = m_cdf[m_cdf.size()-1];
        if (m_sum > 0) {
            m_normalization = 1.0f / m_sum;
            for (size_t i=1; i<m_cdf.size(); ++i) 
                m_cdf[i] *= m_normalization;
            m_cdf[m_cdf.size()-1] = 1.0f;
            m_normalized = true;
        } else {
            m_normalization = 0.0f;
        }
        return m_sum;
    }

    /**
     * \brief %Transform a uniformly distributed sample to the stored distribution
     * 
     * \param[in] sampleValue
     *     An uniformly distributed sample on [0,1]
     * \return
     *     The discrete index associated with the sample
     */
    size_t sample(float sampleValue) const {
        std::vector<float>::const_iterator entry = 
                std::lower_bound(m_cdf.begin(), m_cdf.end(), sampleValue);
        size_t index = (size_t) std::max((ptrdiff_t) 0, entry - m_cdf.begin() - 1);
        return std::min(index, m_cdf.size()-2);
    }

    /**
     * \brief %Transform a uniformly distributed sample to the stored distribution
     * 
     * \param[in] sampleValue
     *     An uniformly distributed sample on [0,1]
     * \param[out] pdf
     *     Probability value of the sample
     * \return
     *     The discrete index associated with the sample
     */
    size_t sample(float sampleValue, float &pdf) const {
        size_t index = sample(sampleValue);
        pdf = operator[](index);
        return index;
    }

    /**
     * \brief %Transform a uniformly distributed sample to the stored distribution
     * 
     * The original sample is value adjusted so that it can be "reused".
     *
     * \param[in, out] sampleValue
     *     An uniformly distributed sample on [0,1]
     * \return
     *     The discrete index associated with the sample
     */
    size_t sampleReuse(float &sampleValue) const {
        size_t index = sample(sampleValue);
        sampleValue = (sampleValue - m_cdf[index])
            / (m_cdf[index + 1] - m_cdf[index]);
        return index;
    }

    /**
     * \brief %Transform a uniformly distributed sample. 
     * 
     * The original sample is value adjusted so that it can be "reused".
     *
     * \param[in,out]
     *     An uniformly distributed sample on [0,1]
     * \param[out] pdf
     *     Probability value of the sample
     * \return
     *     The discrete index associated with the sample
     */
    size_t sampleReuse(float &sampleValue, float &pdf) const {
        size_t index = sample(sampleValue, pdf);
        sampleValue = (sampleValue - m_cdf[index])
            / (m_cdf[index + 1] - m_cdf[index]);
        return index;
    }

    /**
     * \brief Turn the underlying distribution into a
     * human-readable string format
     */
    std::string toString() const {
        std::string result = tfm::format("DiscretePDF[sum=%f, "
            "normalized=%f, pdf = {", m_sum, m_normalized);

        for (size_t i=0; i<m_cdf.size(); ++i) {
            result += std::to_string(operator[](i));
            if (i != m_cdf.size()-1)
                result += ", ";
        }
        return result + "}]";
    }
private:
    std::vector<float> m_cdf;
    float m_sum, m_normalization;
    bool m_normalized;
};

NORI_NAMESPACE_END
