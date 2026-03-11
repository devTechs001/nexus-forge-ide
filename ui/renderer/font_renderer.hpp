// ui/renderer/font_renderer.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace NexusForge::UI {

class RenderEngine;
struct Vec2;
struct Color;

// Font metrics
struct FontMetrics {
    float ascent = 0;
    float descent = 0;
    float lineGap = 0;
    float xHeight = 0;
    float capHeight = 0;
    float underlinePosition = 0;
    float underlineThickness = 0;
};

// Glyph metrics
struct GlyphMetrics {
    uint32_t codepoint = 0;
    float bearingX = 0;
    float bearingY = 0;
    float advance = 0;
    float width = 0;
    float height = 0;
    float offsetX = 0;
    float offsetY = 0;
    uint64_t textureHandle = 0;
};

// Font face
class FontFace {
public:
    FontFace();
    ~FontFace();

    bool loadFromFile(const std::string& path);
    bool loadFromMemory(const void* data, size_t size);
    void unload();

    bool isLoaded() const { return loaded_; }
    const std::string& getName() const { return name_; }
    const std::string& getFamily() const { return family_; }
    int getPointSize() const { return pointSize_; }

    // Metrics
    FontMetrics getMetrics() const { return metrics_; }
    GlyphMetrics getGlyphMetrics(uint32_t codepoint) const;

    // Render glyph
    bool renderGlyph(uint32_t codepoint);
    uint64_t getGlyphTexture(uint32_t codepoint) const;

    // Check if glyph exists
    bool hasGlyph(uint32_t codepoint) const;

    // Kerning
    float getKerning(uint32_t left, uint32_t right) const;

private:
    std::string name_;
    std::string family_;
    int pointSize_ = 0;
    bool loaded_ = false;

    FontMetrics metrics_;
    std::unordered_map<uint32_t, GlyphMetrics> glyphs_;
    std::unordered_map<uint32_t, uint64_t> glyphTextures_;
    std::unordered_map<uint64_t, float> kerning_;

    void* face_ = nullptr;  // FT_Face
};

// Font manager
class FontManager {
public:
    static FontManager& getInstance();

    // Font loading
    bool loadFont(const std::string& name, const std::string& path);
    bool loadFontFromMemory(const std::string& name, const void* data, size_t size);
    void unloadFont(const std::string& name);
    void unloadAllFonts();

    // Font access
    FontFace* getFont(const std::string& name);
    const FontFace* getFont(const std::string& name) const;
    FontFace* getDefaultFont();
    FontFace* getMonospaceFont();

    // Font matching
    FontFace* matchFont(const std::string& family, float size,
                        bool bold = false, bool italic = false);

    // Fallback
    void addFallbackFont(const std::string& name);
    FontFace* getFallbackFont(uint32_t codepoint);

private:
    FontManager();
    std::unordered_map<std::string, std::unique_ptr<FontFace>> fonts_;
    std::vector<std::string> fallbackFonts_;
    FontFace* defaultFont_ = nullptr;
    FontFace* monospaceFont_ = nullptr;
};

// Font renderer
class FontRenderer {
public:
    FontRenderer();
    ~FontRenderer();

    bool initialize();
    void shutdown();

    // Font loading
    bool loadFont(const std::string& name, const std::string& path);
    bool loadFontFromMemory(const std::string& name, const void* data, size_t size);

    // Text rendering
    void drawText(RenderEngine& renderer, const std::string& text,
                  float x, float y, const Color& color,
                  const std::string& fontFamily = "default", float fontSize = 14);

    void drawTextCentered(RenderEngine& renderer, const std::string& text,
                          float x, float y, float width, const Color& color,
                          const std::string& fontFamily = "default", float fontSize = 14);

    // Measurement
    Vec2 measureText(const std::string& text, const std::string& fontFamily = "default",
                     float fontSize = 14);
    float measureTextWidth(const std::string& text, const std::string& fontFamily = "default",
                           float fontSize = 14);
    float measureTextHeight(const std::string& fontFamily = "default", float fontSize = 14);

    // Line wrapping
    std::vector<std::string> wrapText(const std::string& text, float maxWidth,
                                       const std::string& fontFamily = "default",
                                       float fontSize = 14);

    // Font access
    FontFace* getFont(const std::string& name);
    FontFace* getDefaultFont();

private:
    FontManager& fontManager_;
    struct CachedText {
        std::string text;
        std::string font;
        float fontSize;
        uint64_t texture;
        Vec2 size;
    };
    std::vector<CachedText> textCache_;
};

} // namespace NexusForge::UI
