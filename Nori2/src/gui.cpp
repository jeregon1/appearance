#include <nori/gui.h>
#include <nori/block.h>
#include <nanogui/label.h>
#include <nanogui/slider.h>
#include <nanogui/layout.h>
#include <nanogui/opengl.h>

NORI_NAMESPACE_BEGIN

NoriScreen::NoriScreen(const ImageBlock &block)
 : nanogui::Screen(nanogui::Vector2i(block.getSize().x(), block.getSize().y() + 36), "Nori", false), m_block(block) {
    using namespace nanogui;

    /* Add some UI elements to adjust the exposure value */
    Widget *panel = new Widget(this);
    panel->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 10, 10));
    new Label(panel, "Exposure value: ", "sans-bold");
    m_slider = new Slider(panel);
    m_slider->set_value(0.5f);
    m_slider->set_fixed_width(150);
    m_slider->set_callback(
        [&](float value) {
            m_scale = std::pow(2.f, (value - 0.5f) * 20);
        }
    );

    panel->set_size(nanogui::Vector2i(block.getSize().x(), block.getSize().y()));
    perform_layout(nvg_context());

    panel->set_position(
        nanogui::Vector2i((size().x() - panel->size().x()) / 2, block.getSize().y()));

    /* Simple gamma tonemapper as a GLSL shader */
    const char *vertexSource =
        "#version 330\n"
        "out vec2 uv;\n"
        "const vec2 positions[6] = vec2[6](\n"
        "    vec2(0, 0), vec2(1, 0), vec2(1, 1),\n"
        "    vec2(1, 1), vec2(0, 1), vec2(0, 0));\n"
        "void main() {\n"
        "    vec2 position = positions[gl_VertexID];\n"
        "    gl_Position = vec4(position * 2 - 1, 0.0, 1.0);\n"
        "    uv = vec2(position.x, 1 - position.y);\n"
        "}";
    const char *fragmentSource =
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
        "}";

    auto compileShader = [](GLenum type, const char *source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        GLint compiled;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
            throw NoriException("Could not compile the tonemapper shader");
        return shader;
    };
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    m_shader_program = glCreateProgram();
    glAttachShader(m_shader_program, vertexShader);
    glAttachShader(m_shader_program, fragmentShader);
    glLinkProgram(m_shader_program);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glGenVertexArrays(1, &m_vertex_array);

    /* Allocate texture memory for the rendered image */
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    draw_all();
    set_visible(true);
}

NoriScreen::~NoriScreen() {
    glDeleteTextures(1, &m_texture);
    glDeleteVertexArrays(1, &m_vertex_array);
    glDeleteProgram(m_shader_program);
}

void NoriScreen::draw_contents() {
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

        const float pixelRatio = pixel_ratio();
        const auto &framebufferSize = framebuffer_size();
        glViewport(0, GLsizei(36 * pixelRatio), GLsizei(pixelRatio * size[0]),
            GLsizei(pixelRatio * size[1]));
            glBindVertexArray(m_vertex_array);
    glUseProgram(m_shader_program);
    glUniform1f(glGetUniformLocation(m_shader_program, "scale"), m_scale);
    glUniform1i(glGetUniformLocation(m_shader_program, "source"), 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glViewport(0, 0, framebufferSize[0], framebufferSize[1]);
}

NORI_NAMESPACE_END
