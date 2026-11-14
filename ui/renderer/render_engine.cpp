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
    uint32_t value = std::stoul(hex.substr(1), nullptr, 16);
    if (hex.length() == 7) {
        return Color(((value >> 16) & 0xFF)/255.0f, ((value >> 8) & 0xFF)/255.0f, (value & 0xFF)/255.0f, 1.0f);
    } else if (hex.length() == 9) {
        return Color(((value >> 24) & 0xFF)/255.0f, ((value >> 16) & 0xFF)/255.0f, ((value >> 8) & 0xFF)/255.0f, (value & 0xFF)/255.0f);
    }
    return Color();
}

uint32_t Color::toUint32() const {
    return (static_cast<uint32_t>(r * 255) << 24) | (static_cast<uint32_t>(g * 255) << 16) |
           (static_cast<uint32_t>(b * 255) << 8) | static_cast<uint32_t>(a * 255);
}

float Vec2::length() const { return std::sqrt(x * x + y * y); }
Vec2 Vec2::normalized() const { float len = length(); return len > 0 ? Vec2(x/len, y/len) : *this; }

bool Rect::intersects(const Rect& o) const {
    return !(o.left() >= right() || o.right() <= left() || o.top() >= bottom() || o.bottom() <= top());
}

Rect Rect::intersection(const Rect& o) const {
    Rect r; r.x = std::max(x, o.x); r.y = std::max(y, o.y);
    r.width = std::min(right(), o.right()) - r.x; r.height = std::min(bottom(), o.bottom()) - r.y;
    return (r.width > 0 && r.height > 0) ? r : Rect();
}

Mat4::Mat4() { std::memset(m, 0, sizeof(m)); m[0]=m[5]=m[10]=m[15]=1; }
Mat4 Mat4::identity() { return Mat4(); }
Mat4 Mat4::ortho(float l, float r, float b, float t, float n, float f) {
    Mat4 res; res.m[0]=2.0f/(r-l); res.m[5]=2.0f/(t-b); res.m[10]=-2.0f/(f-n);
    res.m[12]=-(r+l)/(r-l); res.m[13]=-(t+b)/(t-b); res.m[14]=-(f+n)/(f-n); return res;
}
Mat4 Mat4::translate(float x, float y, float z) {
    Mat4 res; res.m[12]=x; res.m[13]=y; res.m[14]=z; return res;
}
Mat4 Mat4::scale(float x, float y, float z) {
    Mat4 res; res.m[0]=x; res.m[5]=y; res.m[10]=z; return res;
}
Mat4 Mat4::rotateZ(float angle) {
    Mat4 res; float c=std::cos(angle), s=std::sin(angle);
    res.m[0]=c; res.m[1]=s; res.m[4]=-s; res.m[5]=c; return res;
}
Mat4 Mat4::operator*(const Mat4& o) const {
    Mat4 res; for(int i=0;i<4;++i) for(int j=0;j<4;++j) { res.m[i*4+j]=0;
        for(int k=0;k<4;++k) res.m[i*4+j]+=m[k*4+j]*o.m[i*4+k]; } return res;
}

// TextureManager implementation
TextureManager::TextureManager() = default;
TextureManager::~TextureManager() { shutdown(); }

bool TextureManager::initialize() { return true; }
void TextureManager::shutdown() { textures_.clear(); pathToHandle_.clear(); }

TextureHandle TextureManager::createTexture(int width, int height, const void* data, bool genMips) {
    TextureHandle handle = nextHandle_++;
    auto info = std::make_unique<TextureInfo>();
    info->handle = handle; info->width = width; info->height = height;
    info->loaded = true; info->hasAlpha = (data == nullptr);
    textures_[handle] = std::move(info);
    memoryUsage_ += width * height * 4;
    return handle;
}

TextureHandle TextureManager::createTextureFromColor(int w, int h, const Color& c) {
    std::vector<uint8_t> pixels(w * h * 4);
    uint32_t color = c.toUint32();
    for (int i = 0; i < w * h; ++i) {
        reinterpret_cast<uint32_t*>(pixels.data())[i] = color;
    }
    return createTexture(w, h, pixels.data(), false);
}

TextureHandle TextureManager::loadTexture(const std::string& path) {
    auto it = pathToHandle_.find(path);
    if (it != pathToHandle_.end()) return it->second;
    TextureHandle handle = nextHandle_++;
    auto info = std::make_unique<TextureInfo>();
    info->handle = handle; info->path = path; info->loaded = false;
    if (loadTextureData(info.get())) {
        pathToHandle_[path] = handle;
        textures_[handle] = std::move(info);
        return handle;
    }
    return 0;
}

