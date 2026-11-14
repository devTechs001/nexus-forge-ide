// ui/renderer/render_engine.cpp
#include "render_engine.hpp"
#include "texture_manager.hpp"
#include "shader_manager.hpp"
#include "font_renderer.hpp"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace NexusForge::UI {

// Color utilities
Color Color::fromHex(const std::string& hex) {
    if (hex.empty() || hex[0] != '#') return Color();
    std::string digits = hex.substr(1);
    if (digits.empty()) return Color();

    try {
        size_t consumed = 0;
        unsigned long value = std::stoul(digits, &consumed, 16);
        if (consumed != digits.size()) return Color();

        if (digits.length() == 3) {
            float r = static_cast<float>(((value >> 8) & 0xF) * 17) / 255.0f;
            float g = static_cast<float>(((value >> 4) & 0xF) * 17) / 255.0f;
            float b = static_cast<float>((value & 0xF) * 17) / 255.0f;
            return Color(r, g, b, 1.0f);
        } else if (digits.length() == 4) {
            float r = static_cast<float>(((value >> 12) & 0xF) * 17) / 255.0f;
            float g = static_cast<float>(((value >> 8) & 0xF) * 17) / 255.0f;
            float b = static_cast<float>(((value >> 4) & 0xF) * 17) / 255.0f;
            float a = static_cast<float>((value & 0xF) * 17) / 255.0f;
            return Color(r, g, b, a);
        } else if (digits.length() == 6) {
            return Color(((value >> 16) & 0xFF) / 255.0f,
                         ((value >> 8) & 0xFF) / 255.0f,
                         (value & 0xFF) / 255.0f, 1.0f);
        } else if (digits.length() == 8) {
            return Color(((value >> 24) & 0xFF) / 255.0f,
                         ((value >> 16) & 0xFF) / 255.0f,
                         ((value >> 8) & 0xFF) / 255.0f,
                         (value & 0xFF) / 255.0f);
        }
    } catch (...) {
        return Color();
    }
    return Color();
}

uint32_t Color::toUint32() const {
    auto clampByte = [](float v) -> uint32_t {
        float clamped = std::clamp(v, 0.0f, 1.0f);
        return static_cast<uint32_t>(std::lround(clamped * 255.0f));
    };
    return (clampByte(r) << 24) | (clampByte(g) << 16) |
           (clampByte(b) << 8) | clampByte(a);
}

float Vec2::length() const { return std::sqrt(x * x + y * y); }
Vec2 Vec2::normalized() const { float len = length(); return len > 0.0f ? Vec2(x / len, y / len) : *this; }

bool Rect::intersects(const Rect& o) const {
    return !(o.left() >= right() || o.right() <= left() || o.top() >= bottom() || o.bottom() <= top());
}

Rect Rect::intersection(const Rect& o) const {
    Rect r;
    r.x = std::max(x, o.x);
    r.y = std::max(y, o.y);
    r.width = std::min(right(), o.right()) - r.x;
    r.height = std::min(bottom(), o.bottom()) - r.y;
    return (r.width > 0.0f && r.height > 0.0f) ? r : Rect();
}

Mat4::Mat4() { std::memset(m, 0, sizeof(m)); m[0] = m[5] = m[10] = m[15] = 1.0f; }
Mat4 Mat4::identity() { return Mat4(); }
Mat4 Mat4::ortho(float l, float r, float b, float t, float n, float f) {
    Mat4 res;
    if (r == l || t == b || f == n) return res;
    res.m[0] = 2.0f / (r - l);
    res.m[5] = 2.0f / (t - b);
    res.m[10] = -2.0f / (f - n);
    res.m[12] = -(r + l) / (r - l);
    res.m[13] = -(t + b) / (t - b);
    res.m[14] = -(f + n) / (f - n);
    return res;
}
Mat4 Mat4::translate(float x, float y, float z) {
    Mat4 res; res.m[12] = x; res.m[13] = y; res.m[14] = z; return res;
}
Mat4 Mat4::scale(float x, float y, float z) {
    Mat4 res; res.m[0] = x; res.m[5] = y; res.m[10] = z; return res;
}
Mat4 Mat4::rotateZ(float angle) {
    Mat4 res; float c = std::cos(angle), s = std::sin(angle);
    res.m[0] = c; res.m[1] = s; res.m[4] = -s; res.m[5] = c; return res;
}
Mat4 Mat4::operator*(const Mat4& o) const {
    Mat4 res;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            res.m[i * 4 + j] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                res.m[i * 4 + j] += m[k * 4 + j] * o.m[i * 4 + k];
            }
        }
    }
    return res;
}

