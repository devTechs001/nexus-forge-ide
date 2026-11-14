// ui/renderer/opengl_backend.hpp
#pragma once

#include "render_engine.hpp"

#ifdef NEXUS_ENABLE_OPENGL

#include <vector>
#include <string>
#include <unordered_map>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

namespace NexusForge::UI {

// OpenGL render backend
class OpenGLBackend : public RenderBackend {
public:
    OpenGLBackend();
    ~OpenGLBackend() override;

    bool initialize() override;
    void shutdown() override;

    void beginFrame() override;
    void endFrame() override;
    void setViewport(int x, int y, int width, int height) override;
    void clear(const Color& color) override;
    void draw(const DrawCommand& cmd) override;

    // OpenGL-specific
    GLuint getVAO() const { return vao_; }
    GLuint getVBO() const { return vbo_; }
    GLuint getEBO() const { return ebo_; }

private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ebo_ = 0;
    GLuint shaderProgram_ = 0;
    GLuint currentTexture_ = 0;

    int viewportWidth_ = 0;
    int viewportHeight_ = 0;
    Mat4 projection_;

    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<DrawCommand> commandBuffer_;
    std::unordered_map<uint64_t, GLuint> textures_;

    bool createShaderProgram();
    bool createBuffers();
    void setupVertexAttributes();
    void flush();

    GLuint compileShader(GLenum type, const char* source);
    void renderRectCmd(const DrawCommand& cmd);
    void renderRoundedRectCmd(const DrawCommand& cmd);
    void renderCircleCmd(const DrawCommand& cmd);
    void renderLineCmd(const DrawCommand& cmd);
    void renderTextCmd(const DrawCommand& cmd);
    void renderImageCmd(const DrawCommand& cmd);
    void updateProjection();
};

} // namespace NexusForge::UI

#endif // NEXUS_ENABLE_OPENGL