TextureHandle TextureManager::loadTextureAsync(const std::string& path) {
    return loadTexture(path);
}

void TextureManager::destroyTexture(TextureHandle handle) {
    auto it = textures_.find(handle);
    if (it != textures_.end()) {
        memoryUsage_ -= it->second->width * it->second->height * 4;
        textures_.erase(it);
    }
}

void TextureManager::unloadAllTextures() { textures_.clear(); pathToHandle_.clear(); }

TextureInfo* TextureManager::getTextureInfo(TextureHandle handle) {
    auto it = textures_.find(handle);
    return (it != textures_.end()) ? it->second.get() : nullptr;
}

Vec2 TextureManager::getTextureSize(TextureHandle handle) {
    auto info = getTextureInfo(handle);
    return info ? Vec2(static_cast<float>(info->width), static_cast<float>(info->height)) : Vec2(0,0);
}

bool TextureManager::isTextureLoaded(TextureHandle handle) const {
    auto it = textures_.find(handle);
    return (it != textures_.end()) && it->second->loaded;
}

bool TextureManager::loadTextureData(TextureInfo* info) {
    info->width = 64; info->height = 64; info->loaded = true;
    memoryUsage_ += 64 * 64 * 4; return true;
}

void TextureManager::calculateMemoryUsage() {
    size_t total = 0;
    for (const auto& [h, info] : textures_) total += info->width * info->height * 4;
    memoryUsage_ = total;
}

// ShaderManager implementation
ShaderManager::ShaderManager() = default;
ShaderManager::~ShaderManager() { shutdown(); }

bool ShaderManager::initialize() { createBuiltInShaders(); return true; }
void ShaderManager::shutdown() { shaders_.clear(); }

ShaderHandle ShaderManager::createShader(const std::string& name, const std::string& vertSrc, const std::string& fragSrc) {
    ShaderHandle handle = nextHandle_++;
    auto prog = std::make_unique<ShaderProgram>();
    prog->handle = handle; prog->name = name;
    prog->vertexSource = vertSrc; prog->fragmentSource = fragSrc;
    prog->compiled = true;
    shaders_[handle] = std::move(prog);
    return handle;
}

ShaderHandle ShaderManager::loadShader(const std::string& path) {
    return createShader(path,
        "#version 330 core\nlayout(location=0) in vec2 pos;\nvoid main(){gl_Position=vec4(pos,0,1);}",
        "#version 330 core\nout vec4 color;\nvoid main(){color=vec4(1,0,0,1);}");
}

void ShaderManager::destroyShader(ShaderHandle handle) { shaders_.erase(handle); }

bool ShaderManager::useShader(ShaderHandle handle) {
    if (shaders_.find(handle) != shaders_.end()) { currentShader_ = handle; return true; }
    return false;
}

void ShaderManager::setUniform1i(const std::string& name, int value) {}
void ShaderManager::setUniform1f(const std::string& name, float value) {}
void ShaderManager::setUniform2f(const std::string& name, float x, float y) {}
void ShaderManager::setUniform3f(const std::string& name, float x, float y, float z) {}
void ShaderManager::setUniform4f(const std::string& name, float x, float y, float z, float w) {}
void ShaderManager::setUniformMatrix4(const std::string& name, const Mat4& matrix) {}

void ShaderManager::createBuiltInShaders() {
    solidColorShader_ = createShader("solid_color",
        "#version 330\nlayout(location=0) in vec2 pos;\nuniform mat4 proj;\nvoid main(){gl_Position=proj*vec4(pos,0,1);}",
        "#version 330\nuniform vec4 color;\nout vec4 fragColor;\nvoid main(){fragColor=color;}");
    textureShader_ = createShader("texture",
        "#version 330\nlayout(location=0) in vec2 pos;layout(location=1) in vec2 tex;\nout vec2 texCoord;\nuniform mat4 proj;\nvoid main(){gl_Position=proj*vec4(pos,0,1);texCoord=tex;}",
        "#version 330\nuniform sampler2D tex;in vec2 texCoord;out vec4 fragColor;\nvoid main(){fragColor=texture(tex,texCoord);}");
    gradientShader_ = createShader("gradient",
        "#version 330\nlayout(location=0) in vec2 pos;out vec2 vPos;\nuniform mat4 proj;\nvoid main(){gl_Position=proj*vec4(pos,0,1);vPos=pos;}",
        "#version 330\nuniform vec4 color1,color2;in vec2 vPos;out vec4 fragColor;\nvoid main(){fragColor=mix(color1,color2,vPos.y);}");
    textShader_ = createShader("text",
        "#version 330\nlayout(location=0) in vec2 pos;layout(location=1) in vec2 tex;\nout vec2 texCoord;\nuniform mat4 proj;\nvoid main(){gl_Position=proj*vec4(pos,0,1);texCoord=tex;}",
        "#version 330\nuniform sampler2D tex;in vec2 texCoord;out vec4 fragColor;\nvoid main(){fragColor=texture(tex,texCoord);}");
}

