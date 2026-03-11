// ui/renderer/render_engine.hpp
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <functional>
#include <unordered_map>

namespace NexusForge::UI {

// Forward declarations
class RenderBackend;
class TextureManager;
class ShaderManager;
class FontRenderer;

// Render API selection
enum class RenderAPI {
    Auto,
    Vulkan,
    OpenGL,
    Metal,
    DirectX12,
    DirectX11,
    WebGL
};

// Color structure
struct Color {
    float r, g, b, a;

    Color() : r(0), g(0), b(0), a(1) {}
    Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
    Color(uint32_t hex) {
        r = ((hex >> 24) & 0xFF) / 255.0f;
        g = ((hex >> 16) & 0xFF) / 255.0f;
        b = ((hex >> 8) & 0xFF) / 255.0f;
        a = (hex & 0xFF) / 255.0f;
    }

    static Color fromHex(const std::string& hex);
    uint32_t toUint32() const;

    static Color White() { return Color(1, 1, 1, 1); }
    static Color Black() { return Color(0, 0, 0, 1); }
    static Color Red() { return Color(1, 0, 0, 1); }
    static Color Green() { return Color(0, 1, 0, 1); }
    static Color Blue() { return Color(0, 0, 1, 1); }
    static Color Transparent() { return Color(0, 0, 0, 0); }
};

// Rectangle
struct Rect {
    float x, y, width, height;

    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {}

    float right() const { return x + width; }
    float bottom() const { return y + height; }
    bool contains(float px, float py) const {
        return px >= x && px < right() && py >= y && py < bottom();
    }
    bool intersects(const Rect& other) const;
    Rect intersection(const Rect& other) const;
};

// Vector types
struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
    Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
    float length() const;
    Vec2 normalized() const;
};

struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct Vec4 {
    float x, y, z, w;
    Vec4() : x(0), y(0), z(0), w(0) {}
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
};

// Matrix
struct Mat4 {
    float m[16];

    Mat4();
    static Mat4 identity();
    static Mat4 ortho(float left, float right, float bottom, float top, float near, float far);
    static Mat4 translate(float x, float y, float z);
    static Mat4 scale(float x, float y, float z);
    static Mat4 rotateZ(float angle);

    Mat4 operator*(const Mat4& other) const;
};

// Vertex structure
struct Vertex {
    Vec2 position;
    Vec2 texCoord;
    Color color;
};

// Texture handle
using TextureHandle = uint64_t;
using ShaderHandle = uint64_t;
using BufferHandle = uint64_t;

// Blend modes
enum class BlendMode {
    None,
    Alpha,
    Additive,
    Multiply,
    Screen
};

// Render statistics
struct RenderStats {
    uint64_t drawCalls = 0;
    uint64_t triangles = 0;
    uint64_t vertices = 0;
    uint64_t textureBinds = 0;
    double frameTime = 0.0;
    double gpuTime = 0.0;
};

// Render Engine
class RenderEngine {
public:
    RenderEngine();
    ~RenderEngine();

    // Initialization
    bool initialize(RenderAPI api = RenderAPI::Auto);
    void shutdown();

    // Frame management
    void beginFrame();
    void endFrame();
    void present();

    // Viewport
    void setViewport(int x, int y, int width, int height);
    void setProjection(const Mat4& projection);

    // Clear
    void clear(const Color& color);

    // Drawing primitives
    void drawRect(const Rect& rect, const Color& color);
    void drawRoundedRect(const Rect& rect, const Color& color, float radius);
    void drawCircle(float cx, float cy, float radius, const Color& color, bool filled = true);
    void drawLine(float x1, float y1, float x2, float y2, const Color& color, float thickness = 1.0f);

    // Gradient drawing
    void drawGradientRect(const Rect& rect, const Color& topColor, const Color& bottomColor);
    void drawHorizontalGradient(const Rect& rect, const Color& leftColor, const Color& rightColor);

    // Text drawing
    void drawText(const std::string& text, float x, float y,
                  const Color& color, const std::string& fontFamily = "default",
                  float fontSize = 14.0f);
    void drawTextCentered(const std::string& text, const Rect& rect,
                          const Color& color, const std::string& fontFamily = "default",
                          float fontSize = 14.0f);
    Vec2 measureText(const std::string& text, const std::string& fontFamily = "default",
                     float fontSize = 14.0f);

    // Image drawing
    void drawImage(TextureHandle texture, const Rect& destRect);
    void drawImage(TextureHandle texture, const Rect& destRect, const Rect& srcRect);

    // Clipping
    void pushClipRect(const Rect& rect);
    void popClipRect();

    // Transform
    void pushTransform();
    void popTransform();
    void translate(float x, float y);
    void rotate(float angle);
    void scale(float x, float y);

    // Blend mode
    void setBlendMode(BlendMode mode);

    // Texture management
    TextureHandle createTexture(int width, int height, const void* data,
                                bool generateMipmaps = false);
    TextureHandle loadTexture(const std::string& path);
    void destroyTexture(TextureHandle texture);
    Vec2 getTextureSize(TextureHandle texture);

    // Font management
    bool loadFont(const std::string& name, const std::string& path);
    bool loadFontFromMemory(const std::string& name, const void* data, size_t size);

    // Statistics
    const RenderStats& getStats() const { return stats_; }
    RenderAPI getCurrentAPI() const { return currentAPI_; }

    // Backend access
    RenderBackend* getBackend() { return backend_.get(); }
    TextureManager* getTextureManager() { return textureManager_.get(); }
    FontRenderer* getFontRenderer() { return fontRenderer_.get(); }

private:
    std::unique_ptr<RenderBackend> backend_;
    std::unique_ptr<TextureManager> textureManager_;
    std::unique_ptr<ShaderManager> shaderManager_;
    std::unique_ptr<FontRenderer> fontRenderer_;

    RenderAPI currentAPI_ = RenderAPI::Auto;
    RenderStats stats_;

    std::vector<Mat4> transformStack_;
    std::vector<Rect> clipStack_;

    Mat4 projection_;
    int viewportWidth_ = 0;
    int viewportHeight_ = 0;

    void flushCommandBuffer();
    RenderBackend* createBackend(RenderAPI api);
};

} // namespace NexusForge::UI
