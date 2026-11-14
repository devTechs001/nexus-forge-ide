// config/settings_manager.hpp
#pragma once

#include "keybinding_manager.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>
#include <mutex>

namespace NexusForge::Config {

// Settings value type
struct SettingsValue {
    enum class Type { Bool, Int, Float, String, Array, Object };
    
    Type type = Type::String;
    bool boolValue = false;
    int intValue = 0;
    float floatValue = 0.0f;
    std::string stringValue;
    std::vector<SettingsValue> arrayValue;
    std::unordered_map<std::string, SettingsValue> objectValue;
    
    static SettingsValue fromBool(bool v) { SettingsValue sv; sv.type = Type::Bool; sv.boolValue = v; return sv; }
    static SettingsValue fromInt(int v) { SettingsValue sv; sv.type = Type::Int; sv.intValue = v; return sv; }
    static SettingsValue fromFloat(float v) { SettingsValue sv; sv.type = Type::Float; sv.floatValue = v; return sv; }
    static SettingsValue fromString(const std::string& v) { SettingsValue sv; sv.type = Type::String; sv.stringValue = v; return sv; }
    
    bool toBool() const { return boolValue; }
    int toInt() const { return intValue; }
    float toFloat() const { return floatValue; }
    const std::string& toString() const { return stringValue; }
};

// Editor settings
struct EditorSettings {
    std::string fontFamily = "JetBrains Mono";
    float fontSize = 14.0f;
    float lineHeight = 1.5f;
    bool showLineNumbers = true;
    bool showMinimap = true;
    bool wordWrap = false;
    int tabSize = 4;
    bool tabsToSpaces = true;
    bool autoIndent = true;
    bool autoPairs = true;
    std::string cursorStyle = "line";
    bool smoothScrolling = true;
};

// AI settings
struct AISettings {
    bool enabled = true;
    std::string defaultProvider = "anthropic";
    std::string defaultModel = "claude-3-sonnet";
    std::unordered_map<std::string, std::string> apiKeys;
    bool enableInlineCompletion = true;
    bool enableChat = true;
    int completionDelay = 300;
    float completionTemperature = 0.2f;
};

// Theme settings
struct ThemeSettings {
    std::string themeId = "dark";
    std::string accentColor = "#4A90D9";
    float uiScale = 1.0f;
    bool enableAnimations = true;
    bool enableShadows = true;
};

// Settings Manager
class SettingsManager {
public:
    SettingsManager();
    ~SettingsManager();

    // Initialization
    bool initialize(const std::string& configPath);
    void shutdown();
    bool save();
    bool load();

    // Value access
    void setValue(const std::string& key, const SettingsValue& value);
    SettingsValue getValue(const std::string& key, const SettingsValue& defaultValue = SettingsValue()) const;
    
    // Typed access
    bool getBool(const std::string& key, bool defaultValue = false) const;
    int getInt(const std::string& key, int defaultValue = 0) const;
    float getFloat(const std::string& key, float defaultValue = 0.0f) const;
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    
    void setBool(const std::string& key, bool value);
    void setInt(const std::string& key, int value);
    void setFloat(const std::string& key, float value);
    void setString(const std::string& key, const std::string& value);

    // Settings groups
    EditorSettings& getEditorSettings() { return editorSettings_; }
    AISettings& getAISettings() { return aiSettings_; }
    ThemeSettings& getThemeSettings() { return themeSettings_; }
    
    const EditorSettings& getEditorSettings() const { return editorSettings_; }
    const AISettings& getAISettings() const { return aiSettings_; }
    const ThemeSettings& getThemeSettings() const { return themeSettings_; }

    // Keybindings
    void addKeybinding(const Keybinding& binding);
    std::vector<Keybinding> getKeybindings() const { return keybindings_; }
    Keybinding getKeybinding(const std::string& key) const;

    // Events
    using SettingsChangedCallback = std::function<void(const std::string& key)>;
    void onSettingsChanged(SettingsChangedCallback callback);

    // Paths
    std::string getConfigPath() const { return configPath_; }
    std::string getDataPath() const { return dataPath_; }

private:
    std::string configPath_;
    std::string dataPath_;
    
    std::unordered_map<std::string, SettingsValue> values_;
    EditorSettings editorSettings_;
    AISettings aiSettings_;
    ThemeSettings themeSettings_;
    std::vector<Keybinding> keybindings_;
    
    mutable std::mutex mutex_;
    std::vector<SettingsChangedCallback> callbacks_;
    
    void notifyChanged(const std::string& key);
    std::string serialize() const;
    void deserialize(const std::string& json);
};

} // namespace NexusForge::Config
