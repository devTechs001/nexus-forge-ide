// ui/framework/theme_manager.hpp
#pragma once

#include "../../core/engine/event_system.hpp"
#include "../renderer/render_engine.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace NexusForge::UI {

// Color with gradient support
struct GradientColor {
    struct Stop {
        float position; // 0.0 to 1.0
        float r, g, b, a;
    };
    
    std::vector<Stop> stops;
    bool isGradient = false;
    
    GradientColor() = default;
    
    GradientColor(const Color& c) {
        stops.push_back({0.0f, c.r, c.g, c.b, c.a});
    }
    
    GradientColor& operator=(const Color& c) {
        stops.clear();
        stops.push_back({0.0f, c.r, c.g, c.b, c.a});
        return *this;
    }
    
    operator Color() const {
        if (!stops.empty()) {
            return Color(stops[0].r, stops[0].g, stops[0].b, stops[0].a);
        }
        return Color(1, 1, 1, 1);
    }
    
    // Solid color helpers
    static GradientColor fromRGBA(float r, float g, float b, float a = 1.0f) {
        GradientColor c;
        c.stops.push_back({0.0f, r, g, b, a});
        return c;
    }
    
    static GradientColor fromHex(uint32_t hex) {
        if (hex <= 0xFFFFFFu) {
            float r = ((hex >> 16) & 0xFFu) / 255.0f;
            float g = ((hex >> 8) & 0xFFu) / 255.0f;
            float b = (hex & 0xFFu) / 255.0f;
            return fromRGBA(r, g, b, 1.0f);
        }
        float r = ((hex >> 24) & 0xFFu) / 255.0f;
        float g = ((hex >> 16) & 0xFFu) / 255.0f;
        float b = ((hex >> 8) & 0xFFu) / 255.0f;
        float a = (hex & 0xFFu) / 255.0f;
        return fromRGBA(r, g, b, a);
    }
    
    // Linear gradient
    static GradientColor linearGradient(const Stop& start, const Stop& end) {
        GradientColor c;
        c.isGradient = true;
        c.stops.push_back(start);
        c.stops.push_back(end);
        return c;
    }
    
    // Multi-stop gradient
    static GradientColor multiGradient(std::vector<Stop> stops) {
        GradientColor c;
        c.isGradient = true;
        c.stops = std::move(stops);
        return c;
    }
};

// Predefined gradient presets
struct GradientPresets {
    // Warm gradients
    static GradientColor sunset() {
        return GradientColor::multiGradient({
            {0.0f, 1.0f, 0.5f, 0.0f, 1.0f},   // Orange
            {0.5f, 1.0f, 0.3f, 0.3f, 1.0f},   // Pink-Orange
            {1.0f, 0.8f, 0.0f, 0.5f, 1.0f}    // Purple
        });
    }
    
    static GradientColor sunrise() {
        return GradientColor::multiGradient({
            {0.0f, 1.0f, 0.9f, 0.5f, 1.0f},   // Light Orange
            {0.5f, 1.0f, 0.5f, 0.3f, 1.0f},   // Pink
            {1.0f, 0.5f, 0.3f, 0.8f, 1.0f}    // Purple
        });
    }
    
    static GradientColor fire() {
        return GradientColor::multiGradient({
            {0.0f, 1.0f, 0.0f, 0.0f, 1.0f},   // Red
            {0.5f, 1.0f, 0.5f, 0.0f, 1.0f},   // Orange
            {1.0f, 1.0f, 1.0f, 0.0f, 1.0f}    // Yellow
        });
    }
    
    // Cool gradients
    static GradientColor ocean() {
        return GradientColor::multiGradient({
            {0.0f, 0.0f, 0.5f, 1.0f, 1.0f},   // Deep Blue
            {0.5f, 0.0f, 0.8f, 0.8f, 1.0f},   // Cyan
            {1.0f, 0.2f, 1.0f, 0.6f, 1.0f}    // Light Cyan
        });
    }
    
