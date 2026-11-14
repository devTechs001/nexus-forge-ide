// ui/renderer/font_renderer.hpp
#pragma once

#include "render_engine.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <cstdint>

namespace NexusForge::UI {

// Font glyph data
struct Glyph {
    uint32_t codepoint;
    float x, y, width, height;
    float bearingX, bearingY;
    float advance;
    TextureHandle texture;
};

// Font face data
struct FontFace {
    std::string name;
    float size;
    float lineHeight;
    float ascent;
    float descent;
    std::unordered_map<uint32_t, Glyph> glyphs;
};

// Font Renderer
class FontRenderer {
public:
    FontRenderer();
    ~FontRenderer();

    bool initialize();
    void shutdown();

    // Font loading
    bool loadFont(const std::string& name, const std::string& path, float size = 14.0f);
    bool loadFontFromMemory(const std::string& name, const uint8_t* data, size_t size, float fontSize = 14.0f);
    
    // Font selection
    void setCurrentFont(const std::string& name);
    FontFace* getCurrentFont() const { return currentFont_; }
    
    // Text measurement
    Vec2 measureText(const std::string& text, float fontSize = 14.0f);
    float measureTextWidth(const std::string& text, float fontSize = 14.0f);
    float measureTextHeight(float fontSize = 14.0f);
    
    // Text rendering
    void drawText(const std::string& text, float x, float y, const Color& color);
    void drawText(const std::string& text, float x, float y, float maxWidth, const Color& color);
    void drawTextCentered(const std::string& text, const Rect& rect, const Color& color);
    
    // Line wrapping
    std::vector<std::string> wrapText(const std::string& text, float maxWidth);
    
    // Glyph access
    Glyph* getGlyph(uint32_t codepoint);

    // Properties
    float getLineHeight() const { return currentFont_ ? currentFont_->lineHeight : 16.0f; }
    float getAscent() const { return currentFont_ ? currentFont_->ascent : 12.0f; }
    float getDescent() const { return currentFont_ ? currentFont_->descent : 4.0f; }

private:
    std::unordered_map<std::string, std::unique_ptr<FontFace>> fonts_;
    FontFace* currentFont_ = nullptr;
    
    bool loadGlyph(FontFace* font, uint32_t codepoint);
    void generateFontTexture(FontFace* font);
};

} // namespace NexusForge::UI
