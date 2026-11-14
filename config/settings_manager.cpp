// config/settings_manager.cpp
#include "settings_manager.hpp"
#include <fstream>
#include <sstream>

namespace NexusForge::Config {

SettingsManager::SettingsManager() = default;

SettingsManager::~SettingsManager() {
    shutdown();
}

bool SettingsManager::initialize(const std::string& configPath) {
    configPath_ = configPath;
    dataPath_ = configPath + "/data";
    return load();
}

void SettingsManager::shutdown() {
    save();
}

bool SettingsManager::save() {
    std::ofstream file(configPath_ + "/settings.json");
    if (!file.is_open()) return false;
    file << serialize();
    return true;
}

bool SettingsManager::load() {
    std::ifstream file(configPath_ + "/settings.json");
    if (!file.is_open()) return false;
    std::stringstream buffer;
    buffer << file.rdbuf();
    deserialize(buffer.str());
    return true;
}

void SettingsManager::setValue(const std::string& key, const SettingsValue& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    values_[key] = value;
    notifyChanged(key);
}

SettingsValue SettingsManager::getValue(const std::string& key, const SettingsValue& defaultValue) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = values_.find(key);
    if (it != values_.end()) {
        return it->second;
    }
    return defaultValue;
}

bool SettingsManager::getBool(const std::string& key, bool defaultValue) const {
    auto value = getValue(key, SettingsValue::fromBool(defaultValue));
    return value.type == SettingsValue::Type::Bool ? value.boolValue : defaultValue;
}

int SettingsManager::getInt(const std::string& key, int defaultValue) const {
    auto value = getValue(key, SettingsValue::fromInt(defaultValue));
    return value.type == SettingsValue::Type::Int ? value.intValue : defaultValue;
}

float SettingsManager::getFloat(const std::string& key, float defaultValue) const {
    auto value = getValue(key, SettingsValue::fromFloat(defaultValue));
    return value.type == SettingsValue::Type::Float ? value.floatValue : defaultValue;
}

std::string SettingsManager::getString(const std::string& key, const std::string& defaultValue) const {
    auto value = getValue(key, SettingsValue::fromString(defaultValue));
    return value.type == SettingsValue::Type::String ? value.stringValue : defaultValue;
}

void SettingsManager::setBool(const std::string& key, bool value) {
    setValue(key, SettingsValue::fromBool(value));
}

void SettingsManager::setInt(const std::string& key, int value) {
    setValue(key, SettingsValue::fromInt(value));
}

void SettingsManager::setFloat(const std::string& key, float value) {
    setValue(key, SettingsValue::fromFloat(value));
}

void SettingsManager::setString(const std::string& key, const std::string& value) {
    setValue(key, SettingsValue::fromString(value));
}

void SettingsManager::addKeybinding(const Keybinding& binding) {
    keybindings_.push_back(binding);
}

Keybinding SettingsManager::getKeybinding(const std::string& key) const {
    for (const auto& binding : keybindings_) {
        if (binding.key == key) {
            return binding;
        }
    }
    return Keybinding{};
}

void SettingsManager::onSettingsChanged(SettingsChangedCallback callback) {
    callbacks_.push_back(std::move(callback));
}

void SettingsManager::notifyChanged(const std::string& key) {
    for (auto& cb : callbacks_) {
        cb(key);
    }
}

std::string SettingsManager::serialize() const {
    // Simple JSON-like serialization (placeholder)
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"theme\": \"" << themeSettings_.themeId << "\",\n";
    ss << "  \"fontSize\": " << editorSettings_.fontSize << ",\n";
    ss << "  \"aiEnabled\": " << (aiSettings_.enabled ? "true" : "false") << "\n";
    ss << "}\n";
    return ss.str();
}

void SettingsManager::deserialize(const std::string& json) {
    // Simple JSON parsing (placeholder)
    // Would use a proper JSON library
}

} // namespace NexusForge::Config