// RenderEngine implementation
RenderEngine::RenderEngine() {
    transformStack_.push_back(Mat4::identity());
}

RenderEngine::~RenderEngine() { shutdown(); }

bool RenderEngine::initialize(RenderAPI api) {
    currentAPI_ = api;
    textureManager_ = std::make_unique<TextureManager>();
    shaderManager_ = std::make_unique<ShaderManager>();
    fontRenderer_ = std::make_unique<FontRenderer>();
    textureManager_->initialize();
    shaderManager_->initialize();
    fontRenderer_->initialize();
    if (backend_) {
        backend_->initialize();
    }
    return true;
}

void RenderEngine::shutdown() {
    if (fontRenderer_) fontRenderer_->shutdown();
    if (shaderManager_) shaderManager_->shutdown();
    if (textureManager_) textureManager_->shutdown();
    if (backend_) backend_->shutdown();
    fontRenderer_.reset();
    shaderManager_.reset();
    textureManager_.reset();
    backend_.reset();
    commandBuffer_.clear();
    clipStack_.clear();
    currentPath_.clear();
    transformStack_.assign(1, Mat4::identity());
}

void RenderEngine::beginFrame() {
    commandBuffer_.clear();
    clipStack_.clear();
    transformStack_.assign(1, Mat4::identity());
    stats_.drawCalls = stats_.triangles = stats_.vertices = stats_.textureBinds = 0;
    if (backend_) backend_->beginFrame();
}

void RenderEngine::endFrame() {
    flushCommandBuffer();
    if (backend_) backend_->endFrame();
}

void RenderEngine::present() {
    if (backend_) backend_->present();
}

void RenderEngine::setViewport(int x, int y, int w, int h) {
    viewportWidth_ = w;
    viewportHeight_ = h;
    if (w > 0 && h > 0) {
        projection_ = Mat4::ortho(0.0f, static_cast<float>(w), static_cast<float>(h), 0.0f, -1.0f, 1.0f);
    }
    if (backend_) backend_->setViewport(x, y, w, h);
}

void RenderEngine::setProjection(const Mat4& proj) { projection_ = proj; }

void RenderEngine::clear(const Color& color) {
    if (backend_) backend_->clear(color);
}

