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

	Modified (c) 2018 by Christoph Kreisl idea from Sebastian Herholz
	University of Tuebingen, Germany
*/

#include <nori/object.h>

NORI_NAMESPACE_BEGIN

/**
 * \brief RenderMode handles different render modes
 *
 * This class serves as interface for new render modes
 * In this example we implemented a progressive render mode for a faster visualization
 * (default: blockwise, new: progressive)
 */
class RenderMode : public NoriObject {
public:

	/// Release all memory
	virtual ~RenderMode() { }

	/// Main render process
	virtual void render(Scene *scene, const std::string &filename) = 0;

	/// Sub-Render process for one block
	virtual void renderBlock(const Scene *scene, Sampler *sampler, ImageBlock &block) = 0;

	EClassType getClassType() const { return ERenderMode; }

private:
	/* nothing to-do here */
};

NORI_NAMESPACE_END
