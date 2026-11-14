// ui/widgets/terminal.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include <string>
#include <vector>
#include <deque>

namespace NexusForge::UI {

// Terminal line with style info
struct TerminalLine {
    std::string text;
    Color textColor = Color::White();
    Color backgroundColor = Color::Transparent();
    bool bold = false;
    bool dim = false;
    bool italic = false;
    bool underline = false;
    bool inverse = false;
};

// Terminal widget
class Terminal : public Widget {
public:
    using Ptr = std::shared_ptr<Terminal>;

    Terminal();
    ~Terminal() override;

    // Output
    void write(const std::string& text);
    void writeln(const std::string& text);
    void clear();
    
    // ANSI escape codes
    void setTextColor(const Color& color);
    void setBackgroundColor(const Color& color);
    void setBold(bool bold);
    void setDim(bool dim);
    void setItalic(bool italic);
    void setUnderline(bool underline);
    void setInverse(bool inverse);
    void resetStyles();
    
    // Cursor
    void moveCursor(int x, int y);
    void moveCursorUp(int lines);
    void moveCursorDown(int lines);
    void moveCursorLeft(int cols);
    void moveCursorRight(int cols);
    void saveCursor();
    void restoreCursor();
    
    // Scrollback
    void setScrollbackSize(size_t size) { scrollbackSize_ = size; }
    size_t getScrollbackSize() const { return scrollbackSize_; }
    void scrollToTop();
    void scrollToBottom();
    void scrollBy(int lines);
    
    // Input
    void setInputEnabled(bool enabled) { inputEnabled_ = enabled; }
    bool isInputEnabled() const { return inputEnabled_; }
    
    // Events
    std::function<void(const std::string&)> onCommand;
    std::function<void()> onClear;
    
    // Widget overrides
    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;
    bool onKeyDown(const Core::KeyPressEvent& event) override;
    bool onCharInput(const Core::CharInputEvent& event) override;
    bool onScroll(const Core::ScrollEvent& event) override;

private:
    std::deque<TerminalLine> lines_;
    std::string currentLine_;
    std::string inputBuffer_;
    
    int cursorX_ = 0;
    int cursorY_ = 0;
    int scrollOffset_ = 0;
    size_t scrollbackSize_ = 1000;
    
    Color currentTextColor_ = Color::White();
    Color currentBgColor_ = Color::Transparent();
    bool currentBold_ = false;
    bool currentDim_ = false;
    bool currentItalic_ = false;
    bool currentUnderline_ = false;
    bool currentInverse_ = false;
    
    struct CursorState {
        int x, y;
    };
    std::vector<CursorState> cursorStack_;
    
    bool inputEnabled_ = true;
    bool showCursor_ = true;
    double cursorBlinkTimer_ = 0;
    
    void processANSIEscape(const std::string& escape);
    void addLine(const TerminalLine& line);
    void ensureCursorVisible();
    int getVisibleLines() const;
};

} // namespace NexusForge::UI