void RenderEngine::drawRect(const Rect& r, const Color& c) {
    DrawCommand cmd; cmd.type = DrawCommandType::DrawRect; cmd.rect = r; cmd.color = c;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawRoundedRect(const Rect& r, const Color& c, float radius) {
    DrawCommand cmd; cmd.type = DrawCommandType::DrawRoundedRect; cmd.rect = r; cmd.color = c; cmd.cornerRadius = radius;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawRoundedRect(const Rect& r, const Color& c, float tl, float tr, float br, float bl) {
    DrawCommand cmd;
    cmd.type = DrawCommandType::DrawRoundedRect;
    cmd.rect = r;
    cmd.color = c;
    cmd.cornerRadius = std::max({tl, tr, br, bl});
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawCircle(float cx, float cy, float rad, const Color& c, bool filled) {
    DrawCommand cmd; cmd.type = DrawCommandType::DrawCircle; cmd.rect = Rect(cx - rad, cy - rad, rad * 2, rad * 2);
    cmd.color = c; cmd.filled = filled; commandBuffer_.push_back(cmd);
}

void RenderEngine::drawLine(float x1, float y1, float x2, float y2, const Color& c, float thick) {
    DrawCommand cmd; cmd.type = DrawCommandType::DrawLine; cmd.rect = Rect(x1, y1, x2 - x1, y2 - y1);
    cmd.color = c; cmd.thickness = thick; commandBuffer_.push_back(cmd);
}

void RenderEngine::drawTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, const Color& c, bool filled) {
    DrawCommand cmd; cmd.type = DrawCommandType::DrawTriangle; cmd.pathPoints = {p1, p2, p3};
    cmd.color = c; cmd.filled = filled; commandBuffer_.push_back(cmd);
}

void RenderEngine::drawGradientRect(const Rect& r, const Color& top, const Color& bottom) {
    DrawCommand cmd; cmd.type = DrawCommandType::DrawGradient; cmd.rect = r; cmd.color = top; cmd.color2 = bottom;
    cmd.gradient.type = GradientType::Linear; cmd.gradient.stops = {{0, top}, {1, bottom}};
    cmd.gradient.startPoint = {r.x, r.y}; cmd.gradient.endPoint = {r.x, r.y + r.height}; cmd.useGradient = true;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawHorizontalGradient(const Rect& r, const Color& left, const Color& right) {
    DrawCommand cmd; cmd.type = DrawCommandType::DrawGradient; cmd.rect = r; cmd.color = left; cmd.color2 = right;
    cmd.gradient.type = GradientType::Linear; cmd.gradient.stops = {{0, left}, {1, right}};
    cmd.gradient.startPoint = {r.x, r.y}; cmd.gradient.endPoint = {r.x + r.width, r.y}; cmd.useGradient = true;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawRadialGradient(const Rect& r, const Color& center, const Color& edge) {
    DrawCommand cmd; cmd.type = DrawCommandType::DrawGradient; cmd.rect = r; cmd.color = center; cmd.color2 = edge;
    cmd.gradient.type = GradientType::Radial; cmd.gradient.stops = {{0, center}, {1, edge}};
    cmd.gradient.startPoint = {r.x + r.width / 2, r.y + r.height / 2};
    cmd.gradient.radius = std::max(r.width, r.height) / 2; cmd.useGradient = true;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawMultiGradient(const Rect& r, const Gradient& g) {
    DrawCommand cmd; cmd.type = DrawCommandType::DrawGradient; cmd.rect = r; cmd.gradient = g; cmd.useGradient = true;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawText(const std::string& text, float x, float y, const Color& c, const std::string& font, float size) {
    DrawCommand cmd; cmd.type = DrawCommandType::DrawText; cmd.rect = Rect(x, y, 0, 0);
    cmd.text = text; cmd.color = c; cmd.fontFamily = font; cmd.fontSize = size;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawText(const std::string& text, float x, float y, float maxW, const Color& c, const std::string& font, float size) {
    DrawCommand cmd; cmd.type = DrawCommandType::DrawText; cmd.rect = Rect(x, y, maxW, 0);
    cmd.text = text; cmd.color = c; cmd.fontFamily = font; cmd.fontSize = size;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawTextCentered(const std::string& text, const Rect& r, const Color& c, const std::string& font, float size) {
    Vec2 ts = measureText(text, font, size);
    float x = r.x + (r.width - ts.x) / 2;
    float y = r.y + (r.height - ts.y) / 2;
    drawText(text, x, y, c, font, size);
}

void RenderEngine::drawImage(TextureHandle tex, const Rect& destRect) {
    DrawCommand cmd; cmd.type = DrawCommandType::DrawImage; cmd.rect = destRect; cmd.texture = tex;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawImage(TextureHandle tex, const Rect& destRect, const Rect& srcRect) {
    DrawCommand cmd; cmd.type = DrawCommandType::DrawImage; cmd.rect = destRect; cmd.texture = tex; cmd.srcRect = srcRect;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawImage(TextureHandle tex, const Rect& destRect, const Color& tint) {
    DrawCommand cmd; cmd.type = DrawCommandType::DrawImage; cmd.rect = destRect; cmd.texture = tex; cmd.color = tint;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawNineSlice(TextureHandle texture, const Rect& destRect,
                                  float borderLeft, float borderTop, float borderRight, float borderBottom) {
    Vec2 texSize = getTextureSize(texture);
    if (texSize.x <= 0.0f || texSize.y <= 0.0f) {
        drawImage(texture, destRect);
        return;
    }

    float midSrcW = std::max(0.0f, texSize.x - borderLeft - borderRight);
    float midSrcH = std::max(0.0f, texSize.y - borderTop - borderBottom);
    float midDstW = std::max(0.0f, destRect.width - borderLeft - borderRight);
    float midDstH = std::max(0.0f, destRect.height - borderTop - borderBottom);

    float sx[4] = {0.0f, borderLeft, borderLeft + midSrcW, texSize.x};
    float sy[4] = {0.0f, borderTop, borderTop + midSrcH, texSize.y};
    float dx[4] = {destRect.x, destRect.x + borderLeft, destRect.x + borderLeft + midDstW, destRect.right()};
    float dy[4] = {destRect.y, destRect.y + borderTop, destRect.y + borderTop + midDstH, destRect.bottom()};

    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            Rect dst(dx[col], dy[row], dx[col + 1] - dx[col], dy[row + 1] - dy[row]);
            Rect src(sx[col], sy[row], sx[col + 1] - sx[col], sy[row + 1] - sy[row]);
            if (dst.width > 0.0f && dst.height > 0.0f && src.width > 0.0f && src.height > 0.0f) {
                drawImage(texture, dst, src);
            }
        }
    }
}

// Path drawing
void RenderEngine::beginPath() {
    currentPath_.clear();
    pathStartPoint_ = Vec2(0.0f, 0.0f);
}

void RenderEngine::moveTo(float x, float y) {
    pathStartPoint_ = Vec2(x, y);
    currentPath_.push_back(pathStartPoint_);
}

void RenderEngine::lineTo(float x, float y) {
    if (currentPath_.empty()) {
        pathStartPoint_ = Vec2(x, y);
    }
    currentPath_.push_back(Vec2(x, y));
}

void RenderEngine::bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y) {
    Vec2 p0 = currentPath_.empty() ? Vec2(0.0f, 0.0f) : currentPath_.back();
    Vec2 p1(c1x, c1y);
    Vec2 p2(c2x, c2y);
    Vec2 p3(x, y);
    constexpr int segments = 16;
    for (int i = 1; i <= segments; ++i) {
        float t = static_cast<float>(i) / segments;
        float u = 1.0f - t;
        float bx = u * u * u * p0.x + 3.0f * u * u * t * p1.x + 3.0f * u * t * t * p2.x + t * t * t * p3.x;
        float by = u * u * u * p0.y + 3.0f * u * u * t * p1.y + 3.0f * u * t * t * p2.y + t * t * t * p3.y;
        currentPath_.push_back(Vec2(bx, by));
    }
}

void RenderEngine::quadTo(float cx, float cy, float x, float y) {
    Vec2 p0 = currentPath_.empty() ? Vec2(0.0f, 0.0f) : currentPath_.back();
    Vec2 p1(cx, cy);
    Vec2 p2(x, y);
    constexpr int segments = 12;
    for (int i = 1; i <= segments; ++i) {
        float t = static_cast<float>(i) / segments;
        float u = 1.0f - t;
        float qx = u * u * p0.x + 2.0f * u * t * p1.x + t * t * p2.x;
        float qy = u * u * p0.y + 2.0f * u * t * p1.y + t * t * p2.y;
        currentPath_.push_back(Vec2(qx, qy));
    }
}

void RenderEngine::arcTo(float x, float y, float radius, float startAngle, float endAngle) {
    constexpr int segments = 24;
    for (int i = 0; i <= segments; ++i) {
        float t = static_cast<float>(i) / segments;
        float angle = startAngle + (endAngle - startAngle) * t;
        currentPath_.push_back(Vec2(x + std::cos(angle) * radius, y + std::sin(angle) * radius));
    }
}

void RenderEngine::closePath() {
    if (!currentPath_.empty()) {
        currentPath_.push_back(pathStartPoint_);
    }
}

void RenderEngine::strokePath(const Color& color, float thickness) {
    if (currentPath_.size() < 2) return;
    drawPath(currentPath_, color, thickness, false);
}

void RenderEngine::fillPath(const Color& color) {
    if (currentPath_.size() < 3) return;
    drawPath(currentPath_, color, 1.0f, true);
}

void RenderEngine::drawPath(const std::vector<Vec2>& pts, const Color& c, float thick, bool filled) {
    DrawCommand cmd; cmd.type = DrawCommandType::DrawPath; cmd.pathPoints = pts;
    cmd.color = c; cmd.thickness = thick; cmd.filled = filled; commandBuffer_.push_back(cmd);
}

void RenderEngine::drawBezier(const std::vector<Vec2>& pts, const Color& c, float thick) {
    DrawCommand cmd; cmd.type = DrawCommandType::DrawPath; cmd.pathPoints = pts;
    cmd.color = c; cmd.thickness = thick; commandBuffer_.push_back(cmd);
}

void RenderEngine::pushClipRect(const Rect& rect) {
    Rect effective = clipStack_.empty() ? rect : clipStack_.back().intersection(rect);
    clipStack_.push_back(effective);
    setClipRect(effective);
}

void RenderEngine::popClipRect() {
    if (!clipStack_.empty()) {
        clipStack_.pop_back();
    }
    if (clipStack_.empty()) {
        clearClipRect();
    } else {
        setClipRect(clipStack_.back());
    }
}

void RenderEngine::setClipRect(const Rect& r) {
    DrawCommand cmd;
    cmd.type = DrawCommandType::SetClipRect;
    cmd.rect = r;
    cmd.clipRect = r;
    cmd.hasClip = true;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::clearClipRect() {
    DrawCommand cmd;
    cmd.type = DrawCommandType::ClearClipRect;
    cmd.hasClip = false;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::pushTransform() {
    Mat4 current = transformStack_.empty() ? Mat4::identity() : transformStack_.back();
    transformStack_.push_back(current);
    DrawCommand cmd; cmd.type = DrawCommandType::PushTransform; cmd.transform = current;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::popTransform() {
    if (transformStack_.size() > 1) {
        transformStack_.pop_back();
    }
    DrawCommand cmd; cmd.type = DrawCommandType::PopTransform;
    if (!transformStack_.empty()) cmd.transform = transformStack_.back();
    commandBuffer_.push_back(cmd);
}

void RenderEngine::translate(float x, float y) {
    if (transformStack_.empty()) transformStack_.push_back(Mat4::identity());
    transformStack_.back() = transformStack_.back() * Mat4::translate(x, y, 0.0f);
    setTransform(transformStack_.back());
}

void RenderEngine::rotate(float angle) {
    if (transformStack_.empty()) transformStack_.push_back(Mat4::identity());
    transformStack_.back() = transformStack_.back() * Mat4::rotateZ(angle);
    setTransform(transformStack_.back());
}

void RenderEngine::scale(float x, float y) {
    if (transformStack_.empty()) transformStack_.push_back(Mat4::identity());
    transformStack_.back() = transformStack_.back() * Mat4::scale(x, y, 1.0f);
    setTransform(transformStack_.back());
}

void RenderEngine::setTransform(const Mat4& t) {
    if (transformStack_.empty()) transformStack_.push_back(t);
    else transformStack_.back() = t;
    DrawCommand cmd; cmd.type = DrawCommandType::SetTransform; cmd.transform = t;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::setBlendMode(BlendMode mode) {
    DrawCommand cmd; cmd.type = DrawCommandType::SetBlendMode; cmd.blendMode = mode;
    commandBuffer_.push_back(cmd);
}

Vec2 RenderEngine::measureText(const std::string& text, const std::string& font, float size) {
    if (fontRenderer_) {
        if (!font.empty()) fontRenderer_->setCurrentFont(font);
        return fontRenderer_->measureText(text, size);
    }
    return Vec2(text.length() * size * 0.5f, size);
}

TextureHandle RenderEngine::createTexture(int w, int h, const void* data, bool genMips) {
    return textureManager_ ? textureManager_->createTexture(w, h, data, genMips) : 0;
}

TextureHandle RenderEngine::loadTexture(const std::string& path) {
    return textureManager_ ? textureManager_->loadTexture(path) : 0;
}

void RenderEngine::destroyTexture(TextureHandle tex) { if (textureManager_) textureManager_->destroyTexture(tex); }
Vec2 RenderEngine::getTextureSize(TextureHandle tex) { return textureManager_ ? textureManager_->getTextureSize(tex) : Vec2(0, 0); }

bool RenderEngine::loadFont(const std::string& name, const std::string& path) {
    return fontRenderer_ ? fontRenderer_->loadFont(name, path) : false;
}

bool RenderEngine::loadFontFromMemory(const std::string& name, const void* data, size_t size) {
    return fontRenderer_ ? fontRenderer_->loadFontFromMemory(name, static_cast<const uint8_t*>(data), size) : false;
}

void RenderEngine::executeCommand(const DrawCommand& cmd) {
    if (backend_) {
        backend_->draw(cmd);
    }
}

RenderBackend* RenderEngine::createBackend(RenderAPI api) {
    (void)api;
    return backend_.get();
}

void RenderEngine::flushCommandBuffer() {
    stats_.drawCalls = commandBuffer_.size();
    for (const auto& cmd : commandBuffer_) {
        executeCommand(cmd);
    }
}

void RenderEngine::setBackend(std::unique_ptr<RenderBackend> backend) { backend_ = std::move(backend); }

} // namespace NexusForge::UI
