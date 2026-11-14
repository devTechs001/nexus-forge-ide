// ui/renderer/render_backend.hpp
#pragma once

// Note: This header is included by render_engine.hpp at the end
// All types (Color, DrawCommand) must be defined before this include

namespace NexusForge::UI {

// Base render backend class
class RenderBackend {
public:
    virtual ~RenderBackend() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    virtual void present() = 0;

    virtual void setViewport(int x, int y, int width, int height) = 0;
    virtual void clear(const Color& color) = 0;

    // Drawing commands
    virtual void executeDrawCommand(const DrawCommand& cmd) = 0;
};

} // namespace NexusForge::UI
