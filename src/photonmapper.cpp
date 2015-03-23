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

#include <nori/integrator.h>
#include <nori/sampler.h>
#include <nori/emitter.h>
#include <nori/bsdf.h>
#include <nori/scene.h>
#include <nori/photon.h>

NORI_NAMESPACE_BEGIN

class PhotonMapper : public Integrator {
public:
    /// Photon map data structure
    typedef PointKDTree<Photon> PhotonMap;

    PhotonMapper(const PropertyList &props) {
        /* Lookup parameters */
        m_photonCount  = props.getInteger("photonCount", 1000000);
        m_photonRadius = props.getFloat("photonRadius", 0.0f /* Default: automatic */);
    }

    void preprocess(const Scene *scene) {
        /* Create a sample generator for the preprocess step */
        Sampler *sampler = static_cast<Sampler *>(
            NoriObjectFactory::createInstance("independent", PropertyList()));

        /* Allocate memory for the photon map */
        m_photonMap = std::unique_ptr<PhotonMap>(new PhotonMap());
        m_photonMap->reserve(m_photonCount);

		/* Estimate a default photon radius */
		if (m_photonRadius == 0)
			m_photonRadius = scene->getBoundingBox().getExtents().norm() / 500.0f;

		/* Dummy gathering step: just add a single photon */
		m_photonMap->push_back(Photon(
			Point3f(0, 0, 0)  /* Position */,
			Vector3f(0, 0, 1) /* Direction */,
			Color3f(1, 2, 3)  /* Power */
		));

		/* Build the photon map */
        m_photonMap->build();

		/* Now let's do a lookup to see if it worked */
		std::vector<uint32_t> results;
		m_photonMap->search(Point3f(0, 0, 0) /* lookup position */, m_photonRadius, results);

		for (uint32_t i : results) {
            const Photon &photon = (*m_photonMap)[i];
			cout << "Found photon!" << endl;
			cout << " Position  : " << photon.getPosition().toString() << endl;
			cout << " Power     : " << photon.getPower().toString() << endl;
			cout << " Direction : " << photon.getDirection().toString() << endl;
		}
    }

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &_ray) const {
    	throw NoriException("PhotonMapper::Li(): not implemented!");
    }

    std::string toString() const {
        return tfm::format(
            "PhotonMapper[\n"
            "  photonCount = %i,\n"
            "  photonRadius = %f\n"
            "]",
            m_photonCount,
            m_photonRadius
        );
    }
private:
    int m_photonCount;
    float m_photonRadius;
    std::unique_ptr<PhotonMap> m_photonMap;
};

NORI_REGISTER_CLASS(PhotonMapper, "photonmapper");
NORI_NAMESPACE_END
