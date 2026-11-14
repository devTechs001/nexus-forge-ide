// config/keybinding_manager.cpp
// Keybinding Manager Implementation

#include "keybinding_manager.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace NexusForge::Config {

Modifier operator|(Modifier a, Modifier b) {
    return static_cast<Modifier>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}

Modifier operator&(Modifier a, Modifier b) {
    return static_cast<Modifier>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}

std::string Keybinding::toString() const {
    if (key == KeyCode::Unknown && !keyString.empty()) {
        return keyString;
    }
    return KeybindingParser::toString(key, modifiers);
}

Keybinding Keybinding::fromString(const std::string& str) {
    Keybinding kb;
    kb.keyString = str;
    auto [parsedKey, parsedMods] = KeybindingParser::parse(str);
    kb.key = parsedKey;
    kb.modifiers = parsedMods;
    return kb;
}

bool Keybinding::matches(KeyCode k, Modifier mods) const {
    return key == k && modifiers == mods && matchesPlatform();
}

bool Keybinding::matchesPlatform() const {
    if (platform.empty() || platform == "all") return true;
#if defined(_WIN32) || defined(_WIN64)
    return platform == "windows" || platform == "win32" || platform == "win";
#elif defined(__APPLE__)
    return platform == "macos" || platform == "darwin" || platform == "mac";
#elif defined(__ANDROID__)
    return platform == "android";
#elif defined(__linux__)
    return platform == "linux";
#else
    return true;
#endif
}

KeyCode KeybindingParser::parseKey(const std::string& key) {
    std::string lower = key;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lower.size() == 1) {
        char c = lower[0];
        if (c >= 'a' && c <= 'z') {
            return static_cast<KeyCode>(static_cast<int>(KeyCode::A) + (c - 'a'));
        }
        if (c >= '0' && c <= '9') {
            return static_cast<KeyCode>(static_cast<int>(KeyCode::Num0) + (c - '0'));
        }
        switch (c) {
            case ' ': return KeyCode::Space;
            case ';': return KeyCode::Semicolon;
            case '=': return KeyCode::Equal;
            case ',': return KeyCode::Comma;
            case '-': return KeyCode::Minus;
            case '.': return KeyCode::Period;
            case '/': return KeyCode::Slash;
            case '\\': return KeyCode::Backslash;
            case '[': return KeyCode::BracketLeft;
            case ']': return KeyCode::BracketRight;
            case '\'': return KeyCode::Quote;
            case '`': return KeyCode::Backquote;
            default: break;
        }
    }
    if (lower == "escape" || lower == "esc") return KeyCode::Escape;
    if (lower == "enter" || lower == "return") return KeyCode::Enter;
    if (lower == "tab") return KeyCode::Tab;
    if (lower == "backspace") return KeyCode::Backspace;
    if (lower == "insert") return KeyCode::Insert;
    if (lower == "delete" || lower == "del") return KeyCode::Delete;
    if (lower == "right") return KeyCode::Right;
    if (lower == "left") return KeyCode::Left;
    if (lower == "down") return KeyCode::Down;
    if (lower == "up") return KeyCode::Up;
    if (lower == "home") return KeyCode::Home;
    if (lower == "end") return KeyCode::End;
    if (lower == "pageup") return KeyCode::PageUp;
    if (lower == "pagedown") return KeyCode::PageDown;
    if (lower == "space") return KeyCode::Space;
    if (lower.size() >= 2 && lower[0] == 'f') {
        try {
            int fnum = std::stoi(lower.substr(1));
            if (fnum >= 1 && fnum <= 12) {
                return static_cast<KeyCode>(static_cast<int>(KeyCode::F1) + (fnum - 1));
            }
        } catch (...) {}
    }
    return KeyCode::Unknown;
}

Modifier KeybindingParser::parseModifier(const std::string& mod) {
    std::string lower = mod;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lower == "ctrl" || lower == "control") return Modifier::Control;
    if (lower == "shift") return Modifier::Shift;
    if (lower == "alt" || lower == "option") return Modifier::Alt;
    if (lower == "super" || lower == "meta" || lower == "win") return Modifier::Super;
    if (lower == "cmd" || lower == "command") return Modifier::Cmd;
    return Modifier::None;
}

std::pair<KeyCode, Modifier> KeybindingParser::parse(const std::string& shortcut) {
    KeyCode key = KeyCode::Unknown;
    Modifier mods = Modifier::None;
    std::stringstream ss(shortcut);
    std::string token;
    while (std::getline(ss, token, '+')) {
        size_t first = token.find_first_not_of(" \t");
        if (first == std::string::npos) continue;
        size_t last = token.find_last_not_of(" \t");
        token = token.substr(first, last - first + 1);
        Modifier m = parseModifier(token);
        if (m != Modifier::None) {
            mods = mods | m;
        } else {
            key = parseKey(token);
        }
    }
    return {key, mods};
}

std::string KeybindingParser::toString(KeyCode key, Modifier modifiers) {
    std::string result;
    if ((modifiers & Modifier::Control) != Modifier::None) result += "Ctrl+";
    if ((modifiers & Modifier::Shift) != Modifier::None) result += "Shift+";
    if ((modifiers & Modifier::Alt) != Modifier::None) result += "Alt+";
    if ((modifiers & Modifier::Super) != Modifier::None) result += "Super+";

    int code = static_cast<int>(key);
    if (code >= static_cast<int>(KeyCode::A) && code <= static_cast<int>(KeyCode::Z)) {
        result += static_cast<char>('A' + (code - static_cast<int>(KeyCode::A)));
    } else if (code >= static_cast<int>(KeyCode::Num0) && code <= static_cast<int>(KeyCode::Num9)) {
        result += static_cast<char>('0' + (code - static_cast<int>(KeyCode::Num0)));
    } else if (code >= static_cast<int>(KeyCode::F1) && code <= static_cast<int>(KeyCode::F12)) {
        result += "F" + std::to_string(1 + (code - static_cast<int>(KeyCode::F1)));
    } else {
        switch (key) {
            case KeyCode::Escape: result += "Escape"; break;
            case KeyCode::Enter: result += "Enter"; break;
            case KeyCode::Tab: result += "Tab"; break;
            case KeyCode::Backspace: result += "Backspace"; break;
            case KeyCode::Insert: result += "Insert"; break;
            case KeyCode::Delete: result += "Delete"; break;
            case KeyCode::Right: result += "Right"; break;
            case KeyCode::Left: result += "Left"; break;
            case KeyCode::Down: result += "Down"; break;
            case KeyCode::Up: result += "Up"; break;
            case KeyCode::Home: result += "Home"; break;
            case KeyCode::End: result += "End"; break;
            case KeyCode::PageUp: result += "PageUp"; break;
            case KeyCode::PageDown: result += "PageDown"; break;
            case KeyCode::Space: result += "Space"; break;
            case KeyCode::Semicolon: result += ";"; break;
            case KeyCode::Equal: result += "="; break;
            case KeyCode::Comma: result += ","; break;
            case KeyCode::Minus: result += "-"; break;
            case KeyCode::Period: result += "."; break;
            case KeyCode::Slash: result += "/"; break;
            case KeyCode::Backslash: result += "\\"; break;
            case KeyCode::BracketLeft: result += "["; break;
            case KeyCode::BracketRight: result += "]"; break;
            case KeyCode::Quote: result += "'"; break;
            case KeyCode::Backquote: result += "`"; break;
            default:
                if (!result.empty() && result.back() == '+') result.pop_back();
                break;
        }
    }
    return result;
}

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