// ui/renderer/render_engine.cpp
#include "render_engine.hpp"
#include <cmath>
#include <algorithm>

namespace NexusForge::UI {

Color Color::fromHex(const std::string& hex) {
    std::string h = hex;
    if (h[0] == '#') h = h.substr(1);
    
    uint32_t value = static_cast<uint32_t>(std::stoul(h, nullptr, 16));
    
    if (h.length() == 6) {
        return Color(
            ((value >> 16) & 0xFF) / 255.0f,
            ((value >> 8) & 0xFF) / 255.0f,
            (value & 0xFF) / 255.0f,
            1.0f
        );
    } else if (h.length() == 8) {
        return Color(
            ((value >> 24) & 0xFF) / 255.0f,
            ((value >> 16) & 0xFF) / 255.0f,
            ((value >> 8) & 0xFF) / 255.0f,
            (value & 0xFF) / 255.0f
        );
    }
    
    return Color(0, 0, 0, 1);
}

uint32_t Color::toUint32() const {
    return (static_cast<uint32_t>(r * 255) << 24) |
           (static_cast<uint32_t>(g * 255) << 16) |
           (static_cast<uint32_t>(b * 255) << 8) |
           static_cast<uint32_t>(a * 255);
}

bool Rect::intersects(const Rect& other) const {
    return x < other.right() && right() > other.x &&
           y < other.bottom() && bottom() > other.y;
}

Rect Rect::intersection(const Rect& other) const {
    float ix = std::max(x, other.x);
    float iy = std::max(y, other.y);
    float ir = std::min(right(), other.right());
    float ib = std::min(bottom(), other.bottom());
    
    if (ir <= ix || ib <= iy) {
        return Rect();
    }
    
    return Rect(ix, iy, ir - ix, ib - iy);
}

float Vec2::length() const {
    return std::sqrt(x * x + y * y);
}

Vec2 Vec2::normalized() const {
    float len = length();
    if (len > 0) {
        return Vec2(x / len, y / len);
    }
    return *this;
}

Mat4::Mat4() {
    for (int i = 0; i < 16; ++i) m[i] = 0;
    m[15] = 1;
}

Mat4 Mat4::identity() {
    return Mat4();
}

Mat4 Mat4::ortho(float left, float right, float bottom, float top, float near, float far) {
    Mat4 result;
    result.m[0] = 2.0f / (right - left);
    result.m[5] = 2.0f / (top - bottom);
    result.m[10] = -2.0f / (far - near);
    result.m[12] = -(right + left) / (right - left);
    result.m[13] = -(top + bottom) / (top - bottom);
    result.m[14] = -(far + near) / (far - near);
    return result;
}

Mat4 Mat4::translate(float x, float y, float z) {
    Mat4 result;
    result.m[12] = x;
    result.m[13] = y;
    result.m[14] = z;
    return result;
}

Mat4 Mat4::scale(float x, float y, float z) {
    Mat4 result;
    result.m[0] = x;
    result.m[5] = y;
    result.m[10] = z;
    return result;
}

Mat4 Mat4::rotateZ(float angle) {
    Mat4 result;
    float c = std::cos(angle);
    float s = std::sin(angle);
    result.m[0] = c;
    result.m[1] = s;
    result.m[4] = -s;
    result.m[5] = c;
    return result;
}

Mat4 Mat4::operator*(const Mat4& other) const {
    Mat4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m[i * 4 + j] = 0;
            for (int k = 0; k < 4; ++k) {
                result.m[i * 4 + j] += m[k * 4 + j] * other.m[i * 4 + k];
            }
        }
    }
    return result;
}

// RenderEngine implementation
RenderEngine::RenderEngine() = default;

RenderEngine::~RenderEngine() {
    shutdown();
}

bool RenderEngine::initialize(RenderAPI api) {
    currentAPI_ = api;
    
    // Create appropriate backend based on API
    backend_ = createBackend(api);
    if (!backend_) {
        return false;
    }
    
    textureManager_ = std::make_unique<TextureManager>();
    shaderManager_ = std::make_unique<ShaderManager>();
    fontRenderer_ = std::make_unique<FontRenderer>();
    
    return backend_->initialize();
}

void RenderEngine::shutdown() {
    commandBuffer_.clear();
    transformStack_.clear();
    clipStack_.clear();
    
    fontRenderer_.reset();
    shaderManager_.reset();
    textureManager_.reset();
    backend_.reset();
}

void RenderEngine::beginFrame() {
    stats_ = RenderStats();
    commandBuffer_.clear();
}

void RenderEngine::endFrame() {
    flushCommandBuffer();
}

