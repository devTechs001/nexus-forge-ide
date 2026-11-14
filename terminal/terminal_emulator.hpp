// terminal/terminal_emulator.hpp
#pragma once

#include "pty_handler.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <queue>
#include <cstdint>

namespace NexusForge::Terminal {

// RGBA color for terminal cells
struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    Color() = default;
    Color(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}
};

// Terminal cell (legacy struct preserved)
struct TerminalCell {
    char character = ' ';
    bool bold = false;
    bool dim = false;
    bool italic = false;
    bool underline = false;
    bool blink = false;
    bool reverse = false;
    bool hidden = false;
    bool strikethrough = false;
    uint8_t fgColor = 7;  // Default foreground
    uint8_t bgColor = 0;  // Default background
    bool isWideChar = false;  // Takes 2 columns (CJK)
};

// Terminal screen
struct TerminalScreen {
    size_t columns = 80;
    size_t rows = 24;
    std::vector<TerminalCell> cells;
    size_t cursorX = 0;
    size_t cursorY = 0;
    bool cursorVisible = true;
    bool cursorBlink = true;

    TerminalCell& at(size_t x, size_t y) { return cells[y * columns + x]; }
    const TerminalCell& at(size_t x, size_t y) const { return cells[y * columns + x]; }
    void resize(size_t newColumns, size_t newRows);
    void clear();
    void clearLine(size_t y);
    void clearRegion(size_t x1, size_t y1, size_t x2, size_t y2);
};

// Scrollback buffer
struct ScrollbackBuffer {
    std::vector<std::vector<TerminalCell>> lines;
    size_t maxLines = 10000;
    size_t scrollOffset = 0;

    void addLine(std::vector<TerminalCell> line);
    void clear();
    size_t totalLines() const { return lines.size(); }
    const std::vector<TerminalCell>& getLine(size_t index) const { return lines[index]; }
};

// ANSI escape sequence parser state
enum class ParserState {
    Ground,
    Escape,
    CsiEntry,
    CsiParam,
    CsiIntermediate,
    CsiFinal,
    OscString,
    SosPmApaString,
    DcsEntry,
    DcsParam,
    DcsIntermediate,
    DcsString,
    DcsFinal,
    Ignore
};

// Terminal modes
struct TerminalModes {
    bool insertMode = false;
    bool lineFeedMode = false;  // LNM
    bool columnMode = false;     // COLM
    bool originMode = false;     // DECOM
    bool wraparoundMode = true;  // DECAWM
    bool autoRepeatMode = true;  // ATOM
    bool interlaceMode = false;  // ANISIM
    bool reverseVideo = false;   // DECSCNM
    bool cursorKeysMode = false; // DECCKM
    bool keypadMode = false;     // DECNKM
    bool bracketedPaste = false;
    bool mouseMode = false;
    bool mouseSgrMode = false;
    bool alternateScroll = false;
};

// Color table (16 + 256 colors + RGB)
struct ColorTable {
    uint32_t colors[256];
    bool trueColor = true;

    ColorTable();
    uint32_t getColor(uint8_t index) const;
    void setColor(uint8_t index, uint32_t color);
};

// Terminal emulator
class TerminalEmulator {
public:
    struct Cell {
        char character = ' ';
        Color fgColor{0.9f, 0.9f, 0.9f, 1.0f};
        Color bgColor{0.1f, 0.1f, 0.12f, 1.0f};
        bool bold = false;
        bool underline = false;
        bool inverse = false;
    };

    TerminalEmulator();
    ~TerminalEmulator();

    // Initialization
    bool initialize(int cols = 80, int rows = 24);
    void shutdown();

    // Process spawning
    bool spawn(const std::string& shell,
               const std::vector<std::string>& args = {},
               const std::string& cwd = "",
               const std::unordered_map<std::string, std::string>& env = {});

    // Input / Output
    void write(const std::string& data);
    void write(const char* data, size_t len);
    void input(const std::string& data);
    void inputKey(int keyCode, int modifiers);
    void sendKey(int keyCode, int modifiers) { inputKey(keyCode, modifiers); }

    // Screen operations
    void resize(int cols, int rows);
    void clear();
    void scrollUp(int lines = 1);
    void scrollDown(int lines = 1);

    const std::vector<std::vector<Cell>>& getScreen() const;
    int getCursorX() const;
    int getCursorY() const;
    void setCursorVisible(bool visible);
    bool isCursorVisible() const;
    int getRows() const;
    int getCols() const;
    size_t getColumns() const { return static_cast<size_t>(cols_); }

    // Process state
    bool isProcessRunning() const;
    void kill();

    // Clipboard
    void copySelection();
    void paste();

    // Callbacks
    std::function<void()> onScreenChanged;
    using OutputCallback = std::function<void(const std::string&)>;
    using ScreenChangedCallback = std::function<void()>;
    using TitleCallback = std::function<void(const std::string&)>;
    void setScreenChangedCallback(ScreenChangedCallback callback) { onScreenChanged = std::move(callback); }

private:
    PTYHandler* pty_ = nullptr;
    int rows_ = 24;
    int cols_ = 80;
    int cursorX_ = 0;
    int cursorY_ = 0;
    int savedCursorX_ = 0;
    int savedCursorY_ = 0;
    bool cursorVisible_ = true;

    int scrollTop_ = 0;
    int scrollBottom_ = 23;

    std::vector<std::vector<Cell>> screen_;
    Color defaultFg_;
    Color defaultBg_;
    Color currentFg_;
    Color currentBg_;
    std::vector<Color> ansiColors_;
    bool currentBold_ = false;
    bool currentUnderline_ = false;
    bool currentInverse_ = false;

    bool inEscapeSequence_ = false;
    std::string escapeSequence_;

    void processChar(char c);
    void processEscapeSequence(char c);
    void executeCSI(const std::string& params, char command);
    void executeEscape(const std::string& seq);
    std::vector<int> parseParams(const std::string& params);
    void eraseDisplay(int mode);
    void eraseLine(int mode);
    void setGraphicsMode(const std::vector<int>& params);
    std::string getKeyPressSequence(int keyCode, int modifiers);
};

// Factory function
std::unique_ptr<PTYHandler> createPTYHandler();

} // namespace NexusForge::Terminal
