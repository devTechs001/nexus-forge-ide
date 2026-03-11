// ui/renderer/opengl_backend.hpp
#pragma once

#include "render_engine.hpp"

#ifdef NEXUS_ENABLE_OPENGL

#include <vector>
#include <string>

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

    void setViewport(int x, int y, int width, int height) override;
    void clear(const Color& color) override;
    void present() override;

    void executeDrawCommand(const DrawCommand& cmd) override;

    // OpenGL-specific
    GLuint getVAO() const { return vao_; }
    GLuint getVertexBuffer() const { return vertexBuffer_; }
    GLuint getIndexBuffer() const { return indexBuffer_; }

private:
    GLuint vao_ = 0;
    GLuint vertexBuffer_ = 0;
    GLuint indexBuffer_ = 0;
    GLuint shaderProgram_ = 0;

    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;

    bool createShaderProgram();
    bool createBuffers();
    void setupVertexAttributes();

    void renderRect(const DrawCommand& cmd);
    void renderRoundedRect(const DrawCommand& cmd);
    void renderCircle(const DrawCommand& cmd);
    void renderLine(const DrawCommand& cmd);
    void renderTriangle(const DrawCommand& cmd);
    void renderText(const DrawCommand& cmd);
    void renderImage(const DrawCommand& cmd);
    void renderPath(const DrawCommand& cmd);
};

} // namespace NexusForge::UI

#endif // NEXUS_ENABLE_OPENGL
