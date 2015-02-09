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
#include <nanogui/screen.h>

NORI_NAMESPACE_BEGIN

class NoriScreen : public nanogui::Screen {
public:
    NoriScreen(const ImageBlock &block);
    virtual ~NoriScreen();

    void drawContents();
private:
    const ImageBlock &m_block;
    nanogui::GLShader *m_shader = nullptr;
    nanogui::Slider *m_slider = nullptr;
    uint32_t m_texture = 0;
    float m_scale = 1.f;
};

NORI_NAMESPACE_END
