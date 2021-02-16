#include <nori/gui.h>
#include <nori/block.h>
#include <nanogui/shader.h>
#include <nanogui/label.h>
#include <nanogui/slider.h>
#include <nanogui/layout.h>
#include <nanogui/renderpass.h>
#include <nanogui/texture.h>

NORI_NAMESPACE_BEGIN

NoriScreen::NoriScreen(const ImageBlock &block)
 : nanogui::Screen(nanogui::Vector2i(block.getSize().x(), block.getSize().y() + 36),
                   "Nori", false),
   m_block(block) {
    using namespace nanogui;
    inc_ref();

    /* Add some UI elements to adjust the exposure value */
    Widget *panel = new Widget(this);
    panel->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 10, 10));
    new Label(panel, "Exposure value: ", "sans-bold");
    Slider *slider = new Slider(panel);
    slider->set_value(0.5f);
    slider->set_fixed_width(150);
    slider->set_callback(
        [&](float value) {
            m_scale = std::pow(2.f, (value - 0.5f) * 20);
        }
    );

    panel->set_size(nanogui::Vector2i(block.getSize().x(), block.getSize().y()));
    perform_layout();

    panel->set_position(
        nanogui::Vector2i((m_size.x() - panel->size().x()) / 2, block.getSize().y()));

    /* Simple gamma tonemapper as a GLSL shader */

    m_renderPass = new RenderPass({ this });
    m_renderPass->set_clear_color(0, Color(0.3f, 0.3f, 0.3f, 1.f));

    m_shader = new Shader(
        m_renderPass,
        /* An identifying name */
        "Tonemapper",
        /* Vertex shader */
        R"(#version 330
        uniform ivec2 size;
        uniform int borderSize;

        in vec2 position;
        out vec2 uv;
        void main() {
            gl_Position = vec4(position.x * 2 - 1, position.y * 2 - 1, 0.0, 1.0);

            // Crop away image border (due to pixel filter)
            vec2 total_size = size + 2 * borderSize;
            vec2 scale = size / total_size;
            uv = vec2(position.x * scale.x + borderSize / total_size.x,
                      1 - (position.y * scale.y + borderSize / total_size.y));
        })",
        /* Fragment shader */
        R"(#version 330
        uniform sampler2D source;
        uniform float scale;
        in vec2 uv;
        out vec4 out_color;
        float toSRGB(float value) {
            if (value < 0.0031308)
                return 12.92 * value;
            return 1.055 * pow(value, 0.41666) - 0.055;
        }
        void main() {
            vec4 color = texture(source, uv);
            color *= scale / color.w;
            out_color = vec4(toSRGB(color.r), toSRGB(color.g), toSRGB(color.b), 1);
        })"
    );

    // Draw 2 triangles
    uint32_t indices[3 * 2] = {
        0, 1, 2,
        2, 3, 0
    };
    float positions[2 * 4] = {
        0.f, 0.f,
        1.f, 0.f,
        1.f, 1.f,
        0.f, 1.f
    };

    m_shader->set_buffer("indices", VariableType::UInt32, {3*2}, indices);
    m_shader->set_buffer("position", VariableType::Float32, {4, 2}, positions);

    const Vector2i &size = m_block.getSize();
    m_shader->set_uniform("size", nanogui::Vector2i(size.x(), size.y()));
    m_shader->set_uniform("borderSize", m_block.getBorderSize());

    // Allocate texture memory for the rendered image
    m_texture = new Texture(
        Texture::PixelFormat::RGBA,
        Texture::ComponentFormat::Float32,
        nanogui::Vector2i(size.x() + 2 * m_block.getBorderSize(),
                          size.y() + 2 * m_block.getBorderSize()),
        Texture::InterpolationMode::Nearest,
        Texture::InterpolationMode::Nearest);

    draw_all();
    set_visible(true);
}


void NoriScreen::draw_contents() {
    // Reload the partially rendered image onto the GPU
    m_block.lock();
    const Vector2i &size = m_block.getSize();
    m_shader->set_uniform("scale", m_scale);
    m_renderPass->resize(framebuffer_size());
    m_renderPass->begin();
    m_renderPass->set_viewport(nanogui::Vector2i(0, 0),
                               nanogui::Vector2i(m_pixel_ratio * size[0],
                                                 m_pixel_ratio * size[1]));
    m_texture->upload((uint8_t *) m_block.data());
    m_shader->set_texture("source", m_texture);
    m_shader->begin();
    m_shader->draw_array(nanogui::Shader::PrimitiveType::Triangle, 0, 6, true);
    m_shader->end();
    m_renderPass->set_viewport(nanogui::Vector2i(0, 0), framebuffer_size());
    m_renderPass->end();
    m_block.unlock();
}

NORI_NAMESPACE_END
