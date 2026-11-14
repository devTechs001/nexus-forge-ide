// ui/renderer/font_renderer.cpp
#include "font_renderer.hpp"
#include <algorithm>
#include <sstream>

namespace NexusForge::UI {

FontRenderer::FontRenderer() = default;

FontRenderer::~FontRenderer() {
    shutdown();
}

bool FontRenderer::initialize() {
    // Initialize font rendering system
    // Would load default fonts
    return true;
}

void FontRenderer::shutdown() {
    fonts_.clear();
    currentFont_ = nullptr;
}

bool FontRenderer::loadFont(const std::string& name, const std::string& path, float size) {
    // Would use FreeType or similar to load font
    auto font = std::make_unique<FontFace>();
    font->name = name;
    font->size = size;
    font->lineHeight = size * 1.2f;
    font->ascent = size * 0.8f;
    font->descent = size * 0.2f;
    
    fonts_[name] = std::move(font);
    if (!currentFont_) {
        currentFont_ = fonts_[name].get();
    }
    
    return true;
}

bool FontRenderer::loadFontFromMemory(const std::string& name, const uint8_t* data, 
                                       size_t dataSize, float fontSize) {
    // Would load font from memory buffer
    return loadFont(name, "", fontSize);
}

void FontRenderer::setCurrentFont(const std::string& name) {
    auto it = fonts_.find(name);
    if (it != fonts_.end()) {
        currentFont_ = it->second.get();
    }
}

Vec2 FontRenderer::measureText(const std::string& text, float fontSize) {
    if (!currentFont_) return Vec2(0, fontSize);
    
    float width = 0;
    float scale = fontSize / currentFont_->size;
    
    for (char c : text) {
        if (c == '\n') continue;
        
        auto it = currentFont_->glyphs.find(static_cast<uint32_t>(c));
        if (it != currentFont_->glyphs.end()) {
            width += it->second.advance * scale;
        } else {
            width += fontSize * 0.5f; // Approximate width for missing glyph
        }
    }
    
    return Vec2(width, fontSize);
}

float FontRenderer::measureTextWidth(const std::string& text, float fontSize) {
    return measureText(text, fontSize).x;
}

float FontRenderer::measureTextHeight(float fontSize) {
    return fontSize;
}

void FontRenderer::drawText(const std::string& text, float x, float y, const Color& color) {
    if (!currentFont_) return;
    
    float cursorX = x;
    float cursorY = y;
    float scale = currentFont_->size > 0 ? 14.0f / currentFont_->size : 1.0f;
    
    for (char c : text) {
        if (c == '\n') {
            cursorX = x;
            cursorY += currentFont_->lineHeight * scale;
            continue;
        }
        
        auto it = currentFont_->glyphs.find(static_cast<uint32_t>(c));
        if (it != currentFont_->glyphs.end()) {
            const Glyph& glyph = it->second;
            
            float glyphX = cursorX + glyph.bearingX * scale;
            float glyphY = cursorY - glyph.bearingY * scale;
            float glyphW = glyph.width * scale;
            float glyphH = glyph.height * scale;
            
            // Would render glyph texture here
            cursorX += glyph.advance * scale;
        } else {
            // Draw placeholder for missing glyph
            cursorX += currentFont_->size * 0.5f * scale;
        }
    }
}

void FontRenderer::drawText(const std::string& text, float x, float y, 
                             float maxWidth, const Color& color) {
    // Would implement text truncation/ellipsis
    drawText(text, x, y, color);
}

void FontRenderer::drawTextCentered(const std::string& text, const Rect& rect, 
                                     const Color& color) {
    Vec2 size = measureText(text, 14.0f);
    float x = rect.x + (rect.width - size.x) / 2;
    float y = rect.y + (rect.height - size.y) / 2 + size.y / 2;
    drawText(text, x, y, color);
}

std::vector<std::string> FontRenderer::wrapText(const std::string& text, float maxWidth) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    
    while (std::getline(stream, line)) {
        if (measureTextWidth(line, 14.0f) <= maxWidth) {
            lines.push_back(line);
            continue;
        }
        
        // Word wrap
        std::istringstream wordStream(line);
        std::string word;
        std::string currentLine;
        
        while (wordStream >> word) {
            std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
            if (measureTextWidth(testLine, 14.0f) <= maxWidth) {
                currentLine = testLine;
            } else {
                if (!currentLine.empty()) {
                    lines.push_back(currentLine);
                }
                currentLine = word;
            }
        }
        
        if (!currentLine.empty()) {
            lines.push_back(currentLine);
        }
    }
    
    return lines;
}

Glyph* FontRenderer::getGlyph(uint32_t codepoint) {
    if (!currentFont_) return nullptr;
    
    auto it = currentFont_->glyphs.find(codepoint);
    if (it != currentFont_->glyphs.end()) {
        return &it->second;
    }
    
    // Try to load missing glyph
    if (loadGlyph(currentFont_, codepoint)) {
        return &currentFont_->glyphs[codepoint];
    }
    
    return nullptr;
}

bool FontRenderer::loadGlyph(FontFace* font, uint32_t codepoint) {
    // Would use FreeType to load glyph
    Glyph glyph;
    glyph.codepoint = codepoint;
    glyph.x = 0;
    glyph.y = 0;
    glyph.width = font->size * 0.5f;
    glyph.height = font->size;
    glyph.bearingX = 0;
    glyph.bearingY = font->ascent;
    glyph.advance = font->size * 0.5f;
    glyph.texture = 0;
    
    font->glyphs[codepoint] = glyph;
    return true;
}

void FontRenderer::generateFontTexture(FontFace* font) {
    // Would generate atlas texture for font glyphs
}

} // namespace NexusForge::UI
