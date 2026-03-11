// config/keybinding_manager.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>

namespace NexusForge::Config {

// Key codes
enum class KeyCode {
    Unknown = 0,
    // Letters
    A = 65, B = 66, C = 67, D = 68, E = 69, F = 70, G = 71, H = 72,
    I = 73, J = 74, K = 75, L = 76, M = 77, N = 78, O = 79, P = 80,
    Q = 81, R = 82, S = 83, T = 84, U = 85, V = 86, W = 87, X = 88,
    Y = 89, Z = 90,
    // Numbers
    Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    // Function keys
    F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    // Special keys
    Escape = 256, Enter, Tab, Backspace, Insert, Delete,
    Right, Left, Down, Up,
    Home, End, PageUp, PageDown,
    Space = 32,
    // Modifiers
    LeftShift = 340, RightShift, LeftControl, RightControl,
    LeftAlt, RightAlt, LeftSuper, RightSuper,
    // Punctuation
    Semicolon = 59, Equal = 61, Comma = 44, Minus = 45,
    Period = 46, Slash = 47, Backslash = 92,
    BracketLeft = 91, BracketRight = 93,
    Quote = 39, Backquote = 96,
    // Numpad
    Numpad0 = 320, Numpad1, Numpad2, Numpad3, Numpad4,
    Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
    NumpadDecimal, NumpadDivide, NumpadMultiply,
    NumpadSubtract, NumpadAdd, NumpadEnter
};

// Modifier flags
enum class Modifier : uint16_t {
    None = 0,
    Shift = 0x0001,
    Control = 0x0002,
    Alt = 0x0004,
    Super = 0x0008,
    Cmd = Control,  // On macOS, Cmd is Control
    Option = Alt    // On macOS, Option is Alt
};

Modifier operator|(Modifier a, Modifier b);
Modifier operator&(Modifier a, Modifier b);

// Keybinding
struct Keybinding {
    std::string id;
    std::string command;
    KeyCode key;
    Modifier modifiers = Modifier::None;
    std::string when;  // Context condition
    std::string platform;  // Platform-specific (optional)

    std::string toString() const;
    static Keybinding fromString(const std::string& str);

    bool matches(KeyCode key, Modifier modifiers) const;
    bool matchesPlatform() const;
};

// Keybinding resolver
class KeybindingManager {
public:
    KeybindingManager();
    ~KeybindingManager();

    // Load/Save
    bool loadDefaultKeybindings();
    bool loadUserKeybindings(const std::string& path = "");
    bool saveUserKeybindings(const std::string& path = "");

    // Registration
    void addKeybinding(const Keybinding& binding);
    void removeKeybinding(const std::string& id);
    void updateKeybinding(const std::string& id, const Keybinding& binding);

    // Lookup
    std::optional<Keybinding> findKeybinding(KeyCode key, Modifier modifiers,
                                              const std::string& context = "") const;
    std::vector<Keybinding> getKeybindingsForCommand(const std::string& command) const;
    std::string getKeybindingString(const std::string& command) const;

    // Command handling
    using CommandHandler = std::function<void()>;
    void registerCommand(const std::string& commandId, CommandHandler handler);
    void executeCommand(const std::string& commandId);
    bool handleKey(KeyCode key, Modifier modifiers, const std::string& context = "");

    // Context
    void setContext(const std::string& context);
    void clearContext();

    // Export/Import
    std::string exportKeybindings() const;
    bool importKeybindings(const std::string& json);

    // Default keybindings
    static std::vector<Keybinding> getDefaultKeybindings();

private:
    std::vector<Keybinding> keybindings_;
    std::unordered_map<std::string, CommandHandler> commands_;
    std::string currentContext_;

    bool evaluateWhenClause(const std::string& when, const std::string& context) const;
};

// Keybinding parser
class KeybindingParser {
public:
    static KeyCode parseKey(const std::string& key);
    static Modifier parseModifier(const std::string& mod);
    static std::pair<KeyCode, Modifier> parse(const std::string& shortcut);
    static std::string toString(KeyCode key, Modifier modifiers);
};

} // namespace NexusForge::Config
