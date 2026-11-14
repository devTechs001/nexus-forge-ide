// ui/renderer/render_engine.hpp
#pragma once

#include "../../core/engine/event_system.hpp"
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <functional>
#include <unordered_map>

namespace NexusForge::UI {

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
    
    // Gradient colors
    static Color Orange() { return Color(1, 0.5f, 0, 1); }
    static Color Purple() { return Color(0.6f, 0.2f, 0.8f, 1); }
    static Color Cyan() { return Color(0, 0.9f, 1, 1); }
    static Color Pink() { return Color(1, 0.4f, 0.7f, 1); }
    static Color Lime() { return Color(0.7f, 1, 0, 1); }
};

// Rectangle
struct Rect {
    float x, y, width, height;

    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {}

    float right() const { return x + width; }
    float bottom() const { return y + height; }
    float left() const { return x; }
    float top() const { return y; }
    bool contains(float px, float py) const {
        return px >= x && px < right() && py >= y && py < bottom();
    }
    bool intersects(const Rect& other) const;
    Rect intersection(const Rect& other) const;
    
    bool operator==(const Rect& other) const {
        return x == other.x && y == other.y && width == other.width && height == other.height;
    }
    bool operator!=(const Rect& other) const {
        return !(*this == other);
    }
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

// Draw command types
enum class DrawCommandType {
    DrawRect,
    DrawRoundedRect,
    DrawCircle,
    DrawLine,
    DrawTriangle,
    DrawText,
    DrawImage,
    DrawPath,
    DrawGradient,
    SetClipRect,
    ClearClipRect,
    SetTransform,
    PushTransform,
    PopTransform,
    SetBlendMode,
    Custom
};

// Blend modes
enum class BlendMode {
    None,
    Alpha,
    Additive,
    Multiply,
    Screen
};

// Gradient type
enum class GradientType {
    Linear,
    Radial,
    Conical,
    MultiStop
};

// Gradient definition
struct Gradient {
    GradientType type = GradientType::Linear;
    std::vector<std::pair<float, Color>> stops;
    Vec2 startPoint = {0, 0};
    Vec2 endPoint = {1, 1};
    float radius = 1.0f;
};

// Draw command
struct DrawCommand {
    DrawCommandType type;

    Rect rect;
    Color color = Color::White();
    Color color2 = Color::White();
    float thickness = 1.0f;
    float cornerRadius = 0.0f;

    std::string text;
    std::string fontFamily;
    float fontSize = 14.0f;
    bool bold = false;
    bool italic = false;

    TextureHandle texture = 0;
    Rect srcRect;

    Mat4 transform;
    Rect clipRect;
    bool hasClip = false;

    BlendMode blendMode = BlendMode::Alpha;

    std::vector<Vec2> pathPoints;
    bool closePath = false;
    bool filled = false;

    Gradient gradient;
    bool useGradient = false;
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

// Render Backend interface
class RenderBackend {
public:
    virtual ~RenderBackend() = default;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    virtual void setViewport(int x, int y, int width, int height) = 0;
    virtual void clear(const Color& color) = 0;
    virtual void draw(const DrawCommand& cmd) = 0;
};

// Forward declarations - implementations in separate files
class TextureManager;
class ShaderManager;
class FontRenderer;

// Render Engine
class RenderEngine {
public:
    RenderEngine();
    ~RenderEngine();

    bool initialize(RenderAPI api = RenderAPI::Auto);
    void shutdown();

    void beginFrame();
    void endFrame();
    void present();

    void setViewport(int x, int y, int width, int height);
    void setProjection(const Mat4& projection);
    void clear(const Color& color);

    // Drawing primitives
    void drawRect(const Rect& rect, const Color& color);
    void drawRoundedRect(const Rect& rect, const Color& color, float radius);
    void drawRoundedRect(const Rect& rect, const Color& color,
                         float topLeft, float topRight, float bottomRight, float bottomLeft);
    void drawCircle(float cx, float cy, float radius, const Color& color, bool filled = true);
    void drawLine(float x1, float y1, float x2, float y2, const Color& color, float thickness = 1.0f);
    void drawTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3,
                      const Color& color, bool filled = true);

