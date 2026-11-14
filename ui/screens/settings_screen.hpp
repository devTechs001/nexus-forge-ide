// ui/screens/settings_screen.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include "../renderer/render_engine.hpp"
#include "../../config/settings_manager.hpp"

#include <vector>
#include <string>
#include <functional>

namespace NexusForge::UI {

// Setting types
enum class SettingType {
    String,
    Number,
    Boolean,
    Enum,
    Color,
    Font,
    Keybinding
};

// Setting info
struct SettingInfo {
    std::string id;
    std::string name;
    std::string value;
    SettingType type;
    std::vector<std::string> enumValues;  // For enum type
    std::string description;
    std::string defaultValue;
};

// Settings category
struct SettingsCategory {
    std::string id;
    std::string name;
    std::string icon;
};

// Settings screen class
class SettingsScreen {
public:
    SettingsScreen();
    ~SettingsScreen();

    bool initialize(Config::SettingsManager* settings);
    void show();
    void hide();

    // Settings management
    void loadCurrentSettings();
    void saveSettings();
    void resetSettings();

    // Render
    void render(RenderEngine& renderer);

    // Event handling
    bool onMouseDown(const Core::MouseButtonPressEvent& event);

    // Callbacks
    std::function<void()> onSettingsSaved;
    std::function<void()> onSettingsChanged;

private:
    Config::SettingsManager* settings_ = nullptr;
    bool visible_ = false;
    Rect bounds_{0, 0, 1280, 720};

    std::vector<SettingsCategory> categories_;
    std::string selectedCategory_;

    std::vector<SettingInfo> editorSettings_;
    std::vector<SettingInfo> appearanceSettings_;
    std::vector<SettingInfo> keybindingSettings_;
    std::vector<SettingInfo> terminalSettings_;
    std::vector<SettingInfo> aiSettings_;

    // UI helpers
    void buildUI();
    void createCategoriesList();
    void createSettingsContent();
    void drawSettingsContent(RenderEngine& renderer, const Rect& rect);

    // Setting value editors
    void drawStringSetting(RenderEngine& renderer, float x, float y, const SettingInfo& setting);
    void drawNumberSetting(RenderEngine& renderer, float x, float y, const SettingInfo& setting);
    void drawBooleanSetting(RenderEngine& renderer, float x, float y, const SettingInfo& setting);
    void drawEnumSetting(RenderEngine& renderer, float x, float y, const SettingInfo& setting);
};

} // namespace NexusForge::UI