void RenderEngine::present() {
    if (backend_) {
        backend_->present();
    }
}

void RenderEngine::setViewport(int x, int y, int width, int height) {
    viewportWidth_ = width;
    viewportHeight_ = height;
    if (backend_) {
        backend_->setViewport(x, y, width, height);
    }
}

void RenderEngine::setProjection(const Mat4& projection) {
    projection_ = projection;
}

void RenderEngine::clear(const Color& color) {
    if (backend_) {
        backend_->clear(color);
    }
}

void RenderEngine::drawRect(const Rect& rect, const Color& color) {
    DrawCommand cmd;
    cmd.type = DrawCommandType::DrawRect;
    cmd.rect = rect;
    cmd.color = color;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawRoundedRect(const Rect& rect, const Color& color, float radius) {
    DrawCommand cmd;
    cmd.type = DrawCommandType::DrawRoundedRect;
    cmd.rect = rect;
    cmd.color = color;
    cmd.cornerRadius = radius;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawRoundedRect(const Rect& rect, const Color& color,
                                    float topLeft, float topRight, 
                                    float bottomRight, float bottomLeft) {
    // For simplicity, use minimum radius
    drawRoundedRect(rect, color, std::min({topLeft, topRight, bottomRight, bottomLeft}));
}

void RenderEngine::drawCircle(float cx, float cy, float radius, 
                               const Color& color, bool filled) {
    DrawCommand cmd;
    cmd.type = DrawCommandType::DrawCircle;
    cmd.rect = Rect(cx - radius, cy - radius, radius * 2, radius * 2);
    cmd.color = color;
    cmd.thickness = filled ? -1 : 1.0f;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawLine(float x1, float y1, float x2, float y2, 
                             const Color& color, float thickness) {
    DrawCommand cmd;
    cmd.type = DrawCommandType::DrawLine;
    cmd.rect = Rect(std::min(x1, x2), std::min(y1, y2), 
                    std::abs(x2 - x1), std::abs(y2 - y1));
    cmd.color = color;
    cmd.thickness = thickness;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3,
                                 const Color& color, bool filled) {
    DrawCommand cmd;
    cmd.type = DrawCommandType::DrawTriangle;
    cmd.pathPoints = {p1, p2, p3};
    cmd.color = color;
    cmd.filled = filled;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawGradientRect(const Rect& rect, 
                                     const Color& topColor, 
                                     const Color& bottomColor) {
    DrawCommand cmd;
    cmd.type = DrawCommandType::DrawRect;
    cmd.rect = rect;
    cmd.color = topColor;
    cmd.color2 = bottomColor;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawHorizontalGradient(const Rect& rect,
                                           const Color& leftColor,
                                           const Color& rightColor) {
    DrawCommand cmd;
    cmd.type = DrawCommandType::DrawRect;
    cmd.rect = rect;
    cmd.color = leftColor;
    cmd.color2 = rightColor;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawText(const std::string& text, float x, float y,
                             const Color& color, const std::string& fontFamily,
                             float fontSize) {
    if (fontRenderer_) {
        fontRenderer_->drawText(*this, text, x, y, color, fontFamily, fontSize);
    }
}

void RenderEngine::drawTextCentered(const std::string& text, const Rect& rect,
                                     const Color& color, const std::string& fontFamily,
                                     float fontSize) {
    Vec2 size = measureText(text, fontFamily, fontSize);
    float x = rect.x + (rect.width - size.x) / 2;
    float y = rect.y + (rect.height - size.y) / 2;
    drawText(text, x, y, color, fontFamily, fontSize);
}

Vec2 RenderEngine::measureText(const std::string& text, 
                                const std::string& fontFamily,
                                float fontSize) {
    if (fontRenderer_) {
        return fontRenderer_->measureText(text, fontFamily, fontSize);
    }
    return Vec2(0, 0);
}

void RenderEngine::drawImage(TextureHandle texture, const Rect& destRect) {
    DrawCommand cmd;
    cmd.type = DrawCommandType::DrawImage;
    cmd.rect = destRect;
    cmd.texture = texture;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawImage(TextureHandle texture, const Rect& destRect, 
                              const Rect& srcRect) {
    DrawCommand cmd;
    cmd.type = DrawCommandType::DrawImage;
    cmd.rect = destRect;
    cmd.srcRect = srcRect;
    cmd.texture = texture;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawImage(TextureHandle texture, const Rect& destRect, 
                              const Color& tint) {
    DrawCommand cmd;
    cmd.type = DrawCommandType::DrawImage;
    cmd.rect = destRect;
    cmd.texture = texture;
    cmd.color = tint;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawNineSlice(TextureHandle texture, const Rect& destRect,
                                  float borderLeft, float borderTop,
                                  float borderRight, float borderBottom) {
    // Implementation for 9-slice scaling
}

void RenderEngine::beginPath() {
    currentPath_.clear();
}

void RenderEngine::moveTo(float x, float y) {
    currentPath_.clear();
    currentPath_.push_back(Vec2(x, y));
    pathStartPoint_ = Vec2(x, y);
}

void RenderEngine::lineTo(float x, float y) {
    currentPath_.push_back(Vec2(x, y));
}

void RenderEngine::bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y) {
    // Bezier curve implementation
}

void RenderEngine::quadTo(float cx, float cy, float x, float y) {
    // Quadratic curve implementation
}

void RenderEngine::arcTo(float x, float y, float radius, 
                          float startAngle, float endAngle) {
    // Arc implementation
}

void RenderEngine::closePath() {
    if (!currentPath_.empty()) {
        currentPath_.push_back(pathStartPoint_);
    }
}

void RenderEngine::strokePath(const Color& color, float thickness) {
    DrawCommand cmd;
    cmd.type = DrawCommandType::DrawPath;
    cmd.pathPoints = currentPath_;
    cmd.color = color;
    cmd.thickness = thickness;
    cmd.filled = false;
    commandBuffer_.push_back(cmd);
    currentPath_.clear();
}

void RenderEngine::fillPath(const Color& color) {
    DrawCommand cmd;
    cmd.type = DrawCommandType::DrawPath;
    cmd.pathPoints = currentPath_;
    cmd.color = color;
    cmd.filled = true;
    commandBuffer_.push_back(cmd);
    currentPath_.clear();
}

void RenderEngine::pushClipRect(const Rect& rect) {
    clipStack_.push_back(rect);
}

void RenderEngine::popClipRect() {
    if (!clipStack_.empty()) {
        clipStack_.pop_back();
    }
}

void RenderEngine::pushTransform() {
    if (transformStack_.empty()) {
        transformStack_.push_back(Mat4::identity());
    } else {
        transformStack_.push_back(transformStack_.back());
    }
}

void RenderEngine::popTransform() {
    if (!transformStack_.empty()) {
        transformStack_.pop_back();
    }
}

void RenderEngine::translate(float x, float y) {
    if (!transformStack_.empty()) {
        transformStack_.back() = transformStack_.back() * Mat4::translate(x, y, 0);
    }
}

void RenderEngine::rotate(float angle) {
    if (!transformStack_.empty()) {
        transformStack_.back() = transformStack_.back() * Mat4::rotateZ(angle);
    }
}

void RenderEngine::scale(float x, float y) {
    if (!transformStack_.empty()) {
        transformStack_.back() = transformStack_.back() * Mat4::scale(x, y, 1);
    }
}

void RenderEngine::setBlendMode(BlendMode mode) {
    // Set blend mode for subsequent draw calls
}

TextureHandle RenderEngine::createTexture(int width, int height, const void* data,
                                           bool generateMipmaps) {
    if (textureManager_) {
        return textureManager_->createTexture(width, height, data, generateMipmaps);
    }
    return 0;
}

TextureHandle RenderEngine::loadTexture(const std::string& path) {
    if (textureManager_) {
        return textureManager_->loadTexture(path);
    }
    return 0;
}

void RenderEngine::destroyTexture(TextureHandle texture) {
    if (textureManager_) {
        textureManager_->destroyTexture(texture);
    }
}

Vec2 RenderEngine::getTextureSize(TextureHandle texture) {
    if (textureManager_) {
        return textureManager_->getTextureSize(texture);
    }
    return Vec2(0, 0);
}

bool RenderEngine::loadFont(const std::string& name, const std::string& path) {
    if (fontRenderer_) {
        return fontRenderer_->loadFont(name, path);
    }
    return false;
}

bool RenderEngine::loadFontFromMemory(const std::string& name, const void* data, size_t size) {
    if (fontRenderer_) {
        return fontRenderer_->loadFontFromMemory(name, data, size);
    }
    return false;
}

void RenderEngine::flushCommandBuffer() {
    for (const auto& cmd : commandBuffer_) {
        executeCommand(cmd);
    }
    stats_.drawCalls = commandBuffer_.size();
}

void RenderEngine::executeCommand(const DrawCommand& cmd) {
    if (backend_) {
        backend_->executeDrawCommand(cmd);
    }
}

RenderBackend* RenderEngine::createBackend(RenderAPI api) {
    // Factory method to create appropriate backend
    // Implementation depends on available backends
    return nullptr;
}

} // namespace NexusForge::UI
