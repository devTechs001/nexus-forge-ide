// ui/framework/theme_manager.hpp
#pragma once

#include "widget_system.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

namespace NexusForge::UI {

// Color role
enum class ColorRole {
    Background,
    Foreground,
    Border,
    Primary,
    Secondary,
    Accent,
    Success,
    Warning,
    Error,
    Info,
    
    // Editor colors
    EditorBackground,
    EditorForeground,
    EditorCursor,
    EditorSelection,
    EditorLineNumber,
    EditorLineNumberActive,
    EditorIndentGuide,
    EditorWhitespace,
    
    // UI component colors
    ButtonBackground,
    ButtonForeground,
    ButtonHover,
    ButtonPressed,
    InputBackground,
    InputForeground,
    InputBorder,
    InputFocus,
    MenuBackground,
    MenuForeground,
    MenuHover,
    MenuSelected,
    TabBackground,
    TabForeground,
    TabActive,
    ScrollbarBackground,
    ScrollbarForeground,
    ScrollbarHover,
    
    // Status colors
    StatusBarBackground,
    StatusBarForeground,
    ActivityBarBackground,
    ActivityBarForeground,
    ActivityBarActive,
    SidebarBackground,
    PanelBackground,
    TitleBarBackground,
    TitleBarForeground
};

// Theme variant
enum class ThemeVariant {
    Light,
    Dark,
    HighContrast,
    Custom
};

// Color scheme
struct ColorScheme {
    std::unordered_map<ColorRole, Color> colors;
    
    Color get(ColorRole role) const;
    void set(ColorRole role, const Color& color);
    
    // Predefined schemes
    static ColorScheme createDark();
    static ColorScheme createLight();
    static ColorScheme createHighContrast();
};

// Font scheme
struct FontScheme {
    std::string fontFamily = "Inter";
    std::string monospaceFont = "JetBrains Mono";
    
    float fontSizeSmall = 11;
    float fontSizeNormal = 13;
    float fontSizeLarge = 16;
    float fontSizeXLarge = 20;
    
    float lineHeightSmall = 1.4f;
    float lineHeightNormal = 1.5f;
    float lineHeightLarge = 1.6f;
    
    float fontWeightNormal = 400;
    float fontWeightBold = 600;
};

// Spacing scheme
struct SpacingScheme {
    float unit = 4;
    
    float xs = 4;
    float sm = 8;
    float md = 16;
    float lg = 24;
    float xl = 32;
    float xxl = 48;
};

// Shadow definition
struct Shadow {
    float offsetX = 0;
    float offsetY = 2;
    float blur = 4;
    float spread = 0;
    Color color = Color(0, 0, 0, 0.25f);
    
    static Shadow small();
    static Shadow medium();
    static Shadow large();
};

// Widget style preset
struct WidgetStylePreset {
    WidgetStyle style;
    std::string name;
    ThemeVariant variant = ThemeVariant::Dark;
};

// Theme
class Theme {
public:
    using Ptr = std::shared_ptr<Theme>;
    
    Theme();
    Theme(const std::string& name, ThemeVariant variant = ThemeVariant::Dark);
    ~Theme();
    
    // Identity
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    
    ThemeVariant getVariant() const { return variant_; }
    void setVariant(ThemeVariant variant) { variant_ = variant; }
    
    bool isDark() const { return variant_ == ThemeVariant::Dark; }
    bool isLight() const { return variant_ == ThemeVariant::Light; }
    
    // Colors
    ColorScheme& getColors() { return colors_; }
    const ColorScheme& getColors() const { return colors_; }
    void setColor(ColorRole role, const Color& color);
    Color getColor(ColorRole role) const;
    
    // Fonts
    FontScheme& getFonts() { return fonts_; }
    const FontScheme& getFonts() const { return fonts_; }
    
    // Spacing
    SpacingScheme& getSpacing() { return spacing_; }
    const SpacingScheme& getSpacing() const { return spacing_; }
    
    // Shadows
    Shadow getShadow(const std::string& name) const;
    void setShadow(const std::string& name, const Shadow& shadow);
    
    // Widget styles
    WidgetStyle getWidgetStyle(const std::string& widgetType) const;
    void setWidgetStyle(const std::string& widgetType, const WidgetStyle& style);
    
    // Apply to widget
    void applyToWidget(Widget* widget) const;
    
    // Serialization
    std::string toJson() const;
    static Theme fromJson(const std::string& json);
    
    // Built-in themes
    static Theme createDefaultDark();
    static Theme createDefaultLight();
    static Theme createHighContrast();
    static Theme createSolarizedDark();
    static Theme createSolarizedLight();
    static Theme createMonokai();
    static Theme createDracula();
    static Theme createNord();
    
private:
    std::string name_;
    ThemeVariant variant_ = ThemeVariant::Dark;
    ColorScheme colors_;
    FontScheme fonts_;
    SpacingScheme spacing_;
    std::unordered_map<std::string, Shadow> shadows_;
    std::unordered_map<std::string, WidgetStyle> widgetStyles_;
};

// Theme manager
class ThemeManager {
public:
    static ThemeManager& getInstance();
    
    // Theme management
    void registerTheme(Theme::Ptr theme);
    void unregisterTheme(const std::string& name);
    Theme::Ptr getTheme(const std::string& name) const;
    
    // Current theme
    void setCurrentTheme(const std::string& name);
    Theme::Ptr getCurrentTheme() const { return currentTheme_; }
    
    // Theme switching
    void setDarkMode(bool dark);
    bool isDarkMode() const;
    
    // Auto theme based on system
    void setAutoTheme(bool autoTheme);
    bool isAutoTheme() const { return autoTheme_; }
    
    // Apply theme to widget
    void applyCurrentTheme(Widget* widget) const;
    void applyCurrentThemeToAll(Widget* root) const;
    
    // Events
    using ThemeChangedCallback = std::function<void(Theme::Ptr)>;
    size_t addThemeChangedListener(ThemeChangedCallback callback);
    void removeThemeChangedListener(size_t id);
    
    // List all themes
    std::vector<std::string> getAvailableThemes() const;
    
    // Load themes from directory
    void loadThemesFromDirectory(const std::string& path);
    void saveTheme(const std::string& name, const std::string& path);
    
private:
    ThemeManager();
    
    std::unordered_map<std::string, Theme::Ptr> themes_;
    Theme::Ptr currentTheme_;
    bool autoTheme_ = false;
    
    std::vector<std::pair<size_t, ThemeChangedCallback>> listeners_;
    size_t nextListenerId_ = 1;
    
    void notifyThemeChanged();
    void detectSystemTheme();
};

// Theme builder
class ThemeBuilder {
public:
    ThemeBuilder();
    
    ThemeBuilder& name(const std::string& name);
    ThemeBuilder& variant(ThemeVariant variant);
    ThemeBuilder& color(ColorRole role, const Color& color);
    ThemeBuilder& fontFamily(const std::string& family);
    ThemeBuilder& monospaceFont(const std::string& family);
    ThemeBuilder& fontSize(float size);
    ThemeBuilder& spacing(float unit);
    
    Theme::Ptr build();
    
private:
    Theme::Ptr theme_;
};

} // namespace NexusForge::UI
