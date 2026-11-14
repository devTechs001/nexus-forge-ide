// ui/framework/theme_manager.cpp
#include "theme_manager.hpp"

namespace NexusForge::UI {

ThemeManager::ThemeManager() {
    loadBuiltInThemes();
}

ThemeManager::~ThemeManager() = default;

void ThemeManager::registerTheme(const Theme& theme) {
    themes_[theme.id] = theme;
    if (currentThemeId_.empty() || currentThemeId_ == theme.id) {
        currentThemeId_ = theme.id;
        currentTheme_ = &themes_[currentThemeId_];
    } else {
        auto it = themes_.find(currentThemeId_);
        currentTheme_ = (it != themes_.end()) ? &it->second : nullptr;
    }
}

void ThemeManager::setTheme(const std::string& themeId) {
    auto it = themes_.find(themeId);
    if (it != themes_.end()) {
        currentThemeId_ = themeId;
        currentTheme_ = &it->second;
        notifyThemeChanged();
    }
}

const Theme* ThemeManager::getTheme(const std::string& themeId) const {
    auto it = themes_.find(themeId);
    if (it != themes_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> ThemeManager::getAvailableThemes() const {
    std::vector<std::string> ids;
    for (const auto& [id, theme] : themes_) {
        ids.push_back(id);
    }
    return ids;
}

GradientColor ThemeManager::getColor(const std::string& colorName) const {
    if (currentTheme_) {
        if (colorName == "backgroundPrimary") return currentTheme_->backgroundPrimary;
        if (colorName == "backgroundSecondary") return currentTheme_->backgroundSecondary;
        if (colorName == "backgroundTertiary") return currentTheme_->backgroundTertiary;
        if (colorName == "surface") return currentTheme_->surface;
        if (colorName == "surfaceHover") return currentTheme_->surfaceHover;
        if (colorName == "surfaceActive") return currentTheme_->surfaceActive;
        if (colorName == "surfaceSelected") return currentTheme_->surfaceSelected;
        if (colorName == "textPrimary") return currentTheme_->textPrimary;
        if (colorName == "textSecondary") return currentTheme_->textSecondary;
        if (colorName == "textDisabled") return currentTheme_->textDisabled;
        if (colorName == "textInverse") return currentTheme_->textInverse;
        if (colorName == "accentPrimary") return currentTheme_->accentPrimary;
        if (colorName == "accentSecondary") return currentTheme_->accentSecondary;
        if (colorName == "border") return currentTheme_->border;
        if (colorName == "success") return currentTheme_->success;
        if (colorName == "warning") return currentTheme_->warning;
        if (colorName == "error") return currentTheme_->error;
        if (colorName == "info") return currentTheme_->info;
    }
    return GradientColor::fromRGBA(0.5f, 0.5f, 0.5f, 1.0f);
}

void ThemeManager::onThemeChanged(ThemeChangedCallback callback) {
    callbacks_.push_back(std::move(callback));
}

void ThemeManager::notifyThemeChanged() {
    for (auto& cb : callbacks_) {
        if (currentTheme_) {
            cb(*currentTheme_);
        }
    }
}

void ThemeManager::loadBuiltInThemes() {
    registerTheme(createDarkTheme());
    registerTheme(createLightTheme());
    registerTheme(createMidnightTheme());
    registerTheme(createSunsetTheme());
    registerTheme(createOceanTheme());
    registerTheme(createForestTheme());
    registerTheme(createCyberpunkTheme());
    registerTheme(createMonokaiTheme());
    registerTheme(createDraculaTheme());
    registerTheme(createNordTheme());
    registerTheme(createGruvboxTheme());
    registerTheme(createSolarizedTheme());
    registerTheme(createRainbowTheme());
}

Theme ThemeManager::createDarkTheme() {
    Theme theme;
    theme.id = "dark";
    theme.name = "Dark";
    theme.isDark = true;
    
    // Background colors
    theme.backgroundPrimary = GradientColor::fromHex(0x1E1E2E);
    theme.backgroundSecondary = GradientColor::fromHex(0x252538);
    theme.backgroundTertiary = GradientColor::fromHex(0x2D2D44);
    
    // Surface colors
    theme.surface = GradientColor::fromHex(0x313244);
    theme.surfaceHover = GradientColor::fromHex(0x45475A);
    theme.surfaceActive = GradientColor::fromHex(0x585B70);
    theme.surfaceSelected = GradientColor::fromHex(0x45475A);
    
    // Text colors
    theme.textPrimary = GradientColor::fromHex(0xCDD6F4);
    theme.textSecondary = GradientColor::fromHex(0xA6ADC8);
    theme.textDisabled = GradientColor::fromHex(0x6C7086);
    theme.textInverse = GradientColor::fromHex(0x1E1E2E);
    
    // Accent colors - Blue gradient
    theme.accentPrimary = GradientColor::linearGradient(
        GradientColor::Stop{0.0f, 0.53f, 0.69f, 0.93f, 1.0f},
        GradientColor::Stop{1.0f, 0.49f, 0.57f, 0.99f, 1.0f}
    );
    theme.accentSecondary = GradientColor::fromHex(0x89B4FA);
    theme.accentHover = GradientColor::fromHex(0xB4BEFE);
    theme.accentActive = GradientColor::fromHex(0x74C7EC);
    
    // Border colors
    theme.border = GradientColor::fromHex(0x45475A);
    theme.borderHover = GradientColor::fromHex(0x585B70);
    theme.borderFocus = GradientColor::fromHex(0x89B4FA);
    
    // Status colors
    theme.success = GradientColor::fromHex(0xA6E3A1);
    theme.warning = GradientColor::fromHex(0xF9E2AF);
    theme.error = GradientColor::fromHex(0xF38BA8);
    theme.info = GradientColor::fromHex(0x89DCEB);
    
    // Syntax highlighting
    theme.syntaxKeyword = GradientColor::fromHex(0xF38BA8);
    theme.syntaxType = GradientColor::fromHex(0xFAB387);
    theme.syntaxFunction = GradientColor::fromHex(0x89B4FA);
    theme.syntaxVariable = GradientColor::fromHex(0xCDD6F4);
    theme.syntaxLiteral = GradientColor::fromHex(0xFAB387);
    theme.syntaxComment = GradientColor::fromHex(0x6C7086);
    theme.syntaxString = GradientColor::fromHex(0xA6E3A1);
    theme.syntaxNumber = GradientColor::fromHex(0xFAB387);
    theme.syntaxOperator = GradientColor::fromHex(0x89DCEB);
    theme.syntaxTag = GradientColor::fromHex(0xF38BA8);
    theme.syntaxAttribute = GradientColor::fromHex(0xFAB387);
    
    // UI elements
    theme.scrollbar = GradientColor::fromHex(0x45475A);
    theme.scrollbarHover = GradientColor::fromHex(0x585B70);
    theme.selection = GradientColor::fromHex(0x45475A);
    theme.highlight = GradientColor::fromHex(0x45475A);
    theme.cursor = GradientColor::fromHex(0xF5E0DC);
    theme.lineNumber = GradientColor::fromHex(0x6C7086);
    theme.lineNumberActive = GradientColor::fromHex(0xCDD6F4);
    theme.guideLine = GradientColor::fromHex(0x313244);
    theme.minimap = GradientColor::fromHex(0x181825);
    
    // Window colors
    theme.titleBar = GradientColor::fromHex(0x11111B);
    theme.titleBarInactive = GradientColor::fromHex(0x181825);
    theme.statusBar = GradientColor::fromHex(0x11111B);
    theme.toolbar = GradientColor::fromHex(0x1E1E2E);
    
    return theme;
}

Theme ThemeManager::createLightTheme() {
    Theme theme;
    theme.id = "light";
    theme.name = "Light";
    theme.isDark = false;
    
    // Background colors
    theme.backgroundPrimary = GradientColor::fromHex(0xFFFFFF);
    theme.backgroundSecondary = GradientColor::fromHex(0xF5F5F5);
    theme.backgroundTertiary = GradientColor::fromHex(0xE8E8E8);
    
    // Surface colors
    theme.surface = GradientColor::fromHex(0xFFFFFF);
    theme.surfaceHover = GradientColor::fromHex(0xF0F0F0);
    theme.surfaceActive = GradientColor::fromHex(0xE0E0E0);
    theme.surfaceSelected = GradientColor::fromHex(0xD0D0D0);
    
    // Text colors
    theme.textPrimary = GradientColor::fromHex(0x1A1A1A);
    theme.textSecondary = GradientColor::fromHex(0x555555);
    theme.textDisabled = GradientColor::fromHex(0x999999);
    theme.textInverse = GradientColor::fromHex(0xFFFFFF);
    
    // Accent colors - Purple gradient
    theme.accentPrimary = GradientColor::linearGradient(
        GradientColor::Stop{0.0f, 0.5f, 0.3f, 0.9f, 1.0f},
        GradientColor::Stop{1.0f, 0.7f, 0.4f, 0.95f, 1.0f}
    );
    theme.accentSecondary = GradientColor::fromHex(0x6B5B95);
    theme.accentHover = GradientColor::fromHex(0x8B7BA8);
    theme.accentActive = GradientColor::fromHex(0x5A4A7A);
    
    // Border colors
    theme.border = GradientColor::fromHex(0xDDDDDD);
    theme.borderHover = GradientColor::fromHex(0xCCCCCC);
    theme.borderFocus = GradientColor::fromHex(0x6B5B95);
    
    // Status colors
    theme.success = GradientColor::fromHex(0x2E7D32);
    theme.warning = GradientColor::fromHex(0xF57C00);
    theme.error = GradientColor::fromHex(0xC62828);
    theme.info = GradientColor::fromHex(0x1976D2);
    
    // Syntax highlighting
    theme.syntaxKeyword = GradientColor::fromHex(0x8800CC);
    theme.syntaxType = GradientColor::fromHex(0x0066CC);
    theme.syntaxFunction = GradientColor::fromHex(0x006600);
    theme.syntaxVariable = GradientColor::fromHex(0x000000);
    theme.syntaxLiteral = GradientColor::fromHex(0xCC6600);
    theme.syntaxComment = GradientColor::fromHex(0x888888);
    theme.syntaxString = GradientColor::fromHex(0x008800);
    theme.syntaxNumber = GradientColor::fromHex(0xCC6600);
    theme.syntaxOperator = GradientColor::fromHex(0x333333);
    theme.syntaxTag = GradientColor::fromHex(0x880000);
    theme.syntaxAttribute = GradientColor::fromHex(0xCC6600);
    
    // UI elements
    theme.scrollbar = GradientColor::fromHex(0xDDDDDD);
    theme.scrollbarHover = GradientColor::fromHex(0xCCCCCC);
    theme.selection = GradientColor::fromHex(0xE0E0FF);
    theme.highlight = GradientColor::fromHex(0xFFFFCC);
    theme.cursor = GradientColor::fromHex(0x333333);
    theme.lineNumber = GradientColor::fromHex(0x999999);
    theme.lineNumberActive = GradientColor::fromHex(0x333333);
    theme.guideLine = GradientColor::fromHex(0xEEEEEE);
    theme.minimap = GradientColor::fromHex(0xFAFAFA);
    
    // Window colors
    theme.titleBar = GradientColor::fromHex(0xE8E8E8);
    theme.titleBarInactive = GradientColor::fromHex(0xF0F0F0);
    theme.statusBar = GradientColor::fromHex(0xF5F5F5);
    theme.toolbar = GradientColor::fromHex(0xFFFFFF);
    
    return theme;
}

Theme ThemeManager::createMidnightTheme() {
    Theme theme;
    theme.id = "midnight";
    theme.name = "Midnight Aurora";
    theme.isDark = true;
    
    // Deep blue-black background with subtle gradient
    theme.backgroundPrimary = GradientColor::multiGradient({
        {0.0f, 0.05f, 0.05f, 0.1f, 1.0f},
        {0.5f, 0.1f, 0.1f, 0.2f, 1.0f},
        {1.0f, 0.05f, 0.05f, 0.15f, 1.0f}
    });
    theme.backgroundSecondary = GradientColor::multiGradient({
        {0.0f, 0.1f, 0.1f, 0.2f, 1.0f},
        {1.0f, 0.15f, 0.15f, 0.25f, 1.0f}
    });
    theme.backgroundTertiary = GradientColor::fromHex(0x2A2A3E);
    
    // Surface colors with purple tint
    theme.surface = GradientColor::multiGradient({
        {0.0f, 0.2f, 0.2f, 0.35f, 1.0f},
        {1.0f, 0.25f, 0.2f, 0.4f, 1.0f}
    });
    theme.surfaceHover = GradientColor::fromHex(0x3D3D5C);
    theme.surfaceActive = GradientColor::fromHex(0x4D4D6A);
    theme.surfaceSelected = GradientColor::fromHex(0x404055);
    
    // Text colors - soft white
    theme.textPrimary = GradientColor::fromHex(0xE8E8F0);
    theme.textSecondary = GradientColor::fromHex(0xB0B0C0);
    theme.textDisabled = GradientColor::fromHex(0x606070);
    theme.textInverse = GradientColor::fromHex(0x1A1A2E);
    
    // Aurora accent - Northern lights gradient
    theme.accentPrimary = GradientColor::multiGradient({
        {0.0f, 0.0f, 1.0f, 0.8f, 1.0f},    // Cyan
        {0.5f, 0.0f, 0.9f, 0.5f, 1.0f},    // Green
        {1.0f, 0.5f, 0.3f, 0.9f, 1.0f}     // Purple
    });
    theme.accentSecondary = GradientColor::fromHex(0x7B68EE);
    theme.accentHover = GradientColor::fromHex(0x9370DB);
    theme.accentActive = GradientColor::fromHex(0x4169E1);
    
    // Border colors
    theme.border = GradientColor::fromHex(0x3A3A5A);
    theme.borderHover = GradientColor::fromHex(0x4A4A6A);
    theme.borderFocus = GradientColor::fromHex(0x7B68EE);
    
    // Status colors
    theme.success = GradientColor::fromHex(0x50FA7B);
    theme.warning = GradientColor::fromHex(0xF1FA8C);
    theme.error = GradientColor::fromHex(0xFF5555);
    theme.info = GradientColor::fromHex(0x8BE9FD);
    
    // Syntax highlighting - Aurora palette
    theme.syntaxKeyword = GradientColor::fromHex(0xFF79C6);
    theme.syntaxType = GradientColor::fromHex(0x8BE9FD);
    theme.syntaxFunction = GradientColor::fromHex(0x50FA7B);
    theme.syntaxVariable = GradientColor::fromHex(0xF8F8F2);
    theme.syntaxLiteral = GradientColor::fromHex(0xF1FA8C);
    theme.syntaxComment = GradientColor::fromHex(0x6272A4);
    theme.syntaxString = GradientColor::fromHex(0x50FA7B);
    theme.syntaxNumber = GradientColor::fromHex(0xBD93F9);
    theme.syntaxOperator = GradientColor::fromHex(0xFF79C6);
    theme.syntaxTag = GradientColor::fromHex(0xFF79C6);
    theme.syntaxAttribute = GradientColor::fromHex(0x50FA7B);
    
    // UI elements
    theme.scrollbar = GradientColor::fromHex(0x3A3A5A);
    theme.scrollbarHover = GradientColor::fromHex(0x4A4A6A);
    theme.selection = GradientColor::fromHex(0x44475A);
    theme.highlight = GradientColor::fromHex(0x44475A);
    theme.cursor = GradientColor::fromHex(0xF8F8F0);
    theme.lineNumber = GradientColor::fromHex(0x6272A4);
    theme.lineNumberActive = GradientColor::fromHex(0xF8F8F2);
    theme.guideLine = GradientColor::fromHex(0x2A2A3E);
    theme.minimap = GradientColor::fromHex(0x1A1A2E);
    
    // Window colors
    theme.titleBar = GradientColor::multiGradient({
        {0.0f, 0.1f, 0.1f, 0.2f, 1.0f},
        {1.0f, 0.15f, 0.1f, 0.25f, 1.0f}
    });
    theme.titleBarInactive = GradientColor::fromHex(0x1A1A2E);
    theme.statusBar = GradientColor::fromHex(0x16162A);
    theme.toolbar = GradientColor::fromHex(0x1E1E3A);
    
    return theme;
}

Theme ThemeManager::createSunsetTheme() {
    Theme theme;
    theme.id = "sunset";
    theme.name = "Sunset Glow";
    theme.isDark = true;
    
    // Warm sunset background
    theme.backgroundPrimary = GradientColor::multiGradient({
        {0.0f, 0.15f, 0.08f, 0.12f, 1.0f},
        {0.5f, 0.2f, 0.1f, 0.15f, 1.0f},
        {1.0f, 0.18f, 0.12f, 0.18f, 1.0f}
    });
    theme.backgroundSecondary = GradientColor::multiGradient({
        {0.0f, 0.2f, 0.12f, 0.18f, 1.0f},
        {1.0f, 0.25f, 0.15f, 0.2f, 1.0f}
    });
    theme.backgroundTertiary = GradientColor::fromHex(0x3D2A3D);
    
    // Surface colors with warm tones
    theme.surface = GradientColor::multiGradient({
        {0.0f, 0.3f, 0.18f, 0.25f, 1.0f},
        {1.0f, 0.35f, 0.2f, 0.28f, 1.0f}
    });
    theme.surfaceHover = GradientColor::fromHex(0x4A3A4A);
    theme.surfaceActive = GradientColor::fromHex(0x5A4A5A);
    theme.surfaceSelected = GradientColor::fromHex(0x504050);
    
    // Text colors - warm white
    theme.textPrimary = GradientColor::fromHex(0xFFF0E0);
    theme.textSecondary = GradientColor::fromHex(0xE0C0B0);
    theme.textDisabled = GradientColor::fromHex(0x807060);
    theme.textInverse = GradientColor::fromHex(0x2A1818);
    
    // Sunset accent - Orange to pink gradient
    theme.accentPrimary = GradientColor::multiGradient({
        {0.0f, 1.0f, 0.4f, 0.2f, 1.0f},    // Orange
        {0.5f, 1.0f, 0.3f, 0.3f, 1.0f},    // Coral
        {1.0f, 0.9f, 0.2f, 0.5f, 1.0f}     // Pink
    });
    theme.accentSecondary = GradientColor::fromHex(0xFF6B6B);
    theme.accentHover = GradientColor::fromHex(0xFF8E8E);
    theme.accentActive = GradientColor::fromHex(0xFF5252);
    
    // Border colors
    theme.border = GradientColor::fromHex(0x5A4A4A);
    theme.borderHover = GradientColor::fromHex(0x6A5A5A);
    theme.borderFocus = GradientColor::fromHex(0xFF6B6B);
    
    // Status colors
    theme.success = GradientColor::fromHex(0x81C784);
    theme.warning = GradientColor::fromHex(0xFFB74D);
    theme.error = GradientColor::fromHex(0xE57373);
    theme.info = GradientColor::fromHex(0x64B5F6);
    
    // Syntax highlighting - Warm palette
    theme.syntaxKeyword = GradientColor::fromHex(0xFF6B6B);
    theme.syntaxType = GradientColor::fromHex(0xFFB74D);
    theme.syntaxFunction = GradientColor::fromHex(0x81C784);
    theme.syntaxVariable = GradientColor::fromHex(0xFFF0E0);
    theme.syntaxLiteral = GradientColor::fromHex(0xFFD54F);
    theme.syntaxComment = GradientColor::fromHex(0x8D6E63);
    theme.syntaxString = GradientColor::fromHex(0x81C784);
    theme.syntaxNumber = GradientColor::fromHex(0xFFB74D);
    theme.syntaxOperator = GradientColor::fromHex(0xFF8A65);
    theme.syntaxTag = GradientColor::fromHex(0xFF6B6B);
    theme.syntaxAttribute = GradientColor::fromHex(0xFFD54F);
    
    // UI elements
    theme.scrollbar = GradientColor::fromHex(0x5A4A4A);
    theme.scrollbarHover = GradientColor::fromHex(0x6A5A5A);
    theme.selection = GradientColor::fromHex(0x5A3A3A);
    theme.highlight = GradientColor::fromHex(0x5A4A3A);
    theme.cursor = GradientColor::fromHex(0xFFE0B2);
    theme.lineNumber = GradientColor::fromHex(0x8D6E63);
    theme.lineNumberActive = GradientColor::fromHex(0xFFE0B2);
    theme.guideLine = GradientColor::fromHex(0x3D2A2A);
    theme.minimap = GradientColor::fromHex(0x2A1818);
    
    // Window colors
    theme.titleBar = GradientColor::multiGradient({
        {0.0f, 0.2f, 0.12f, 0.18f, 1.0f},
        {1.0f, 0.25f, 0.15f, 0.22f, 1.0f}
    });
    theme.titleBarInactive = GradientColor::fromHex(0x2A1818);
    theme.statusBar = GradientColor::fromHex(0x1F1414);
    theme.toolbar = GradientColor::fromHex(0x2A1A1A);
    
    return theme;
}

Theme ThemeManager::createOceanTheme() {
    Theme theme;
    theme.id = "ocean";
    theme.name = "Deep Ocean";
    theme.isDark = true;
    
    // Deep ocean blue background
    theme.backgroundPrimary = GradientColor::multiGradient({
        {0.0f, 0.0f, 0.1f, 0.15f, 1.0f},
        {0.5f, 0.0f, 0.15f, 0.2f, 1.0f},
        {1.0f, 0.0f, 0.12f, 0.18f, 1.0f}
    });
    theme.backgroundSecondary = GradientColor::multiGradient({
        {0.0f, 0.05f, 0.15f, 0.25f, 1.0f},
        {1.0f, 0.0f, 0.2f, 0.3f, 1.0f}
    });
    theme.backgroundTertiary = GradientColor::fromHex(0x1A3A4A);
    
    // Surface colors with cyan tint
    theme.surface = GradientColor::multiGradient({
        {0.0f, 0.1f, 0.25f, 0.35f, 1.0f},
        {1.0f, 0.05f, 0.3f, 0.4f, 1.0f}
    });
    theme.surfaceHover = GradientColor::fromHex(0x2A4A5A);
    theme.surfaceActive = GradientColor::fromHex(0x3A5A6A);
    theme.surfaceSelected = GradientColor::fromHex(0x305060);
    
    // Text colors - cyan tinted
    theme.textPrimary = GradientColor::fromHex(0xE0F0F0);
    theme.textSecondary = GradientColor::fromHex(0xA0C0C0);
    theme.textDisabled = GradientColor::fromHex(0x507070);
    theme.textInverse = GradientColor::fromHex(0x0A1A2A);
    
    // Ocean accent - Cyan to blue gradient
    theme.accentPrimary = GradientColor::multiGradient({
        {0.0f, 0.0f, 0.9f, 1.0f, 1.0f},    // Cyan
        {0.5f, 0.0f, 0.7f, 0.9f, 1.0f},    // Light Blue
        {1.0f, 0.0f, 0.5f, 0.8f, 1.0f}     // Blue
    });
    theme.accentSecondary = GradientColor::fromHex(0x00BCD4);
    theme.accentHover = GradientColor::fromHex(0x26C6DA);
    theme.accentActive = GradientColor::fromHex(0x00ACC1);
    
    // Border colors
    theme.border = GradientColor::fromHex(0x2A4A5A);
    theme.borderHover = GradientColor::fromHex(0x3A5A6A);
    theme.borderFocus = GradientColor::fromHex(0x00BCD4);
    
    // Status colors
    theme.success = GradientColor::fromHex(0x26A69A);
    theme.warning = GradientColor::fromHex(0xFFCA28);
    theme.error = GradientColor::fromHex(0xEF5350);
    theme.info = GradientColor::fromHex(0x42A5F5);
    
    // Syntax highlighting - Ocean palette
    theme.syntaxKeyword = GradientColor::fromHex(0xFF7043);
    theme.syntaxType = GradientColor::fromHex(0x26C6DA);
    theme.syntaxFunction = GradientColor::fromHex(0x26A69A);
    theme.syntaxVariable = GradientColor::fromHex(0xB0BEC5);
    theme.syntaxLiteral = GradientColor::fromHex(0xFFCA28);
    theme.syntaxComment = GradientColor::fromHex(0x546E7A);
    theme.syntaxString = GradientColor::fromHex(0x66BB6A);
    theme.syntaxNumber = GradientColor::fromHex(0xFFA726);
    theme.syntaxOperator = GradientColor::fromHex(0x29B6F6);
    theme.syntaxTag = GradientColor::fromHex(0xFF7043);
    theme.syntaxAttribute = GradientColor::fromHex(0xFFCA28);
    
    // UI elements
    theme.scrollbar = GradientColor::fromHex(0x2A4A5A);
    theme.scrollbarHover = GradientColor::fromHex(0x3A5A6A);
    theme.selection = GradientColor::fromHex(0x1A3A4A);
    theme.highlight = GradientColor::fromHex(0x2A3A4A);
    theme.cursor = GradientColor::fromHex(0x80DEEA);
    theme.lineNumber = GradientColor::fromHex(0x546E7A);
    theme.lineNumberActive = GradientColor::fromHex(0x80DEEA);
    theme.guideLine = GradientColor::fromHex(0x1A2A3A);
    theme.minimap = GradientColor::fromHex(0x0A1A2A);
    
    // Window colors
    theme.titleBar = GradientColor::multiGradient({
        {0.0f, 0.0f, 0.15f, 0.25f, 1.0f},
        {1.0f, 0.0f, 0.2f, 0.3f, 1.0f}
    });
    theme.titleBarInactive = GradientColor::fromHex(0x0A1A2A);
    theme.statusBar = GradientColor::fromHex(0x051018);
    theme.toolbar = GradientColor::fromHex(0x0A2028);
    
    return theme;
}

Theme ThemeManager::createForestTheme() {
    Theme theme;
    theme.id = "forest";
    theme.name = "Enchanted Forest";
    theme.isDark = true;
    
    // Deep forest green background
    theme.backgroundPrimary = GradientColor::multiGradient({
        {0.0f, 0.05f, 0.12f, 0.08f, 1.0f},
        {0.5f, 0.08f, 0.15f, 0.1f, 1.0f},
        {1.0f, 0.06f, 0.14f, 0.09f, 1.0f}
    });
    theme.backgroundSecondary = GradientColor::multiGradient({
        {0.0f, 0.1f, 0.18f, 0.12f, 1.0f},
        {1.0f, 0.12f, 0.2f, 0.14f, 1.0f}
    });
    theme.backgroundTertiary = GradientColor::fromHex(0x2A3A2A);
    
    // Surface colors with moss tint
    theme.surface = GradientColor::multiGradient({
        {0.0f, 0.15f, 0.25f, 0.18f, 1.0f},
        {1.0f, 0.18f, 0.28f, 0.2f, 1.0f}
    });
    theme.surfaceHover = GradientColor::fromHex(0x3A4A3A);
    theme.surfaceActive = GradientColor::fromHex(0x4A5A4A);
    theme.surfaceSelected = GradientColor::fromHex(0x405040);
    
    // Text colors - soft green tint
    theme.textPrimary = GradientColor::fromHex(0xE8F0E8);
    theme.textSecondary = GradientColor::fromHex(0xB0C0B0);
    theme.textDisabled = GradientColor::fromHex(0x607060);
    theme.textInverse = GradientColor::fromHex(0x1A2A1A);
    
    // Forest accent - Green to gold gradient
    theme.accentPrimary = GradientColor::multiGradient({
        {0.0f, 0.4f, 0.7f, 0.3f, 1.0f},    // Forest Green
        {0.5f, 0.5f, 0.8f, 0.4f, 1.0f},    // Light Green
        {1.0f, 0.8f, 0.7f, 0.2f, 1.0f}     // Gold
    });
    theme.accentSecondary = GradientColor::fromHex(0x66BB6A);
    theme.accentHover = GradientColor::fromHex(0x81C784);
    theme.accentActive = GradientColor::fromHex(0x4CAF50);
    
    // Border colors
    theme.border = GradientColor::fromHex(0x3A4A3A);
    theme.borderHover = GradientColor::fromHex(0x4A5A4A);
    theme.borderFocus = GradientColor::fromHex(0x66BB6A);
    
    // Status colors
    theme.success = GradientColor::fromHex(0x81C784);
    theme.warning = GradientColor::fromHex(0xFFD54F);
    theme.error = GradientColor::fromHex(0xE57373);
    theme.info = GradientColor::fromHex(0x4DB6AC);
    
    // Syntax highlighting - Forest palette
    theme.syntaxKeyword = GradientColor::fromHex(0xE57373);
    theme.syntaxType = GradientColor::fromHex(0x4DB6AC);
    theme.syntaxFunction = GradientColor::fromHex(0x81C784);
    theme.syntaxVariable = GradientColor::fromHex(0xC5E1A5);
    theme.syntaxLiteral = GradientColor::fromHex(0xFFD54F);
    theme.syntaxComment = GradientColor::fromHex(0x81A081);
    theme.syntaxString = GradientColor::fromHex(0xAED581);
    theme.syntaxNumber = GradientColor::fromHex(0xFFB74D);
    theme.syntaxOperator = GradientColor::fromHex(0x4DB6AC);
    theme.syntaxTag = GradientColor::fromHex(0xE57373);
    theme.syntaxAttribute = GradientColor::fromHex(0xFFD54F);
    
    // UI elements
    theme.scrollbar = GradientColor::fromHex(0x3A4A3A);
    theme.scrollbarHover = GradientColor::fromHex(0x4A5A4A);
    theme.selection = GradientColor::fromHex(0x2A3A2A);
    theme.highlight = GradientColor::fromHex(0x2A3A2A);
    theme.cursor = GradientColor::fromHex(0xC5E1A5);
    theme.lineNumber = GradientColor::fromHex(0x81A081);
    theme.lineNumberActive = GradientColor::fromHex(0xC5E1A5);
    theme.guideLine = GradientColor::fromHex(0x1A2A1A);
    theme.minimap = GradientColor::fromHex(0x0A1A0A);
    
    // Window colors
    theme.titleBar = GradientColor::multiGradient({
        {0.0f, 0.1f, 0.18f, 0.12f, 1.0f},
        {1.0f, 0.12f, 0.2f, 0.14f, 1.0f}
    });
    theme.titleBarInactive = GradientColor::fromHex(0x0A1A0A);
    theme.statusBar = GradientColor::fromHex(0x051005);
    theme.toolbar = GradientColor::fromHex(0x1A2A1A);
    
    return theme;
}

Theme ThemeManager::createCyberpunkTheme() {
    Theme theme;
    theme.id = "cyberpunk";
    theme.name = "Cyberpunk Neon";
    theme.isDark = true;
    
    // Dark cyberpunk background
    theme.backgroundPrimary = GradientColor::fromHex(0x0D0D1A);
    theme.backgroundSecondary = GradientColor::multiGradient({
        {0.0f, 0.1f, 0.05f, 0.15f, 1.0f},
        {1.0f, 0.15f, 0.05f, 0.2f, 1.0f}
    });
    theme.backgroundTertiary = GradientColor::fromHex(0x1A1A2E);
    
    // Surface colors with neon tint
    theme.surface = GradientColor::multiGradient({
        {0.0f, 0.15f, 0.08f, 0.2f, 1.0f},
        {1.0f, 0.2f, 0.08f, 0.25f, 1.0f}
    });
    theme.surfaceHover = GradientColor::fromHex(0x2A1A3A);
    theme.surfaceActive = GradientColor::fromHex(0x3A2A4A);
    theme.surfaceSelected = GradientColor::fromHex(0x302040);
    
    // Text colors - bright
    theme.textPrimary = GradientColor::fromHex(0xFFFFFF);
    theme.textSecondary = GradientColor::fromHex(0xCC99FF);
    theme.textDisabled = GradientColor::fromHex(0x666688);
    theme.textInverse = GradientColor::fromHex(0x0D0D1A);
    
    // Cyberpunk accent - Neon pink and cyan
    theme.accentPrimary = GradientColor::multiGradient({
        {0.0f, 1.0f, 0.0f, 0.5f, 1.0f},    // Neon Pink
        {0.5f, 1.0f, 0.2f, 0.8f, 1.0f},    // Hot Pink
        {1.0f, 0.0f, 1.0f, 1.0f, 1.0f}     // Cyan
    });
    theme.accentSecondary = GradientColor::fromHex(0xFF00FF);
    theme.accentHover = GradientColor::fromHex(0xFF33FF);
    theme.accentActive = GradientColor::fromHex(0x00FFFF);
    
    // Border colors with glow effect
    theme.border = GradientColor::fromHex(0x3A2A4A);
    theme.borderHover = GradientColor::fromHex(0xFF00FF);
    theme.borderFocus = GradientColor::fromHex(0x00FFFF);
    
    // Status colors - neon
    theme.success = GradientColor::fromHex(0x00FF00);
    theme.warning = GradientColor::fromHex(0xFFFF00);
    theme.error = GradientColor::fromHex(0xFF0066);
    theme.info = GradientColor::fromHex(0x00FFFF);
    
    // Syntax highlighting - Cyberpunk palette
    theme.syntaxKeyword = GradientColor::fromHex(0xFF00FF);
    theme.syntaxType = GradientColor::fromHex(0x00FFFF);
    theme.syntaxFunction = GradientColor::fromHex(0x00FF00);
    theme.syntaxVariable = GradientColor::fromHex(0xFFFFFF);
    theme.syntaxLiteral = GradientColor::fromHex(0xFFFF00);
    theme.syntaxComment = GradientColor::fromHex(0x666699);
    theme.syntaxString = GradientColor::fromHex(0x00FF66);
    theme.syntaxNumber = GradientColor::fromHex(0xFF6600);
    theme.syntaxOperator = GradientColor::fromHex(0xFF0066);
    theme.syntaxTag = GradientColor::fromHex(0xFF00FF);
    theme.syntaxAttribute = GradientColor::fromHex(0xFFFF00);
    
    // UI elements
    theme.scrollbar = GradientColor::fromHex(0x3A2A4A);
    theme.scrollbarHover = GradientColor::fromHex(0xFF00FF);
    theme.selection = GradientColor::fromHex(0x2A1A3A);
    theme.highlight = GradientColor::fromHex(0x2A1A3A);
    theme.cursor = GradientColor::fromHex(0x00FFFF);
    theme.lineNumber = GradientColor::fromHex(0x666699);
    theme.lineNumberActive = GradientColor::fromHex(0xFF00FF);
    theme.guideLine = GradientColor::fromHex(0x1A1A2E);
    theme.minimap = GradientColor::fromHex(0x050510);
    
    // Window colors
    theme.titleBar = GradientColor::multiGradient({
        {0.0f, 0.1f, 0.05f, 0.15f, 1.0f},
        {1.0f, 0.15f, 0.05f, 0.2f, 1.0f}
    });
    theme.titleBarInactive = GradientColor::fromHex(0x0D0D1A);
    theme.statusBar = GradientColor::fromHex(0x050510);
    theme.toolbar = GradientColor::fromHex(0x0D0D1A);
    
    return theme;
}

Theme ThemeManager::createMonokaiTheme() {
    Theme theme;
    theme.id = "monokai";
    theme.name = "Monokai Vivid";
    theme.isDark = true;
    
    theme.backgroundPrimary = GradientColor::fromHex(0x272822);
    theme.backgroundSecondary = GradientColor::fromHex(0x1E1F1C);
    theme.backgroundTertiary = GradientColor::fromHex(0x3E3D32);
    
    theme.surface = GradientColor::fromHex(0x3E3D32);
    theme.surfaceHover = GradientColor::fromHex(0x4E4D42);
    theme.surfaceActive = GradientColor::fromHex(0x5E5D52);
    theme.surfaceSelected = GradientColor::fromHex(0x49483E);
    
    theme.textPrimary = GradientColor::fromHex(0xF8F8F2);
    theme.textSecondary = GradientColor::fromHex(0xA6A693);
    theme.textDisabled = GradientColor::fromHex(0x75715E);
    theme.textInverse = GradientColor::fromHex(0x272822);
    
    theme.accentPrimary = GradientColor::fromHex(0xA6E22E);
    theme.accentSecondary = GradientColor::fromHex(0x66D9EF);
    theme.accentHover = GradientColor::fromHex(0xB9E255);
    theme.accentActive = GradientColor::fromHex(0x8BC34A);
    
    theme.border = GradientColor::fromHex(0x49483E);
    theme.borderHover = GradientColor::fromHex(0x59584E);
    theme.borderFocus = GradientColor::fromHex(0xA6E22E);
    
    theme.success = GradientColor::fromHex(0xA6E22E);
    theme.warning = GradientColor::fromHex(0xE6DB74);
    theme.error = GradientColor::fromHex(0xF92672);
    theme.info = GradientColor::fromHex(0x66D9EF);
    
    theme.syntaxKeyword = GradientColor::fromHex(0xF92672);
    theme.syntaxType = GradientColor::fromHex(0x66D9EF);
    theme.syntaxFunction = GradientColor::fromHex(0xA6E22E);
    theme.syntaxVariable = GradientColor::fromHex(0xF8F8F2);
    theme.syntaxLiteral = GradientColor::fromHex(0xAE81FF);
    theme.syntaxComment = GradientColor::fromHex(0x75715E);
    theme.syntaxString = GradientColor::fromHex(0xE6DB74);
    theme.syntaxNumber = GradientColor::fromHex(0xAE81FF);
    theme.syntaxOperator = GradientColor::fromHex(0xF92672);
    theme.syntaxTag = GradientColor::fromHex(0xF92672);
    theme.syntaxAttribute = GradientColor::fromHex(0xA6E22E);
    
    theme.scrollbar = GradientColor::fromHex(0x49483E);
    theme.scrollbarHover = GradientColor::fromHex(0x59584E);
    theme.selection = GradientColor::fromHex(0x49483E);
    theme.highlight = GradientColor::fromHex(0x3E3D32);
    theme.cursor = GradientColor::fromHex(0xF8F8F0);
    theme.lineNumber = GradientColor::fromHex(0x75715E);
    theme.lineNumberActive = GradientColor::fromHex(0xF8F8F2);
    theme.guideLine = GradientColor::fromHex(0x3E3D32);
    theme.minimap = GradientColor::fromHex(0x1E1F1C);
    
    theme.titleBar = GradientColor::fromHex(0x1E1F1C);
    theme.titleBarInactive = GradientColor::fromHex(0x272822);
    theme.statusBar = GradientColor::fromHex(0x1E1F1C);
    theme.toolbar = GradientColor::fromHex(0x272822);
    
    return theme;
}

Theme ThemeManager::createDraculaTheme() {
    Theme theme;
    theme.id = "dracula";
    theme.name = "Dracula";
    theme.isDark = true;
    
    theme.backgroundPrimary = GradientColor::fromHex(0x282A36);
    theme.backgroundSecondary = GradientColor::fromHex(0x21222C);
    theme.backgroundTertiary = GradientColor::fromHex(0x343746);
    
    theme.surface = GradientColor::fromHex(0x343746);
    theme.surfaceHover = GradientColor::fromHex(0x44475A);
    theme.surfaceActive = GradientColor::fromHex(0x55586B);
    theme.surfaceSelected = GradientColor::fromHex(0x44475A);
    
    theme.textPrimary = GradientColor::fromHex(0xF8F8F2);
    theme.textSecondary = GradientColor::fromHex(0xB0B0B0);
    theme.textDisabled = GradientColor::fromHex(0x6272A4);
    theme.textInverse = GradientColor::fromHex(0x282A36);
    
    theme.accentPrimary = GradientColor::fromHex(0xFF79C6);
    theme.accentSecondary = GradientColor::fromHex(0xBD93F9);
    theme.accentHover = GradientColor::fromHex(0xFF92D0);
    theme.accentActive = GradientColor::fromHex(0x8B5FC4);
    
    theme.border = GradientColor::fromHex(0x44475A);
    theme.borderHover = GradientColor::fromHex(0x55586B);
    theme.borderFocus = GradientColor::fromHex(0xBD93F9);
    
    theme.success = GradientColor::fromHex(0x50FA7B);
    theme.warning = GradientColor::fromHex(0xF1FA8C);
    theme.error = GradientColor::fromHex(0xFF5555);
    theme.info = GradientColor::fromHex(0x8BE9FD);
    
    theme.syntaxKeyword = GradientColor::fromHex(0xFF79C6);
    theme.syntaxType = GradientColor::fromHex(0x8BE9FD);
    theme.syntaxFunction = GradientColor::fromHex(0x50FA7B);
    theme.syntaxVariable = GradientColor::fromHex(0xF8F8F2);
    theme.syntaxLiteral = GradientColor::fromHex(0xF1FA8C);
    theme.syntaxComment = GradientColor::fromHex(0x6272A4);
    theme.syntaxString = GradientColor::fromHex(0x50FA7B);
    theme.syntaxNumber = GradientColor::fromHex(0xBD93F9);
    theme.syntaxOperator = GradientColor::fromHex(0xFF79C6);
    theme.syntaxTag = GradientColor::fromHex(0xFF79C6);
    theme.syntaxAttribute = GradientColor::fromHex(0x50FA7B);
    
    theme.scrollbar = GradientColor::fromHex(0x44475A);
    theme.scrollbarHover = GradientColor::fromHex(0x55586B);
    theme.selection = GradientColor::fromHex(0x44475A);
    theme.highlight = GradientColor::fromHex(0x44475A);
    theme.cursor = GradientColor::fromHex(0xF8F8F0);
    theme.lineNumber = GradientColor::fromHex(0x6272A4);
    theme.lineNumberActive = GradientColor::fromHex(0xF8F8F2);
    theme.guideLine = GradientColor::fromHex(0x343746);
    theme.minimap = GradientColor::fromHex(0x21222C);
    
    theme.titleBar = GradientColor::fromHex(0x21222C);
    theme.titleBarInactive = GradientColor::fromHex(0x282A36);
    theme.statusBar = GradientColor::fromHex(0x21222C);
    theme.toolbar = GradientColor::fromHex(0x282A36);
    
    return theme;
}

Theme ThemeManager::createNordTheme() {
    Theme theme;
    theme.id = "nord";
    theme.name = "Nordic Frost";
    theme.isDark = true;
    
    theme.backgroundPrimary = GradientColor::fromHex(0x2E3440);
    theme.backgroundSecondary = GradientColor::fromHex(0x242933);
    theme.backgroundTertiary = GradientColor::fromHex(0x3B4252);
    
    theme.surface = GradientColor::fromHex(0x3B4252);
    theme.surfaceHover = GradientColor::fromHex(0x434C5E);
    theme.surfaceActive = GradientColor::fromHex(0x4C566A);
    theme.surfaceSelected = GradientColor::fromHex(0x434C5E);
    
    theme.textPrimary = GradientColor::fromHex(0xECEFF4);
    theme.textSecondary = GradientColor::fromHex(0xD8DEE9);
    theme.textDisabled = GradientColor::fromHex(0x4C566A);
    theme.textInverse = GradientColor::fromHex(0x2E3440);
    
    theme.accentPrimary = GradientColor::fromHex(0x88C0D0);
    theme.accentSecondary = GradientColor::fromHex(0x81A1C1);
    theme.accentHover = GradientColor::fromHex(0x8FBCBB);
    theme.accentActive = GradientColor::fromHex(0x5E81AC);
    
    theme.border = GradientColor::fromHex(0x4C566A);
    theme.borderHover = GradientColor::fromHex(0x555F75);
    theme.borderFocus = GradientColor::fromHex(0x88C0D0);
    
    theme.success = GradientColor::fromHex(0xA3BE8C);
    theme.warning = GradientColor::fromHex(0xEBCB8B);
    theme.error = GradientColor::fromHex(0xBF616A);
    theme.info = GradientColor::fromHex(0x88C0D0);
    
    theme.syntaxKeyword = GradientColor::fromHex(0x81A1C1);
    theme.syntaxType = GradientColor::fromHex(0x8FBCBB);
    theme.syntaxFunction = GradientColor::fromHex(0x88C0D0);
    theme.syntaxVariable = GradientColor::fromHex(0xD8DEE9);
    theme.syntaxLiteral = GradientColor::fromHex(0xEBCB8B);
    theme.syntaxComment = GradientColor::fromHex(0x4C566A);
    theme.syntaxString = GradientColor::fromHex(0xA3BE8C);
    theme.syntaxNumber = GradientColor::fromHex(0xB48EAD);
    theme.syntaxOperator = GradientColor::fromHex(0x81A1C1);
    theme.syntaxTag = GradientColor::fromHex(0x81A1C1);
    theme.syntaxAttribute = GradientColor::fromHex(0x8FBCBB);
    
    theme.scrollbar = GradientColor::fromHex(0x4C566A);
    theme.scrollbarHover = GradientColor::fromHex(0x555F75);
    theme.selection = GradientColor::fromHex(0x434C5E);
    theme.highlight = GradientColor::fromHex(0x434C5E);
    theme.cursor = GradientColor::fromHex(0xD8DEE9);
    theme.lineNumber = GradientColor::fromHex(0x4C566A);
    theme.lineNumberActive = GradientColor::fromHex(0xD8DEE9);
    theme.guideLine = GradientColor::fromHex(0x3B4252);
    theme.minimap = GradientColor::fromHex(0x242933);
    
    theme.titleBar = GradientColor::fromHex(0x242933);
    theme.titleBarInactive = GradientColor::fromHex(0x2E3440);
    theme.statusBar = GradientColor::fromHex(0x242933);
    theme.toolbar = GradientColor::fromHex(0x2E3440);
    
    return theme;
}

Theme ThemeManager::createGruvboxTheme() {
    Theme theme;
    theme.id = "gruvbox";
    theme.name = "Gruvbox Warm";
    theme.isDark = true;
    
    theme.backgroundPrimary = GradientColor::fromHex(0x282828);
    theme.backgroundSecondary = GradientColor::fromHex(0x1D2021);
    theme.backgroundTertiary = GradientColor::fromHex(0x3C3836);
    
    theme.surface = GradientColor::fromHex(0x3C3836);
    theme.surfaceHover = GradientColor::fromHex(0x504945);
    theme.surfaceActive = GradientColor::fromHex(0x665C54);
    theme.surfaceSelected = GradientColor::fromHex(0x504945);
    
    theme.textPrimary = GradientColor::fromHex(0xEBDBB2);
    theme.textSecondary = GradientColor::fromHex(0xD5C4A1);
    theme.textDisabled = GradientColor::fromHex(0x665C54);
    theme.textInverse = GradientColor::fromHex(0x282828);
    
    theme.accentPrimary = GradientColor::fromHex(0xFE8019);
    theme.accentSecondary = GradientColor::fromHex(0xB8BB26);
    theme.accentHover = GradientColor::fromHex(0xFF9835);
    theme.accentActive = GradientColor::fromHex(0xD79921);
    
    theme.border = GradientColor::fromHex(0x665C54);
    theme.borderHover = GradientColor::fromHex(0x7C6F64);
    theme.borderFocus = GradientColor::fromHex(0xFE8019);
    
    theme.success = GradientColor::fromHex(0xB8BB26);
    theme.warning = GradientColor::fromHex(0xFABD2F);
    theme.error = GradientColor::fromHex(0xFB4934);
    theme.info = GradientColor::fromHex(0x83A598);
    
    theme.syntaxKeyword = GradientColor::fromHex(0xFE8019);
    theme.syntaxType = GradientColor::fromHex(0x8EC07C);
    theme.syntaxFunction = GradientColor::fromHex(0xB8BB26);
    theme.syntaxVariable = GradientColor::fromHex(0xEBDBB2);
    theme.syntaxLiteral = GradientColor::fromHex(0xD3869B);
    theme.syntaxComment = GradientColor::fromHex(0x928374);
    theme.syntaxString = GradientColor::fromHex(0xB8BB26);
    theme.syntaxNumber = GradientColor::fromHex(0xD3869B);
    theme.syntaxOperator = GradientColor::fromHex(0xFE8019);
    theme.syntaxTag = GradientColor::fromHex(0xFE8019);
    theme.syntaxAttribute = GradientColor::fromHex(0xFABD2F);
    
    theme.scrollbar = GradientColor::fromHex(0x665C54);
    theme.scrollbarHover = GradientColor::fromHex(0x7C6F64);
    theme.selection = GradientColor::fromHex(0x504945);
    theme.highlight = GradientColor::fromHex(0x504945);
    theme.cursor = GradientColor::fromHex(0xEBDBB2);
    theme.lineNumber = GradientColor::fromHex(0x928374);
    theme.lineNumberActive = GradientColor::fromHex(0xEBDBB2);
    theme.guideLine = GradientColor::fromHex(0x3C3836);
    theme.minimap = GradientColor::fromHex(0x1D2021);
    
    theme.titleBar = GradientColor::fromHex(0x1D2021);
    theme.titleBarInactive = GradientColor::fromHex(0x282828);
    theme.statusBar = GradientColor::fromHex(0x1D2021);
    theme.toolbar = GradientColor::fromHex(0x282828);
    
    return theme;
}

Theme ThemeManager::createSolarizedTheme() {
    Theme theme;
    theme.id = "solarized";
    theme.name = "Solarized Dark";
    theme.isDark = true;
    
    theme.backgroundPrimary = GradientColor::fromHex(0x002B36);
    theme.backgroundSecondary = GradientColor::fromHex(0x00252E);
    theme.backgroundTertiary = GradientColor::fromHex(0x073642);
    
    theme.surface = GradientColor::fromHex(0x073642);
    theme.surfaceHover = GradientColor::fromHex(0x0A404D);
    theme.surfaceActive = GradientColor::fromHex(0x0D4B5A);
    theme.surfaceSelected = GradientColor::fromHex(0x0A404D);
    
    theme.textPrimary = GradientColor::fromHex(0x839496);
    theme.textSecondary = GradientColor::fromHex(0x657B83);
    theme.textDisabled = GradientColor::fromHex(0x586E75);
    theme.textInverse = GradientColor::fromHex(0x002B36);
    
    theme.accentPrimary = GradientColor::fromHex(0x268BD2);
    theme.accentSecondary = GradientColor::fromHex(0x2AA198);
    theme.accentHover = GradientColor::fromHex(0x3FA3D9);
    theme.accentActive = GradientColor::fromHex(0x1E7BB5);
    
    theme.border = GradientColor::fromHex(0x586E75);
    theme.borderHover = GradientColor::fromHex(0x657B83);
    theme.borderFocus = GradientColor::fromHex(0x268BD2);
    
    theme.success = GradientColor::fromHex(0x859900);
    theme.warning = GradientColor::fromHex(0xB58900);
    theme.error = GradientColor::fromHex(0xDC322F);
    theme.info = GradientColor::fromHex(0x268BD2);
    
    theme.syntaxKeyword = GradientColor::fromHex(0x859900);
    theme.syntaxType = GradientColor::fromHex(0xB58900);
    theme.syntaxFunction = GradientColor::fromHex(0x268BD2);
    theme.syntaxVariable = GradientColor::fromHex(0x839496);
    theme.syntaxLiteral = GradientColor::fromHex(0x2AA198);
    theme.syntaxComment = GradientColor::fromHex(0x586E75);
    theme.syntaxString = GradientColor::fromHex(0x2AA198);
    theme.syntaxNumber = GradientColor::fromHex(0xD33682);
    theme.syntaxOperator = GradientColor::fromHex(0x859900);
    theme.syntaxTag = GradientColor::fromHex(0x268BD2);
    theme.syntaxAttribute = GradientColor::fromHex(0xB58900);
    
    theme.scrollbar = GradientColor::fromHex(0x586E75);
    theme.scrollbarHover = GradientColor::fromHex(0x657B83);
    theme.selection = GradientColor::fromHex(0x0A404D);
    theme.highlight = GradientColor::fromHex(0x0A404D);
    theme.cursor = GradientColor::fromHex(0x839496);
    theme.lineNumber = GradientColor::fromHex(0x586E75);
    theme.lineNumberActive = GradientColor::fromHex(0x839496);
    theme.guideLine = GradientColor::fromHex(0x073642);
    theme.minimap = GradientColor::fromHex(0x00252E);
    
    theme.titleBar = GradientColor::fromHex(0x00252E);
    theme.titleBarInactive = GradientColor::fromHex(0x002B36);
    theme.statusBar = GradientColor::fromHex(0x00252E);
    theme.toolbar = GradientColor::fromHex(0x002B36);
    
    return theme;
}

Theme ThemeManager::createRainbowTheme() {
    Theme theme;
    theme.id = "rainbow";
    theme.name = "Rainbow Bright";
    theme.isDark = true;
    
    // Dark background to make rainbow pop
    theme.backgroundPrimary = GradientColor::fromHex(0x1A1A2E);
    theme.backgroundSecondary = GradientColor::multiGradient({
        {0.0f, 0.1f, 0.1f, 0.2f, 1.0f},
        {0.2f, 0.15f, 0.1f, 0.2f, 1.0f},
        {0.4f, 0.1f, 0.15f, 0.2f, 1.0f},
        {0.6f, 0.1f, 0.1f, 0.25f, 1.0f},
        {0.8f, 0.15f, 0.1f, 0.2f, 1.0f},
        {1.0f, 0.1f, 0.1f, 0.2f, 1.0f}
    });
    theme.backgroundTertiary = GradientColor::fromHex(0x2A2A4E);
    
    theme.surface = GradientColor::fromHex(0x2A2A4E);
    theme.surfaceHover = GradientColor::fromHex(0x3A3A5E);
    theme.surfaceActive = GradientColor::fromHex(0x4A4A6E);
    theme.surfaceSelected = GradientColor::fromHex(0x3A3A5E);
    
    theme.textPrimary = GradientColor::fromHex(0xFFFFFF);
    theme.textSecondary = GradientColor::fromHex(0xE0E0E0);
    theme.textDisabled = GradientColor::fromHex(0x808080);
    theme.textInverse = GradientColor::fromHex(0x1A1A2E);
    
    // Full rainbow accent
    theme.accentPrimary = GradientColor::multiGradient({
        {0.0f, 1.0f, 0.0f, 0.0f, 1.0f},
        {0.17f, 1.0f, 0.5f, 0.0f, 1.0f},
        {0.33f, 1.0f, 1.0f, 0.0f, 1.0f},
        {0.5f, 0.0f, 1.0f, 0.0f, 1.0f},
        {0.67f, 0.0f, 0.5f, 1.0f, 1.0f},
        {0.83f, 0.5f, 0.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 0.0f, 0.5f, 1.0f}
    });
    theme.accentSecondary = GradientColor::fromHex(0xFF6B6B);
    theme.accentHover = GradientColor::fromHex(0x4ECDC4);
    theme.accentActive = GradientColor::fromHex(0x45B7D1);
    
    theme.border = GradientColor::fromHex(0x4A4A6A);
    theme.borderHover = GradientColor::multiGradient({
        GradientColor::Stop{0.0f, 1.0f, 0.5f, 0.5f, 1.0f},
        GradientColor::Stop{1.0f, 0.5f, 0.5f, 1.0f, 1.0f}
    });
    theme.borderFocus = GradientColor::fromHex(0xFFFFFF);
    
    theme.success = GradientColor::fromHex(0x00FF00);
    theme.warning = GradientColor::fromHex(0xFFFF00);
    theme.error = GradientColor::fromHex(0xFF0000);
    theme.info = GradientColor::fromHex(0x00FFFF);
    
    // Rainbow syntax highlighting
    theme.syntaxKeyword = GradientColor::fromHex(0xFF6B6B);
    theme.syntaxType = GradientColor::fromHex(0xFFD93D);
    theme.syntaxFunction = GradientColor::fromHex(0x6BCB77);
    theme.syntaxVariable = GradientColor::fromHex(0xFFFFFF);
    theme.syntaxLiteral = GradientColor::fromHex(0xFFD93D);
    theme.syntaxComment = GradientColor::fromHex(0x6B7280);
    theme.syntaxString = GradientColor::fromHex(0x6BCB77);
    theme.syntaxNumber = GradientColor::fromHex(0x4D96FF);
    theme.syntaxOperator = GradientColor::fromHex(0xC77DFF);
    theme.syntaxTag = GradientColor::fromHex(0xFF6B6B);
    theme.syntaxAttribute = GradientColor::fromHex(0xFFD93D);
    
    theme.scrollbar = GradientColor::fromHex(0x4A4A6A);
    theme.scrollbarHover = GradientColor::multiGradient({
        GradientColor::Stop{0.0f, 1.0f, 0.0f, 0.0f, 1.0f},
        GradientColor::Stop{1.0f, 0.0f, 0.0f, 1.0f, 1.0f}
    });
    theme.selection = GradientColor::fromHex(0x3A3A5E);
    theme.highlight = GradientColor::fromHex(0x3A3A5E);
    theme.cursor = GradientColor::multiGradient({
        GradientColor::Stop{0.0f, 1.0f, 1.0f, 1.0f, 1.0f},
        GradientColor::Stop{1.0f, 1.0f, 1.0f, 1.0f, 1.0f}
    });
    theme.lineNumber = GradientColor::fromHex(0x6B7280);
    theme.lineNumberActive = GradientColor::fromHex(0xFFFFFF);
    theme.guideLine = GradientColor::fromHex(0x2A2A4E);
    theme.minimap = GradientColor::fromHex(0x1A1A2E);
    
    theme.titleBar = GradientColor::multiGradient({
        {0.0f, 0.15f, 0.1f, 0.2f, 1.0f},
        {0.5f, 0.2f, 0.1f, 0.25f, 1.0f},
        {1.0f, 0.15f, 0.1f, 0.2f, 1.0f}
    });
    theme.titleBarInactive = GradientColor::fromHex(0x1A1A2E);
    theme.statusBar = GradientColor::fromHex(0x1A1A2E);
    theme.toolbar = GradientColor::fromHex(0x1E1E3E);
    
    return theme;
}

} // namespace NexusForge::UI