// FontRenderer implementation
FontRenderer::FontRenderer() = default;
FontRenderer::~FontRenderer() { shutdown(); }

bool FontRenderer::initialize() {
    loadFont("default", "", 14.0f);
    return true;
}

void FontRenderer::shutdown() { fonts_.clear(); currentFont_ = nullptr; }

bool FontRenderer::loadFont(const std::string& name, const std::string& path, float size) {
    auto face = std::make_unique<FontFace>();
    face->name = name; face->size = size;
    face->lineHeight = size * 1.2f; face->ascent = size * 0.8f; face->descent = size * 0.2f;
    for (uint32_t c = 32; c < 128; ++c) loadGlyph(face.get(), c);
    fonts_[name] = std::move(face);
    currentFont_ = fonts_[name].get();
    return true;
}

bool FontRenderer::loadFontFromMemory(const std::string& name, const uint8_t* data, size_t size, float fontSize) {
    return loadFont(name, "", fontSize);
}

void FontRenderer::setCurrentFont(const std::string& name) {
    auto it = fonts_.find(name);
    if (it != fonts_.end()) currentFont_ = it->second.get();
}

Vec2 FontRenderer::measureText(const std::string& text, float fontSize) {
    float width = 0; float scale = fontSize / (currentFont_ ? currentFont_->size : 14.0f);
    for (char c : text) {
        if (c == '\n') continue;
        auto it = (currentFont_ ? currentFont_->glyphs : std::unordered_map<uint32_t,Glyph>{}) .find(c);
        width += (it != (currentFont_ ? currentFont_->glyphs : std::unordered_map<uint32_t,Glyph>{}).end()) ? it->second.advance : fontSize * 0.5f;
    }
    return Vec2(width * scale, fontSize);
}

float FontRenderer::measureTextWidth(const std::string& text, float fontSize) {
    return measureText(text, fontSize).x;
}

float FontRenderer::measureTextHeight(float fontSize) { return fontSize; }

void FontRenderer::drawText(const std::string& text, float x, float y, const Color& color) {
    drawText(text, x, y, 0, color);
}

void FontRenderer::drawText(const std::string& text, float x, float y, float maxWidth, const Color& color) {}
void FontRenderer::drawTextCentered(const std::string& text, const Rect& rect, const Color& color) {}

std::vector<std::string> FontRenderer::wrapText(const std::string& text, float maxWidth) {
    return {text};
}

Glyph* FontRenderer::getGlyph(uint32_t codepoint) {
    if (!currentFont_) return nullptr;
    auto it = currentFont_->glyphs.find(codepoint);
    return (it != currentFont_->glyphs.end()) ? &it->second : nullptr;
}

bool FontRenderer::loadGlyph(FontFace* font, uint32_t codepoint) {
    Glyph g; g.codepoint = codepoint; g.x = 0; g.y = 0;
    g.width = font->size * 0.5f; g.height = font->size;
    g.bearingX = 0; g.bearingY = font->size * 0.8f;
    g.advance = font->size * 0.5f; g.texture = 0;
    font->glyphs[codepoint] = g;
    return true;
}

void FontRenderer::generateFontTexture(FontFace* font) {}

// RenderEngine implementation
RenderEngine::RenderEngine() = default;
RenderEngine::~RenderEngine() { shutdown(); }

bool RenderEngine::initialize(RenderAPI api) {
    currentAPI_ = api;
    textureManager_ = std::make_unique<TextureManager>();
    shaderManager_ = std::make_unique<ShaderManager>();
    fontRenderer_ = std::make_unique<FontRenderer>();
    textureManager_->initialize();
    shaderManager_->initialize();
    fontRenderer_->initialize();
    return true;
}