    // Gradients
    void drawGradientRect(const Rect& rect, const Color& topColor, const Color& bottomColor);
    void drawHorizontalGradient(const Rect& rect, const Color& leftColor, const Color& rightColor);
    void drawRadialGradient(const Rect& rect, const Color& centerColor, const Color& edgeColor);
    void drawMultiGradient(const Rect& rect, const Gradient& gradient);

    // Text
    void drawText(const std::string& text, float x, float y,
                  const Color& color, const std::string& fontFamily = "default",
                  float fontSize = 14.0f);
    void drawText(const std::string& text, float x, float y, float maxWidth,
                  const Color& color, const std::string& fontFamily = "default",
                  float fontSize = 14.0f);
    void drawTextCentered(const std::string& text, const Rect& rect,
                          const Color& color, const std::string& fontFamily = "default",
                          float fontSize = 14.0f);
    Vec2 measureText(const std::string& text, const std::string& fontFamily = "default",
                     float fontSize = 14.0f);

    // Images
    void drawImage(TextureHandle texture, const Rect& destRect);
    void drawImage(TextureHandle texture, const Rect& destRect, const Rect& srcRect);
    void drawImage(TextureHandle texture, const Rect& destRect, const Color& tint);
    void drawNineSlice(TextureHandle texture, const Rect& destRect,
                       float borderLeft, float borderTop, float borderRight, float borderBottom);

    // Path drawing
    void beginPath();
    void moveTo(float x, float y);
    void lineTo(float x, float y);
    void bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y);
    void quadTo(float cx, float cy, float x, float y);
    void arcTo(float x, float y, float radius, float startAngle, float endAngle);
    void closePath();
    void strokePath(const Color& color, float thickness = 1.0f);
    void fillPath(const Color& color);
    void drawPath(const std::vector<Vec2>& points, const Color& color, float thickness = 1.0f, bool filled = false);
    void drawBezier(const std::vector<Vec2>& points, const Color& color, float thickness = 1.0f);

    // Clipping
    void pushClipRect(const Rect& rect);
    void popClipRect();
    void setClipRect(const Rect& rect);
    void clearClipRect();

    // Transform
    void pushTransform();
    void popTransform();
    void translate(float x, float y);
    void rotate(float angle);
    void scale(float x, float y);
    void setTransform(const Mat4& transform);

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

    // Statistics and accessors
    const RenderStats& getStats() const { return stats_; }
    RenderAPI getCurrentAPI() const { return currentAPI_; }
    int getViewportWidth() const { return viewportWidth_; }
    int getViewportHeight() const { return viewportHeight_; }
    const Mat4& getProjection() const { return projection_; }
    const std::vector<DrawCommand>& getCommandBuffer() const { return commandBuffer_; }

    RenderBackend* getBackend() { return backend_.get(); }
    TextureManager* getTextureManager() { return textureManager_.get(); }
    ShaderManager* getShaderManager() { return shaderManager_.get(); }
    FontRenderer* getFontRenderer() { return fontRenderer_.get(); }
    void setBackend(std::unique_ptr<RenderBackend> backend);

private:
    std::unique_ptr<RenderBackend> backend_;
    std::unique_ptr<TextureManager> textureManager_;
    std::unique_ptr<ShaderManager> shaderManager_;
    std::unique_ptr<FontRenderer> fontRenderer_;

    RenderAPI currentAPI_ = RenderAPI::Auto;
    RenderStats stats_;

    std::vector<DrawCommand> commandBuffer_;
    std::vector<Mat4> transformStack_;
    std::vector<Rect> clipStack_;

    Mat4 projection_;
    int viewportWidth_ = 0;
    int viewportHeight_ = 0;

    std::vector<Vec2> currentPath_;
    Vec2 pathStartPoint_;

    void flushCommandBuffer();
    void executeCommand(const DrawCommand& cmd);
    RenderBackend* createBackend(RenderAPI api);
};

} // namespace NexusForge::UI
