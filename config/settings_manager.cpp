// config/settings_manager.cpp
#include "settings_manager.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace NexusForge::Config {

SettingsManager::SettingsManager() = default;

SettingsManager::~SettingsManager() {
    shutdown();
}

bool SettingsManager::initialize(const std::string& configPath) {
    std::filesystem::path p(configPath);
    if (p.extension() == ".json") {
        configPath_ = p.has_parent_path() ? p.parent_path().string() : ".";
    } else {
        configPath_ = configPath;
    }
    dataPath_ = configPath_.empty() ? "data" : (configPath_ + "/data");
    std::error_code ec;
    if (!configPath_.empty()) {
        std::filesystem::create_directories(configPath_, ec);
    }
    if (!load()) {
        // First run: save default settings file rather than failing initialization
        save();
    }
    return true;
}

void SettingsManager::shutdown() {
    if (!configPath_.empty()) {
        save();
    }
}

bool SettingsManager::save() {
    if (configPath_.empty()) return false;
    std::ofstream file(configPath_ + "/settings.json");
    if (!file.is_open()) return false;
    file << serialize();
    return file.good();
}

bool SettingsManager::load() {
    if (configPath_.empty()) return false;
    std::ifstream file(configPath_ + "/settings.json");
    if (!file.is_open()) return false;
    std::stringstream buffer;
    buffer << file.rdbuf();
    deserialize(buffer.str());
    return true;
}

void SettingsManager::setValue(const std::string& key, const SettingsValue& value) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        values_[key] = value;
    }
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
    std::lock_guard<std::mutex> lock(mutex_);
    keybindings_.push_back(binding);
}

Keybinding SettingsManager::getKeybinding(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& binding : keybindings_) {
        if (binding.keyString == key || binding.toString() == key) {
            return binding;
        }
    }
    return Keybinding{};
}

void SettingsManager::onSettingsChanged(SettingsChangedCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_.push_back(std::move(callback));
}

void SettingsManager::notifyChanged(const std::string& key) {
    std::vector<SettingsChangedCallback> snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot = callbacks_;
    }
    for (auto& cb : snapshot) {
        if (cb) cb(key);
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
