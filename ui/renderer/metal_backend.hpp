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

    bool initialize() override { return initialize(nullptr); }
    bool initialize(void* windowHandle);
    void shutdown() override;

    void beginFrame() override;
    void endFrame() override;
    void present() override;

    void setViewport(int x, int y, int width, int height) override;
    void setProjection(const Mat4& projection);
    void clear(const Color& color) override;
    void draw(const DrawCommand& cmd) override { executeDrawCommand(cmd); }
    void executeDrawCommand(const DrawCommand& cmd) override;

    void drawRect(const Rect& rect, const Color& color);
    void drawRoundedRect(const Rect& rect, const Color& color, float radius);
    void drawTexture(TextureHandle texture, const Rect& rect, const Color& tint);
    void drawLine(float x1, float y1, float x2, float y2, const Color& color, float thickness);
    void drawCircle(float cx, float cy, float radius, const Color& color, bool filled);
    void drawText(const std::string& text, float x, float y, const Color& color,
                  const std::string& fontFamily, float fontSize);
    void setClipRect(const Rect& rect);
    void clearClipRect();

    TextureHandle createTexture(int width, int height, const void* data);
    void destroyTexture(TextureHandle texture);
    void updateTexture(TextureHandle texture, int width, int height, const void* data);

    bool isInitialized() const;
    std::string getRendererInfo() const;

    // Metal-specific
    id<MTLDevice> getDevice() const { return device_; }
    id<MTLCommandQueue> getCommandQueue() const { return commandQueue_; }
    id<MTLRenderPipelineState> getPipelineState() const { return pipelineState_; }

private:
    bool initialized_ = false;
    void* windowHandle_ = nullptr;
    int viewportX_ = 0;
    int viewportY_ = 0;
    int viewportWidth_ = 0;
    int viewportHeight_ = 0;
    Mat4 projection_;
    Color clearColor_;

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
