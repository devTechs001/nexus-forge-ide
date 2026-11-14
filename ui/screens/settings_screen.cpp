// ui/screens/settings_screen.cpp
// Settings Screen Implementation

#include "settings_screen.hpp"
#include "../framework/widget_system.hpp"
#include "../widgets/text_input.hpp"
#include "../widgets/button.hpp"
#include "../../config/settings_manager.hpp"

namespace NexusForge::UI {

SettingsScreen::SettingsScreen() = default;

SettingsScreen::~SettingsScreen() = default;

bool SettingsScreen::initialize(Config::SettingsManager* settings) {
    settings_ = settings;
    
    // Build settings UI
    buildUI();
    
    return true;
}

void SettingsScreen::buildUI() {
    // Create categories sidebar
    createCategoriesList();
    
    // Create settings content area
    createSettingsContent();
}

void SettingsScreen::createCategoriesList() {
    // Common settings categories
    categories_ = {
        {"editor", "Editor", "📝"},
        {"appearance", "Appearance", "🎨"},
        {"keybindings", "Keybindings", "⌨️"},
        {"extensions", "Extensions", "🔌"},
        {"terminal", "Terminal", "💻"},
        {"ai", "AI Features", "🤖"},
        {"workspace", "Workspace", "📁"},
        {"security", "Security", "🔒"},
    };
}

void SettingsScreen::createSettingsContent() {
    // Create editor settings
    editorSettings_ = {
        {"fontFamily", "Font Family", "JetBrains Mono", SettingType::String},
        {"fontSize", "Font Size", "14", SettingType::Number},
        {"lineHeight", "Line Height", "1.5", SettingType::Number},
        {"tabSize", "Tab Size", "4", SettingType::Number},
        {"insertSpaces", "Insert Spaces", "true", SettingType::Boolean},
        {"wordWrap", "Word Wrap", "off", SettingType::Enum},
        {"minimap", "Show Minimap", "true", SettingType::Boolean},
        {"lineNumbers", "Line Numbers", "on", SettingType::Enum},
        {"bracketPairColorization", "Bracket Pair Colorization", "true", SettingType::Boolean},
        {"formatOnSave", "Format On Save", "false", SettingType::Boolean},
        {"autoSave", "Auto Save", "off", SettingType::Enum},
    };
    
    // Create appearance settings
    appearanceSettings_ = {
        {"theme", "Color Theme", "NexusForge Dark", SettingType::Enum},
        {"iconTheme", "Icon Theme", "nexusforge-icons", SettingType::Enum},
        {"activityBarLocation", "Activity Bar Location", "left", SettingType::Enum},
        {"panelLocation", "Panel Location", "bottom", SettingType::Enum},
        {"fontSize", "Window Zoom", "100%", SettingType::Number},
    };
}

void SettingsScreen::show() {
    visible_ = true;
    selectedCategory_ = "editor";
    loadCurrentSettings();
}

void SettingsScreen::hide() {
    visible_ = false;
}

void SettingsScreen::loadCurrentSettings() {
    if (!settings_) return;
    
    // Load actual settings values
    // This would read from the settings manager
}

void SettingsScreen::saveSettings() {
    if (!settings_) return;
    
    // Save settings to the settings manager
    // This would persist the changes
    
    if (onSettingsSaved) {
        onSettingsSaved();
    }
}

void SettingsScreen::resetSettings() {
    if (!settings_) return;
    
    // Reset to defaults
    loadCurrentSettings();
}

void SettingsScreen::render(RenderEngine& renderer) {
    if (!visible_) return;
    
    const Rect& bounds = bounds_;
    
    // Draw background
    renderer.drawRect(bounds, Color(0.1f, 0.1f, 0.12f));
    
    // Draw header
    Rect headerRect(bounds.x, bounds.y, bounds.width, 50);
    renderer.drawRect(headerRect, Color(0.15f, 0.15f, 0.18f));
    renderer.drawText("Settings", bounds.x + 20, bounds.y + 20, 
                     Color(0.9f, 0.9f, 0.9f), "default", 20);
    
    // Draw categories sidebar
    Rect sidebarRect(bounds.x, bounds.y + 50, 200, bounds.height - 100);
    renderer.drawRect(sidebarRect, Color(0.12f, 0.12f, 0.15f));
    
    float y = bounds.y + 60;
    for (const auto& cat : categories_) {
        Color bgColor = (selectedCategory_ == cat.id) ? 
                       Color(0.2f, 0.4f, 0.8f, 0.3f) : Color::Transparent();
        
        renderer.drawRect(Rect(bounds.x, y, 200, 36), bgColor);
        
        std::string text = cat.icon + "  " + cat.name;
        Color textColor = (selectedCategory_ == cat.id) ? 
                         Color(0.9f, 0.9f, 0.9f) : Color(0.6f, 0.6f, 0.65f);
        
        renderer.drawText(text, bounds.x + 16, y + 22, textColor, "default", 14);
        y += 36;
    }
    
    // Draw settings content
    Rect contentRect(bounds.x + 200, bounds.y + 50, bounds.width - 200, bounds.height - 100);
    renderer.drawRect(contentRect, Color(0.1f, 0.1f, 0.12f));
    
    // Draw current category settings
    drawSettingsContent(renderer, contentRect);
    
    // Draw footer with buttons
    Rect footerRect(bounds.x, bounds.y + bounds.height - 50, bounds.width, 50);
    renderer.drawRect(footerRect, Color(0.15f, 0.15f, 0.18f));
    
    // Save button
    renderer.drawRoundedRect(Rect(bounds.x + bounds.width - 220, bounds.y + bounds.height - 40, 100, 30),
                            Color(0.25f, 0.4f, 0.7f), 4);
    renderer.drawText("Save", bounds.x + bounds.width - 180, bounds.y + bounds.height - 20,
                     Color(1.0f, 1.0f, 1.0f), "default", 14);
    
    // Cancel button
    renderer.drawRoundedRect(Rect(bounds.x + bounds.width - 110, bounds.y + bounds.height - 40, 90, 30),
                            Color(0.2f, 0.2f, 0.25f), 4);
    renderer.drawText("Cancel", bounds.x + bounds.width - 95, bounds.y + bounds.height - 20,
                     Color(0.9f, 0.9f, 0.9f), "default", 14);
}

void SettingsScreen::drawSettingsContent(RenderEngine& renderer, const Rect& rect) {
    float y = rect.y + 20;
    float x = rect.x + 20;
    
    if (selectedCategory_ == "editor") {
        for (const auto& setting : editorSettings_) {
            renderer.drawText(setting.name, x, y, Color(0.9f, 0.9f, 0.9f), "default", 14);
            
            // Draw value input/display
            Rect valueRect(x + 200, y - 10, 200, 24);
            renderer.drawRoundedRect(valueRect, Color(0.15f, 0.15f, 0.18f), 4);
            renderer.drawText(setting.value, x + 210, y, Color(0.7f, 0.7f, 0.75f), "default", 14);
            
            y += 40;
        }
    } else if (selectedCategory_ == "appearance") {
        for (const auto& setting : appearanceSettings_) {
            renderer.drawText(setting.name, x, y, Color(0.9f, 0.9f, 0.9f), "default", 14);
            
            Rect valueRect(x + 200, y - 10, 200, 24);
            renderer.drawRoundedRect(valueRect, Color(0.15f, 0.15f, 0.18f), 4);
            renderer.drawText(setting.value, x + 210, y, Color(0.7f, 0.7f, 0.75f), "default", 14);
            
            y += 40;
        }
    }
}

bool SettingsScreen::onMouseDown(const Core::MouseButtonPressEvent& event) {
    if (!visible_) return false;
    
    // Handle category selection
    float y = bounds_.y + 60;
    for (const auto& cat : categories_) {
        if (event.x >= bounds_.x && event.x < bounds_.x + 200 &&
            event.y >= y && event.y < y + 36) {
            selectedCategory_ = cat.id;
            return true;
        }
        y += 36;
    }
    
    return false;
}

} // namespace NexusForge::UI
