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

#if !defined(__NORI_PHOTON_H)
#define __NORI_PHOTON_H

#include <nori/kdtree.h>
#include <nori/color.h>

NORI_NAMESPACE_BEGIN

/// Stores the direction and power of a photon as "payload" of a generic kd-tree node (see kdtree.h)
struct PhotonData {
    uint8_t rgbe[4];        //!< Photon power stored in Greg Ward's RGBE format
    uint8_t theta;            //!< Discretized photon direction (\a theta component)
    uint8_t phi;            //!< Discretized photon direction (\a phi component)

    /// Dummy constructor
    PhotonData() { }

    /// Initialize a photon with the specified direction of propagation and power
    PhotonData(const Vector3f &dir, const Color3f &power);

    /**
     * Convert the stored photon direction from quantized spherical
     * coordinates to a Vector3f value. Uses precomputation similar to
     * that of Henrik Wann Jensen's implementation.
     */
    Vector3f getDirection() const {
        return Vector3f(
            m_cosPhi[phi] * m_sinTheta[theta],
            m_sinPhi[phi] * m_sinTheta[theta],
            m_cosTheta[theta]
        );
    }

    /// Convert the photon power from RGBE to floating point
    Color3f getPower() const {
        return Color3f(rgbe[0], rgbe[1], rgbe[2]) * m_expTable[rgbe[3]];
    }
protected:
    // ======================================================================
    /// @{ \name Precomputed lookup tables
    // ======================================================================

    static float m_cosTheta[256];
    static float m_sinTheta[256];
    static float m_cosPhi[256];
    static float m_sinPhi[256];
    static float m_expTable[256];
    static bool m_precompTableReady;

    /// @}
    // ======================================================================

    /// Initialize the precomputed lookup tables
    static bool initialize();
};

/// A Photon which also doubles as a kd-tree node for use with \ref PointKDTree
struct Photon : public GenericKDTreeNode<Point3f, PhotonData> {
public:
    /// Dummy constructor
    Photon() { }

    /// Construct from a photon interaction
    Photon(const Point3f &pos, const Vector3f &dir, const Color3f &power)
        : GenericKDTreeNode(pos, PhotonData(dir, power)) { }
    
    Vector3f getDirection() const { return data.getDirection(); }
    Color3f getPower() const { return data.getPower(); }
};

NORI_NAMESPACE_END

#endif /* __NORI_PHOTON_H */
