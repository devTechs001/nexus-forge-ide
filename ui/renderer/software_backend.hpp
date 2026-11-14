// ui/renderer/software_backend.hpp
// Cross-platform CPU 2D software renderer with optional FreeType font rendering
// and native framebuffer presentation.
#pragma once

#include "render_engine.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace NexusForge::UI {

class SoftwareRenderBackend : public RenderBackend {
public:
    explicit SoftwareRenderBackend(void* nativeWindow = nullptr);
    ~SoftwareRenderBackend() override;

    bool initialize() override;
    void shutdown() override;
    void beginFrame() override;
    void endFrame() override;
    void present() override;
    void setViewport(int x, int y, int width, int height) override;
    void clear(const Color& color) override;
    void draw(const DrawCommand& cmd) override;
    bool measureText(const std::string& text, const std::string& fontFamily,
                     float fontSize, Vec2& outSize) const override;

    void setNativeWindow(void* window) { nativeWindow_ = window; }
    void* getNativeWindow() const { return nativeWindow_; }

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    const std::vector<uint32_t>& getFramebuffer() const { return pixels_; }

    // Diagnostic/testing helper: write current framebuffer to a binary PPM image
    bool savePPM(const std::string& path) const;

private:
    struct FreeTypeState;

    void* nativeWindow_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    std::vector<uint32_t> pixels_; // 0xAARRGGBB (little-endian BGRA for X11 ZPixmap)

    Mat4 currentTransform_;
    std::vector<Mat4> transformStack_;
    Rect clipRect_;
    bool hasClip_ = false;
    BlendMode blendMode_ = BlendMode::Alpha;

    std::unique_ptr<FreeTypeState> ft_;

    void ensureFramebuffer(int w, int h);
    Vec2 transformPoint(float x, float y) const;
    float transformScale() const;

    void blendPixel(int x, int y, uint8_t sr, uint8_t sg, uint8_t sb, float sa);
    void fillSpan(int y, int x0, int x1, uint8_t sr, uint8_t sg, uint8_t sb, float sa);

    void drawRectImpl(const Rect& rect, const Color& color);
    void drawRoundedRectImpl(const Rect& rect, const Color& color, float radius);
    void drawCircleImpl(const Rect& rect, const Color& color, bool filled, float thickness);
    void drawLineImpl(float x1, float y1, float x2, float y2, const Color& color, float thickness);
    void drawGradientImpl(const DrawCommand& cmd);
    void drawPathImpl(const DrawCommand& cmd);
    void drawTextImpl(const DrawCommand& cmd);

    Color evaluateGradient(const Gradient& g, float t) const;
};

} // namespace NexusForge::UI
