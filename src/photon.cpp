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

#include <nori/photon.h>

NORI_NAMESPACE_BEGIN

bool PhotonData::m_precompTableReady = PhotonData::initialize();
float PhotonData::m_cosTheta[256];
float PhotonData::m_sinTheta[256];
float PhotonData::m_cosPhi[256];
float PhotonData::m_sinPhi[256];
float PhotonData::m_expTable[256];

bool PhotonData::initialize() {
    for (int i=0; i<256; i++) {
        float angle = (float) i * ((float) M_PI / 256.0f);
        m_cosPhi[i] = std::cos(2.0f * angle);
        m_sinPhi[i] = std::sin(2.0f * angle);
        m_cosTheta[i] = std::cos(angle);
        m_sinTheta[i] = std::sin(angle);
        m_expTable[i] = std::ldexp((float) 1, i - (128+8));
    }
    m_expTable[0] = 0;
    return true;
}

PhotonData::PhotonData(const Vector3f &dir, const Color3f &power) {
    if (!power.isValid())
        std::cerr << "Creating an invalid photon with power: " << power << endl;

    /* Convert the direction into an approximate spherical
       coordinate format to reduce storage requirements */
    theta = (uint8_t) std::min(255,
        (int) (std::acos(dir.z()) * (256.0f / M_PI)));

    int tmp = std::min(255,
        (int) (std::atan2(dir.y(), dir.x()) * (256.0f / (2.0f * M_PI))));
    if (tmp < 0)
        phi = (uint8_t) (tmp + 256);
    else
        phi = (uint8_t) tmp;

    /* Convert to Ward's RGBE format */
    float max = power.maxCoeff();
    if (max < 1e-32) {
        rgbe[0] = rgbe[1] = rgbe[2] = rgbe[3] = 0;
    } else {
        int e;
        /* Extract exponent and convert the fractional part into
           the [0..255] range. Afterwards, divide by max so that
           any color component multiplied by the result will be in [0,255] */
        max = std::frexp(max, &e) * 256.0f / max;
        rgbe[0] = (uint8_t) (power.r() * max);
        rgbe[1] = (uint8_t) (power.g() * max);
        rgbe[2] = (uint8_t) (power.b() * max);
        rgbe[3] = e + 128; /* Exponent value in bias format */
    }
}

NORI_NAMESPACE_END
