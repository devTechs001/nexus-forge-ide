// terminal/terminal_emulator.hpp
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <queue>

namespace NexusForge::Terminal {

// Terminal cell
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
    TerminalEmulator();
    ~TerminalEmulator();

    // Initialization
    bool initialize(size_t columns = 80, size_t rows = 24);
    void shutdown();

    // Screen access
    const TerminalScreen& getScreen() const { return screen_; }
    const ScrollbackBuffer& getScrollback() const { return scrollback_; }
    size_t getColumns() const { return screen_.columns; }
    size_t getRows() const { return screen_.rows; }

    // Input
    void write(const std::string& data);
    void write(const char* data, size_t length);
    void input(const std::string& text);
    void sendKey(int keyCode, int modifiers);

    // Output callbacks
    using OutputCallback = std::function<void(const std::string&)>;
    void setOutputCallback(OutputCallback callback) { outputCallback_ = callback; }

    // Screen changed callback
    using ScreenChangedCallback = std::function<void()>;
    void setScreenChangedCallback(ScreenChangedCallback callback) { screenChanged_ = callback; }

    // Title callback
    using TitleCallback = std::function<void(const std::string&)>;
    void setTitleCallback(TitleCallback callback) { titleCallback_ = callback; }

    // Cursor
    void showCursor();
    void hideCursor();
    void moveCursor(size_t x, size_t y);
    void moveCursorRelative(int dx, int dy);

    // Scrolling
    void scrollUp(size_t lines = 1);
    void scrollDown(size_t lines = 1);
    void setScrollOffset(size_t offset);
    size_t getScrollOffset() const { return scrollback_.scrollOffset; }
    void scrollToTop();
    void scrollToBottom();

    // Selection
    void selectAll();
    void clearSelection();
    std::string getSelectedText() const;
    bool hasSelection() const { return hasSelection_; }

    // Search
    bool search(const std::string& pattern, bool forward = true, bool caseSensitive = false);
    void clearSearch();

    // Copy/Paste
    std::string getClipboardText() const;
    void setClipboardText(const std::string& text);

    // Reset
    void reset();
    void softReset();
    void clear();
    void clearHistory();

    // Modes
    const TerminalModes& getModes() const { return modes_; }
    void setMode(size_t mode, bool value);
    bool getMode(size_t mode) const;

    // Colors
    const ColorTable& getColors() const { return colors_; }
    void setColor(size_t index, uint32_t color);

    // PTY interaction
    void setPtyDataCallback(OutputCallback callback) { ptyDataCallback_ = callback; }
    void onPtyData(const std::string& data);

private:
    TerminalScreen screen_;
    ScrollbackBuffer scrollback_;
    TerminalModes modes_;
    ColorTable colors_;

    ParserState parserState_ = ParserState::Ground;
    std::string escapeBuffer_;
    std::vector<int> csiParams_;

    // Current style
    uint8_t currentFgColor = 7;
    uint8_t currentBgColor = 0;
    bool currentBold = false;
    bool currentDim = false;
    bool currentItalic = false;
    bool currentUnderline = false;
    bool currentBlink = false;
    bool currentReverse = false;
    bool currentHidden = false;
    bool currentStrikethrough = false;

    // Margins
    size_t scrollTop_ = 0;
    size_t scrollBottom_ = 0;

    // Saved state (for DECSC/DECRC)
    struct SavedState {
        size_t cursorX;
        size_t cursorY;
        uint8_t fgColor;
        uint8_t bgColor;
        bool bold, dim, italic, underline, blink, reverse, hidden, strikethrough;
        TerminalModes modes;
    };
    SavedState savedState_{};

    // Callbacks
    OutputCallback outputCallback_;
    OutputCallback ptyDataCallback_;
    ScreenChangedCallback screenChanged_;
    TitleCallback titleCallback_;

    // Selection
    bool hasSelection_ = false;
    size_t selectionStartX_ = 0;
    size_t selectionStartY_ = 0;
    size_t selectionEndX_ = 0;
    size_t selectionEndY_ = 0;

    // Parser
    void parse(const char* data, size_t length);
    void processChar(char c);
    void executeEscape(const std::string& seq);
    void processCsi(char finalChar);
    void processOsc(const std::string& params);

    // CSI handlers
    void csiCursorUp(int n);
    void csiCursorDown(int n);
    void csiCursorForward(int n);
    void csiCursorBack(int n);
    void csiCursorNextLine(int n);
    void csiCursorPrevLine(int n);
    void csiCursorColumn(int n);
    void csiCursorPosition(int line, int column);
    void csiEraseDisplay(int n);
    void csiEraseLine(int n);
    void csiScrollUp(int n);
    void csiScrollDown(int n);
    void csiCursorPositionReport();
    void csiSetMode(int n);
    void csiResetMode(int n);
    void csiSetGraphicsRendition(const std::vector<int>& params);
    void csiDeviceStatusReport(int n);

    // Helpers
    void updateCell();
    void lineFeed();
    void carriageReturn();
    void tab();
    void backspace();
    void deleteChar();
    void insertBlank(int n);
    void saveCursor();
    void restoreCursor();
    void setScrollRegion(size_t top, size_t bottom);
    void applyScrollRegion();
};

// PTY Handler interface
class PTYHandler {
public:
    virtual ~PTYHandler() = default;
    virtual bool initialize(const std::string& shell, const std::string& cwd,
                            const std::unordered_map<std::string, std::string>& env) = 0;
    virtual void shutdown() = 0;
    virtual void write(const std::string& data) = 0;
    virtual void resize(size_t columns, size_t rows) = 0;
    virtual bool isRunning() const = 0;
    virtual int getPid() const = 0;

    using DataCallback = std::function<void(const std::string&)>;
    using ExitCallback = std::function<void(int)>;

    virtual void setDataCallback(DataCallback callback) = 0;
    virtual void setExitCallback(ExitCallback callback) = 0;
};

// Factory function
std::unique_ptr<PTYHandler> createPTYHandler();

} // namespace NexusForge::Terminal
