#include <nori/gui.h>
#include <nori/block.h>
#include <nanogui/glutil.h>
#include <nanogui/label.h>
#include <nanogui/slider.h>
#include <nanogui/layout.h>

NORI_NAMESPACE_BEGIN

NoriScreen::NoriScreen(const ImageBlock &block)
 : nanogui::Screen(block.getSize() + Vector2i(0, 36), "Nori", false), m_block(block) {
    using namespace nanogui;

    /* Add some UI elements to adjust the exposure value */
    Widget *panel = new Widget(this);
    panel->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 10, 10));
    new Label(panel, "Exposure value: ", "sans-bold");
    m_slider = new Slider(panel);
    m_slider->setValue(0.5f);
    m_slider->setFixedWidth(150);
    m_slider->setCallback(
        [&](float value) {
            m_scale = std::pow(2.f, (value - 0.5f) * 20);
        }
    );

    panel->setSize(block.getSize());
    performLayout(mNVGContext);

    panel->setPosition(
        Vector2i((mSize.x() - panel->size().x()) / 2, block.getSize().y()));

    /* Simple gamma tonemapper as a GLSL shader */
    m_shader = new GLShader();
    m_shader->init(
        "Tonemapper",

        /* Vertex shader */
        "#version 330\n"
        "in vec2 position;\n"
        "out vec2 uv;\n"
        "void main() {\n"
        "    gl_Position = vec4(position.x*2-1, position.y*2-1, 0.0, 1.0);\n"
        "    uv = vec2(position.x, 1-position.y);\n"
        "}",

        /* Fragment shader */
        "#version 330\n"
        "uniform sampler2D source;\n"
        "uniform float scale;\n"
        "in vec2 uv;\n"
        "out vec4 out_color;\n"
        "float toSRGB(float value) {\n"
        "    if (value < 0.0031308)\n"
        "        return 12.92 * value;\n"
        "    return 1.055 * pow(value, 0.41666) - 0.055;\n"
        "}\n"
        "void main() {\n"
        "    vec4 color = texture(source, uv);\n"
        "    color *= scale / color.w;\n"
        "    out_color = vec4(toSRGB(color.r), toSRGB(color.g), toSRGB(color.b), 1);\n"
        "}"
    );

    MatrixXu indices(3, 2); /* Draw 2 triangles */
    indices.col(0) << 0, 1, 2;
    indices.col(1) << 2, 3, 0;

    MatrixXf positions(2, 4);
    positions.col(0) << 0, 0;
    positions.col(1) << 1, 0;
    positions.col(2) << 1, 1;
    positions.col(3) << 0, 1;

    m_shader->bind();
    m_shader->uploadIndices(indices);
    m_shader->uploadAttrib("position", positions);

    /* Allocate texture memory for the rendered image */
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    drawAll();
    setVisible(true);
}

NoriScreen::~NoriScreen() {
    glDeleteTextures(1, &m_texture);
    delete m_shader;
}

void NoriScreen::drawContents() {
    /* Reload the partially rendered image onto the GPU */
    m_block.lock();
    int borderSize = m_block.getBorderSize();
    const Vector2i &size = m_block.getSize();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint) m_block.cols());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size.x(), size.y(),
            0, GL_RGBA, GL_FLOAT, (uint8_t *) m_block.data() +
            (borderSize * m_block.cols() + borderSize) * sizeof(Color4f));
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    m_block.unlock();

    glViewport(0, GLsizei(36 * mPixelRatio), GLsizei(mPixelRatio*size[0]),
         GLsizei(mPixelRatio*size[1]));
    m_shader->bind();
    m_shader->setUniform("scale", m_scale);
    m_shader->setUniform("source", 0);
    m_shader->drawIndexed(GL_TRIANGLES, 0, 2);
    glViewport(0, 0, mFBSize[0], mFBSize[1]);
}

NORI_NAMESPACE_END