    static GradientColor forest() {
        return GradientColor::multiGradient({
            {0.0f, 0.1f, 0.5f, 0.1f, 1.0f},   // Dark Green
            {0.5f, 0.2f, 0.7f, 0.2f, 1.0f},   // Green
            {1.0f, 0.5f, 0.9f, 0.3f, 1.0f}    // Light Green
        });
    }
    
    static GradientColor ice() {
        return GradientColor::multiGradient({
            {0.0f, 0.8f, 0.9f, 1.0f, 1.0f},   // Light Blue
            {0.5f, 0.5f, 0.7f, 1.0f, 1.0f},   // Blue
            {1.0f, 0.3f, 0.5f, 0.8f, 1.0f}    // Dark Blue
        });
    }
    
    // Vibrant gradients
    static GradientColor rainbow() {
        return GradientColor::multiGradient({
            {0.00f, 1.0f, 0.0f, 0.0f, 1.0f},  // Red
            {0.17f, 1.0f, 0.5f, 0.0f, 1.0f},  // Orange
            {0.33f, 1.0f, 1.0f, 0.0f, 1.0f},  // Yellow
            {0.50f, 0.0f, 1.0f, 0.0f, 1.0f},  // Green
            {0.67f, 0.0f, 0.5f, 1.0f, 1.0f},  // Blue
            {0.83f, 0.5f, 0.0f, 1.0f, 1.0f},  // Indigo
            {1.00f, 1.0f, 0.0f, 0.5f, 1.0f}   // Violet
        });
    }
    
    static GradientColor neon() {
        return GradientColor::multiGradient({
            {0.0f, 1.0f, 0.0f, 1.0f, 1.0f},   // Neon Red
            {0.5f, 0.0f, 1.0f, 1.0f, 1.0f},   // Neon Green
            {1.0f, 1.0f, 0.0f, 1.0f, 1.0f}    // Neon Blue
        });
    }
    
    static GradientColor cyberpunk() {
        return GradientColor::multiGradient({
            {0.0f, 1.0f, 0.0f, 0.5f, 1.0f},   // Hot Pink
            {0.5f, 0.0f, 1.0f, 1.0f, 1.0f},   // Cyan
            {1.0f, 0.8f, 0.0f, 1.0f, 1.0f}    // Purple
        });
    }
    
    // Subtle gradients
    static GradientColor midnight() {
        return GradientColor::multiGradient({
            {0.0f, 0.1f, 0.1f, 0.2f, 1.0f},   // Dark Blue-Gray
            {0.5f, 0.2f, 0.2f, 0.3f, 1.0f},   // Medium Blue-Gray
            {1.0f, 0.3f, 0.3f, 0.4f, 1.0f}    // Light Blue-Gray
        });
    }
    
    static GradientColor dawn() {
        return GradientColor::multiGradient({
            {0.0f, 0.9f, 0.9f, 1.0f, 1.0f},   // Light Blue
            {0.5f, 1.0f, 0.8f, 0.6f, 1.0f},   // Soft Pink
            {1.0f, 1.0f, 0.9f, 0.7f, 1.0f}    // Warm Yellow
        });
    }
    
    // Monochrome gradients
    static GradientColor grayscale() {
        return GradientColor::multiGradient({
            {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},   // Black
            {0.5f, 0.5f, 0.5f, 0.5f, 1.0f},   // Gray
            {1.0f, 1.0f, 1.0f, 1.0f, 1.0f}    // White
        });
    }
    
    static GradientColor charcoal() {
        return GradientColor::multiGradient({
            {0.0f, 0.1f, 0.1f, 0.1f, 1.0f},   // Dark Charcoal
            {0.5f, 0.2f, 0.2f, 0.2f, 1.0f},   // Medium Charcoal
            {1.0f, 0.3f, 0.3f, 0.3f, 1.0f}    // Light Charcoal
        });
    }
};

// Theme definition
struct Theme {
    std::string name;
    std::string id;
    bool isDark = true;
    
