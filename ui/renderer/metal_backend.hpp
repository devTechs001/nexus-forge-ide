// ui/renderer/metal_backend.hpp
#pragma once

#include "render_engine.hpp"

#ifdef NEXUS_ENABLE_METAL

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include <vector>

namespace NexusForge::UI {

// Metal render backend
class MetalBackend : public RenderBackend {
public:
    MetalBackend();
    ~MetalBackend() override;

    bool initialize() override;
    void shutdown() override;

    void setViewport(int x, int y, int width, int height) override;
    void clear(const Color& color) override;
    void present() override;

    void executeDrawCommand(const DrawCommand& cmd) override;

    // Metal-specific
    id<MTLDevice> getDevice() const { return device_; }
    id<MTLCommandQueue> getCommandQueue() const { return commandQueue_; }
    id<MTLRenderPipelineState> getPipelineState() const { return pipelineState_; }

private:
    id<MTLDevice> device_ = nil;
    id<MTLCommandQueue> commandQueue_ = nil;
    id<MTLRenderPipelineState> pipelineState_ = nil;
    id<CAMetalLayer> metalLayer_ = nil;

    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;

    bool createDevice();
    bool createCommandQueue();
    bool createPipelineState();
    bool createBuffers();

    void renderRect(const DrawCommand& cmd);
    void renderRoundedRect(const DrawCommand& cmd);
    void renderCircle(const DrawCommand& cmd);
    void renderLine(const DrawCommand& cmd);
    void renderTriangle(const DrawCommand& cmd);
    void renderText(const DrawCommand& cmd);
    void renderImage(const DrawCommand& cmd);
};

} // namespace NexusForge::UI

#endif // NEXUS_ENABLE_METAL