void RenderEngine::shutdown() {
    if (fontRenderer_) fontRenderer_->shutdown();
    if (shaderManager_) shaderManager_->shutdown();
    if (textureManager_) textureManager_->shutdown();
    fontRenderer_.reset(); shaderManager_.reset(); textureManager_.reset();
    backend_.reset();
}

void RenderEngine::beginFrame() { commandBuffer_.clear(); stats_.drawCalls=stats_.triangles=stats_.vertices=0; }
void RenderEngine::endFrame() { flushCommandBuffer(); }
void RenderEngine::present() {}
void RenderEngine::setViewport(int x, int y, int w, int h) { viewportWidth_=w; viewportHeight_=h; }
void RenderEngine::setProjection(const Mat4& proj) { projection_ = proj; }
void RenderEngine::clear(const Color& color) {}

void RenderEngine::drawRect(const Rect& r, const Color& c) {
    DrawCommand cmd; cmd.type=DrawCommandType::DrawRect; cmd.rect=r; cmd.color=c;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawRoundedRect(const Rect& r, const Color& c, float radius) {
    DrawCommand cmd; cmd.type=DrawCommandType::DrawRoundedRect; cmd.rect=r; cmd.color=c; cmd.cornerRadius=radius;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawRoundedRect(const Rect& r, const Color& c, float tl, float tr, float br, float bl) {
    DrawCommand cmd; cmd.type=DrawCommandType::DrawRoundedRect; cmd.rect=r; cmd.color=c; cmd.cornerRadius=tl;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawCircle(float cx, float cy, float rad, const Color& c, bool filled) {
    DrawCommand cmd; cmd.type=DrawCommandType::DrawCircle; cmd.rect=Rect(cx-rad,cy-rad,rad*2,rad*2);
    cmd.color=c; cmd.filled=filled; commandBuffer_.push_back(cmd);
}

void RenderEngine::drawLine(float x1, float y1, float x2, float y2, const Color& c, float thick) {
    DrawCommand cmd; cmd.type=DrawCommandType::DrawLine; cmd.rect=Rect(x1,y1,x2-x1,y2-y1);
    cmd.color=c; cmd.thickness=thick; commandBuffer_.push_back(cmd);
}

void RenderEngine::drawTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, const Color& c, bool filled) {
    DrawCommand cmd; cmd.type=DrawCommandType::DrawTriangle; cmd.pathPoints={p1,p2,p3};
    cmd.color=c; cmd.filled=filled; commandBuffer_.push_back(cmd);
}

void RenderEngine::drawGradientRect(const Rect& r, const Color& top, const Color& bottom) {
    DrawCommand cmd; cmd.type=DrawCommandType::DrawGradient; cmd.rect=r; cmd.color=top; cmd.color2=bottom;
    cmd.gradient.type=GradientType::Linear; cmd.gradient.stops={{0,top},{1,bottom}};
    cmd.gradient.startPoint={r.x,r.y}; cmd.gradient.endPoint={r.x,r.y+r.height}; cmd.useGradient=true;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawHorizontalGradient(const Rect& r, const Color& left, const Color& right) {
    DrawCommand cmd; cmd.type=DrawCommandType::DrawGradient; cmd.rect=r; cmd.color=left; cmd.color2=right;
    cmd.gradient.type=GradientType::Linear; cmd.gradient.stops={{0,left},{1,right}};
    cmd.gradient.startPoint={r.x,r.y}; cmd.gradient.endPoint={r.x+r.width,r.y}; cmd.useGradient=true;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawRadialGradient(const Rect& r, const Color& center, const Color& edge) {
    DrawCommand cmd; cmd.type=DrawCommandType::DrawGradient; cmd.rect=r; cmd.color=center; cmd.color2=edge;
    cmd.gradient.type=GradientType::Radial; cmd.gradient.stops={{0,center},{1,edge}};
    cmd.gradient.startPoint={r.x+r.width/2,r.y+r.height/2};
    cmd.gradient.radius=std::max(r.width,r.height)/2; cmd.useGradient=true;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawMultiGradient(const Rect& r, const Gradient& g) {
    DrawCommand cmd; cmd.type=DrawCommandType::DrawGradient; cmd.rect=r; cmd.gradient=g; cmd.useGradient=true;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawText(const std::string& text, float x, float y, const Color& c, const std::string& font, float size) {
    DrawCommand cmd; cmd.type=DrawCommandType::DrawText; cmd.rect=Rect(x,y,0,0);
    cmd.text=text; cmd.color=c; cmd.fontFamily=font; cmd.fontSize=size;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawText(const std::string& text, float x, float y, float maxW, const Color& c, const std::string& font, float size) {
    DrawCommand cmd; cmd.type=DrawCommandType::DrawText; cmd.rect=Rect(x,y,maxW,0);
    cmd.text=text; cmd.color=c; cmd.fontFamily=font; cmd.fontSize=size;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawTextCentered(const std::string& text, const Rect& r, const Color& c, const std::string& font, float size) {
    Vec2 ts = measureText(text, font, size);
    float x = r.x + (r.width - ts.x) / 2; float y = r.y + (r.height - ts.y) / 2;
    drawText(text, x, y, c, font, size);
}

void RenderEngine::drawImage(TextureHandle tex, const Rect& r) {
    DrawCommand cmd; cmd.type=DrawCommandType::DrawImage; cmd.rect=r; cmd.texture=tex;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawImage(TextureHandle tex, const Rect& src, const Rect& dst) {
    DrawCommand cmd; cmd.type=DrawCommandType::DrawImage; cmd.rect=dst; cmd.texture=tex; cmd.srcRect=src;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::drawPath(const std::vector<Vec2>& pts, const Color& c, float thick, bool filled) {
    DrawCommand cmd; cmd.type=DrawCommandType::DrawPath; cmd.pathPoints=pts;
    cmd.color=c; cmd.thickness=thick; cmd.filled=filled; commandBuffer_.push_back(cmd);
}

void RenderEngine::drawBezier(const std::vector<Vec2>& pts, const Color& c, float thick) {
    DrawCommand cmd; cmd.type=DrawCommandType::DrawPath; cmd.pathPoints=pts;
    cmd.color=c; cmd.thickness=thick; commandBuffer_.push_back(cmd);
}

void RenderEngine::setClipRect(const Rect& r) {
    DrawCommand cmd; cmd.type=DrawCommandType::SetClipRect; cmd.rect=r; cmd.hasClip=true;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::clearClipRect() {
    DrawCommand cmd; cmd.type=DrawCommandType::ClearClipRect; commandBuffer_.push_back(cmd);
}

void RenderEngine::pushTransform() {
    DrawCommand cmd; cmd.type=DrawCommandType::PushTransform; commandBuffer_.push_back(cmd);
}

void RenderEngine::popTransform() {
    DrawCommand cmd; cmd.type=DrawCommandType::PopTransform; commandBuffer_.push_back(cmd);
}

void RenderEngine::setTransform(const Mat4& t) {
    DrawCommand cmd; cmd.type=DrawCommandType::SetTransform; cmd.transform=t;
    commandBuffer_.push_back(cmd);
}

void RenderEngine::setBlendMode(BlendMode mode) {
    DrawCommand cmd; cmd.type=DrawCommandType::SetBlendMode; cmd.blendMode=mode;
    commandBuffer_.push_back(cmd);
}

Vec2 RenderEngine::measureText(const std::string& text, const std::string& font, float size) {
    if (fontRenderer_) return fontRenderer_->measureText(text, size);
    return Vec2(text.length() * size * 0.5f, size);
}

TextureHandle RenderEngine::createTexture(int w, int h, const void* data, bool genMips) {
    return textureManager_ ? textureManager_->createTexture(w, h, data, genMips) : 0;
}

TextureHandle RenderEngine::loadTexture(const std::string& path) {
    return textureManager_ ? textureManager_->loadTexture(path) : 0;
}

void RenderEngine::destroyTexture(TextureHandle tex) { if (textureManager_) textureManager_->destroyTexture(tex); }
Vec2 RenderEngine::getTextureSize(TextureHandle tex) { return textureManager_ ? textureManager_->getTextureSize(tex) : Vec2(0,0); }

bool RenderEngine::loadFont(const std::string& name, const std::string& path) {
    return fontRenderer_ ? fontRenderer_->loadFont(name, path) : false;
}

bool RenderEngine::loadFontFromMemory(const std::string& name, const void* data, size_t size) {
    return fontRenderer_ ? fontRenderer_->loadFontFromMemory(name, static_cast<const uint8_t*>(data), size) : false;
}

void RenderEngine::flushCommandBuffer() {
    stats_.drawCalls = commandBuffer_.size();
    for (const auto& cmd : commandBuffer_) {
        if (backend_) backend_->draw(cmd);
    }
}

void RenderEngine::setBackend(std::unique_ptr<RenderBackend> backend) { backend_ = std::move(backend); }

} // namespace NexusForge::UI