    // Base colors
    GradientColor backgroundPrimary;
    GradientColor backgroundSecondary;
    GradientColor backgroundTertiary;
    
    // Surface colors
    GradientColor surface;
    GradientColor surfaceHover;
    GradientColor surfaceActive;
    GradientColor surfaceSelected;
    
    // Text colors
    GradientColor textPrimary;
    GradientColor textSecondary;
    GradientColor textDisabled;
    GradientColor textInverse;
    
    // Accent colors
    GradientColor accentPrimary;
    GradientColor accentSecondary;
    GradientColor accentHover;
    GradientColor accentActive;
    
    // Border colors
    GradientColor border;
    GradientColor borderHover;
    GradientColor borderFocus;
    
    // Status colors
    GradientColor success;
    GradientColor warning;
    GradientColor error;
    GradientColor info;
    
    // Syntax highlighting colors
    GradientColor syntaxKeyword;
    GradientColor syntaxType;
    GradientColor syntaxFunction;
    GradientColor syntaxVariable;
    GradientColor syntaxLiteral;
    GradientColor syntaxComment;
    GradientColor syntaxString;
    GradientColor syntaxNumber;
    GradientColor syntaxOperator;
    GradientColor syntaxTag;
    GradientColor syntaxAttribute;
    
    // UI element colors
    GradientColor scrollbar;
    GradientColor scrollbarHover;
    GradientColor selection;
    GradientColor highlight;
    GradientColor cursor;
    GradientColor lineNumber;
    GradientColor lineNumberActive;
    GradientColor guideLine;
    GradientColor minimap;
    
    // Window colors
    GradientColor titleBar;
    GradientColor titleBarInactive;
    GradientColor statusBar;
    GradientColor toolbar;
    
    // Font settings
    std::string fontFamily = "Inter";
    std::string monospaceFont = "JetBrains Mono";
    float fontSize = 14.0f;
    float lineHeight = 1.5f;
    
    // Spacing
    float spacingXS = 4.0f;
    float spacingS = 8.0f;
    float spacingM = 16.0f;
    float spacingL = 24.0f;
    float spacingXL = 32.0f;
    
    // Border radius
    float borderRadiusS = 4.0f;
    float borderRadiusM = 8.0f;
    float borderRadiusL = 12.0f;
    float borderRadiusXL = 16.0f;
    
    // Shadow
    bool enableShadows = true;
    float shadowBlur = 16.0f;
    float shadowOffsetX = 0.0f;
    float shadowOffsetY = 4.0f;
};

// Theme Manager
class ThemeManager {
public:
    ThemeManager();
    ~ThemeManager();

    // Theme management
    void registerTheme(const Theme& theme);
    void setTheme(const std::string& themeId);
    const Theme* getCurrentTheme() const { return currentTheme_; }
    const Theme* getTheme(const std::string& themeId) const;
    std::vector<std::string> getAvailableThemes() const;

    // Color resolution
    GradientColor getColor(const std::string& colorName) const;
    
    // Events
    using ThemeChangedCallback = std::function<void(const Theme&)>;
    void onThemeChanged(ThemeChangedCallback callback);

    // Built-in themes
    static Theme createDarkTheme();
    static Theme createLightTheme();
    static Theme createMidnightTheme();
    static Theme createSunsetTheme();
    static Theme createOceanTheme();
    static Theme createForestTheme();
    static Theme createCyberpunkTheme();
    static Theme createMonokaiTheme();
    static Theme createDraculaTheme();
    static Theme createNordTheme();
    static Theme createGruvboxTheme();
    static Theme createSolarizedTheme();
    static Theme createRainbowTheme();

private:
    std::unordered_map<std::string, Theme> themes_;
    std::string currentThemeId_;
    Theme* currentTheme_ = nullptr;
    std::vector<ThemeChangedCallback> callbacks_;
    
    void loadBuiltInThemes();
    void notifyThemeChanged();
};

} // namespace NexusForge::UI
