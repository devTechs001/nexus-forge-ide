// ui/renderer/opengl_backend.cpp
// OpenGL Backend Implementation

#include "opengl_backend.hpp"

#ifdef NEXUS_ENABLE_OPENGL

#include <iostream>
#include <cstring>
#include <sstream>
#include <cmath>
#include <cstddef>

namespace NexusForge::UI {

OpenGLBackend::OpenGLBackend() = default;

OpenGLBackend::~OpenGLBackend() {
    shutdown();
}

bool OpenGLBackend::initialize() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_SCISSOR_TEST);

    if (!createShaderProgram()) {
        std::cerr << "Failed to create OpenGL shader program" << std::endl;
        return false;
    }
    if (!createBuffers()) {
        std::cerr << "Failed to create OpenGL buffers" << std::endl;
        return false;
    }
    setupVertexAttributes();

    return true;
}

void OpenGLBackend::shutdown() {
    if (shaderProgram_) glDeleteProgram(shaderProgram_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (ebo_) glDeleteBuffers(1, &ebo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);

    for (auto& [id, tex] : textures_) {
        glDeleteTextures(1, &tex);
    }
    textures_.clear();

    shaderProgram_ = 0;
    vbo_ = 0;
    ebo_ = 0;
    vao_ = 0;
}

void OpenGLBackend::beginFrame() {
    commandBuffer_.clear();
    vertices_.clear();
    indices_.clear();
}

void OpenGLBackend::endFrame() {
    flush();
}

void OpenGLBackend::setViewport(int x, int y, int width, int height) {
    glViewport(x, y, width, height);
    viewportWidth_ = width;
    viewportHeight_ = height;
    projection_ = Mat4::ortho(0, (float)width, (float)height, 0, -1, 1);
}

void OpenGLBackend::clear(const Color& color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLBackend::draw(const DrawCommand& cmd) {
    switch (cmd.type) {
        case DrawCommandType::DrawRect:
            renderRectCmd(cmd);
            break;
        case DrawCommandType::DrawRoundedRect:
            renderRoundedRectCmd(cmd);
            break;
        case DrawCommandType::DrawCircle:
            renderCircleCmd(cmd);
            break;
        case DrawCommandType::DrawLine:
            renderLineCmd(cmd);
            break;
        case DrawCommandType::DrawText:
            renderTextCmd(cmd);
            break;
        case DrawCommandType::DrawImage:
            renderImageCmd(cmd);
            break;
        case DrawCommandType::SetClipRect:
            if (cmd.hasClip) {
                int x = (int)cmd.clipRect.x;
                int y = (int)(viewportHeight_ - cmd.clipRect.y - cmd.clipRect.height);
                glScissor(x, y, (int)cmd.clipRect.width, (int)cmd.clipRect.height);
                glEnable(GL_SCISSOR_TEST);
            }
            break;
        case DrawCommandType::ClearClipRect:
            glDisable(GL_SCISSOR_TEST);
            break;
        default:
            break;
    }
}

bool OpenGLBackend::createShaderProgram() {
    const char* vertexSource = R"(
        #version 330 core
        layout(location = 0) in vec2 aPosition;
        layout(location = 1) in vec2 aTexCoord;
        layout(location = 2) in vec4 aColor;

        out vec2 vTexCoord;
        out vec4 vColor;

        uniform mat4 uProjection;
        uniform mat4 uTransform;

        void main() {
            vTexCoord = aTexCoord;
            vColor = aColor;
            gl_Position = uProjection * uTransform * vec4(aPosition, 0.0, 1.0);
        }
    )";

    const char* fragmentSource = R"(
        #version 330 core
        in vec2 vTexCoord;
        in vec4 vColor;
        out vec4 fragColor;

        uniform sampler2D uTexture;
        uniform vec4 uUniformColor;
        uniform bool uUseTexture;
        uniform float uCornerRadius;
        uniform vec2 uSize;

        float roundedBoxSDF(vec2 p, vec2 b, float r) {
            return length(max(abs(p) - b + r, 0.0)) - r;
        }

        void main() {
            vec4 baseColor = vColor;
            if (uUseTexture) {
                vec4 texColor = texture(uTexture, vTexCoord);
                baseColor *= texColor;
            }
            if (uCornerRadius > 0.0) {
                vec2 uv = gl_FragCoord.xy;
                vec2 center = uSize * 0.5;
                float d = roundedBoxSDF(uv, center, uCornerRadius);
                float alpha = 1.0 - smoothstep(0.0, 1.0, d);
                baseColor.a *= alpha;
            }
            fragColor = baseColor * uUniformColor;
        }
    )";

    GLuint vert = compileShader(GL_VERTEX_SHADER, vertexSource);
    if (!vert) return false;
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!frag) {
        glDeleteShader(vert);
        return false;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    glDeleteShader(vert);
    glDeleteShader(frag);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader link failed: " << infoLog << std::endl;
        glDeleteProgram(program);
        return false;
    }

    shaderProgram_ = program;
    return true;
}

GLuint OpenGLBackend::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compile failed: " << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool OpenGLBackend::createBuffers() {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    return true;
}

