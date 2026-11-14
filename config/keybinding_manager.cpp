// config/keybinding_manager.cpp
// Keybinding Manager Implementation

#include "keybinding_manager.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace NexusForge::Config {

KeybindingManager::KeybindingManager() = default;
KeybindingManager::~KeybindingManager() = default;

bool KeybindingManager::loadDefaultKeybindings() {
    auto defaults = getDefaultKeybindings();
    keybindings_ = defaults;
    return true;
}

bool KeybindingManager::loadUserKeybindings(const std::string& path) {
    return true;
}

bool KeybindingManager::saveUserKeybindings(const std::string& path) {
    return true;
}

void KeybindingManager::addKeybinding(const Keybinding& binding) {
    keybindings_.push_back(binding);
}

void KeybindingManager::removeKeybinding(const std::string& id) {
    keybindings_.erase(std::remove_if(keybindings_.begin(), keybindings_.end(),
        [&](const Keybinding& kb) { return kb.id == id; }), keybindings_.end());
}

void KeybindingManager::updateKeybinding(const std::string& id, const Keybinding& binding) {
    for (auto& kb : keybindings_) {
        if (kb.id == id) {
            kb = binding;
            return;
        }
    }
}

std::optional<Keybinding> KeybindingManager::findKeybinding(KeyCode key, Modifier modifiers,
                                                             const std::string& context) const {
    for (const auto& kb : keybindings_) {
        if (kb.key == key && kb.modifiers == modifiers) {
            if (context.empty() || evaluateWhenClause(kb.when, context)) {
                return kb;
            }
        }
    }
    return std::nullopt;
}

std::vector<Keybinding> KeybindingManager::getKeybindingsForCommand(const std::string& command) const {
    std::vector<Keybinding> result;
    for (const auto& kb : keybindings_) {
        if (kb.command == command) result.push_back(kb);
    }
    return result;
}

std::string KeybindingManager::getKeybindingString(const std::string& command) const {
    for (const auto& kb : keybindings_) {
        if (kb.command == command) return kb.toString();
    }
    return "";
}

void KeybindingManager::registerCommand(const std::string& commandId, CommandHandler handler) {
    commands_[commandId] = std::move(handler);
}

void KeybindingManager::executeCommand(const std::string& commandId) {
    auto it = commands_.find(commandId);
    if (it != commands_.end()) it->second();
}

bool KeybindingManager::handleKey(KeyCode key, Modifier modifiers, const std::string& context) {
    auto binding = findKeybinding(key, modifiers, context);
    if (binding) {
        executeCommand(binding->command);
        return true;
    }
    return false;
}

void KeybindingManager::setContext(const std::string& context) {
    currentContext_ = context;
}

void KeybindingManager::clearContext() {
    currentContext_.clear();
}

std::string KeybindingManager::exportKeybindings() const {
    return "";
}

bool KeybindingManager::importKeybindings(const std::string& json) {
    return true;
}

std::vector<Keybinding> KeybindingManager::getDefaultKeybindings() {
    std::vector<Keybinding> defaults;
    return defaults;
}

bool KeybindingManager::evaluateWhenClause(const std::string& when, const std::string& context) const {
    return when.empty() || when == context;
}

} // namespace NexusForge::Config