void OpenGLBackend::setupVertexAttributes() {
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void OpenGLBackend::flush() {
    if (vertices_.empty()) return;

    glUseProgram(shaderProgram_);

    GLint projLoc = glGetUniformLocation(shaderProgram_, "uProjection");
    if (projLoc >= 0) glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection_.m);

    Mat4 identity = Mat4::identity();
    GLint transformLoc = glGetUniformLocation(shaderProgram_, "uTransform");
    if (transformLoc >= 0) glUniformMatrix4fv(transformLoc, 1, GL_FALSE, identity.m);

    GLint colorLoc = glGetUniformLocation(shaderProgram_, "uUniformColor");
    if (colorLoc >= 0) glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);

    GLint useTexLoc = glGetUniformLocation(shaderProgram_, "uUseTexture");
    if (useTexLoc >= 0) glUniform1i(useTexLoc, 0);

    GLint radiusLoc = glGetUniformLocation(shaderProgram_, "uCornerRadius");
    if (radiusLoc >= 0) glUniform1f(radiusLoc, 0.0f);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Vertex), vertices_.data(), GL_STREAM_DRAW);

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertices_.size());
    glBindVertexArray(0);

    vertices_.clear();
    indices_.clear();
}

void OpenGLBackend::renderRectCmd(const DrawCommand& cmd) {
    float x1 = cmd.rect.x, y1 = cmd.rect.y;
    float x2 = cmd.rect.x + cmd.rect.width, y2 = cmd.rect.y + cmd.rect.height;

    Vertex v;
    v.color = {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a};

    v.position = {x1, y1}; v.texCoord = {0, 0}; vertices_.push_back(v);
    v.position = {x2, y1}; v.texCoord = {1, 0}; vertices_.push_back(v);
    v.position = {x2, y2}; v.texCoord = {1, 1}; vertices_.push_back(v);
    v.position = {x1, y1}; v.texCoord = {0, 0}; vertices_.push_back(v);
    v.position = {x2, y2}; v.texCoord = {1, 1}; vertices_.push_back(v);
    v.position = {x1, y2}; v.texCoord = {0, 1}; vertices_.push_back(v);
}

void OpenGLBackend::renderRoundedRectCmd(const DrawCommand& cmd) {
    renderRectCmd(cmd);
}

void OpenGLBackend::renderCircleCmd(const DrawCommand& cmd) {
    const int segments = 32;
    float cx = cmd.rect.x + cmd.rect.width / 2;
    float cy = cmd.rect.y + cmd.rect.height / 2;
    float radius = std::min(cmd.rect.width, cmd.rect.height) / 2;

    Vertex cv;
    cv.position = {cx, cy};
    cv.texCoord = {0.5f, 0.5f};
    cv.color = {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a};

    for (int i = 0; i < segments; ++i) {
        float angle0 = (i / static_cast<float>(segments)) * 2.0f * 3.14159265f;
        float angle1 = ((i + 1) / static_cast<float>(segments)) * 2.0f * 3.14159265f;

        Vertex v0;
        v0.position = {cx + std::cos(angle0) * radius, cy + std::sin(angle0) * radius};
        v0.texCoord = {0.5f + 0.5f * std::cos(angle0), 0.5f + 0.5f * std::sin(angle0)};
        v0.color = cv.color;

        Vertex v1;
        v1.position = {cx + std::cos(angle1) * radius, cy + std::sin(angle1) * radius};
        v1.texCoord = {0.5f + 0.5f * std::cos(angle1), 0.5f + 0.5f * std::sin(angle1)};
        v1.color = cv.color;

        vertices_.push_back(cv);
        vertices_.push_back(v0);
        vertices_.push_back(v1);
    }
}

void OpenGLBackend::renderLineCmd(const DrawCommand& cmd) {
    float dx = cmd.rect.width;
    float dy = cmd.rect.height;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return;

    dx /= len; dy /= len;
    float px = -dy * cmd.thickness / 2;
    float py = dx * cmd.thickness / 2;

    float x1 = cmd.rect.x, y1 = cmd.rect.y;
    float x2 = cmd.rect.x + cmd.rect.width, y2 = cmd.rect.y + cmd.rect.height;

    Vertex v;
    v.color = {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a};
    v.texCoord = {0, 0};

    v.position = {x1 + px, y1 + py}; vertices_.push_back(v);
    v.position = {x2 + px, y2 + py}; vertices_.push_back(v);
    v.position = {x2 - px, y2 - py}; vertices_.push_back(v);
    v.position = {x1 + px, y1 + py}; vertices_.push_back(v);
    v.position = {x2 - px, y2 - py}; vertices_.push_back(v);
    v.position = {x1 - px, y1 - py}; vertices_.push_back(v);
}

void OpenGLBackend::renderTextCmd(const DrawCommand& cmd) {
    float cursorX = cmd.rect.x;
    for (char c : cmd.text) {
        if (c == ' ') { cursorX += cmd.fontSize * 0.3f; continue; }
        DrawCommand charCmd;
        charCmd.type = DrawCommandType::DrawRect;
        charCmd.rect = {cursorX, cmd.rect.y, cmd.fontSize * 0.6f, cmd.fontSize};
        charCmd.color = cmd.color;
        renderRectCmd(charCmd);
        cursorX += cmd.fontSize * 0.6f;
    }
}

void OpenGLBackend::renderImageCmd(const DrawCommand& cmd) {
    renderRectCmd(cmd);
}

void OpenGLBackend::updateProjection() {
    glUseProgram(shaderProgram_);
    GLint loc = glGetUniformLocation(shaderProgram_, "uProjection");
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, projection_.m);
}

} // namespace NexusForge::UI

#endif // NEXUS_ENABLE_OPENGL