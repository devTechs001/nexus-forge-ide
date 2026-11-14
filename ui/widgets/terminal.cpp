// ui/widgets/terminal.cpp
#include "terminal.hpp"
#include <algorithm>
#include <sstream>

namespace NexusForge::UI {

Terminal::Terminal() {
    style_.backgroundColor = GradientColor::fromHex(0x1A1A2E);
    style_.foregroundColor = GradientColor::fromHex(0xE8E8F0);
    style_.fontFamily = "JetBrains Mono";
    style_.fontSize = 14;
    style_.padding = EdgeInsets(8);
}

Terminal::~Terminal() = default;

void Terminal::write(const std::string& text) {
    for (char c : text) {
        if (c == '\n') {
            TerminalLine line;
            line.text = currentLine_;
            line.textColor = currentTextColor_;
            line.backgroundColor = currentBgColor_;
            line.bold = currentBold_;
            line.dim = currentDim_;
            line.italic = currentItalic_;
            line.underline = currentUnderline_;
            line.inverse = currentInverse_;
            addLine(line);
            currentLine_.clear();
            cursorX_ = 0;
            cursorY_++;
        } else if (c == '\r') {
            cursorX_ = 0;
        } else if (c == '\t') {
            currentLine_ += "    ";
            cursorX_ += 4;
        } else {
            currentLine_ += c;
            cursorX_++;
        }
    }
    ensureCursorVisible();
    markDirty();
}

void Terminal::writeln(const std::string& text) {
    write(text + "\n");
}

void Terminal::clear() {
    lines_.clear();
    currentLine_.clear();
    cursorX_ = 0;
    cursorY_ = 0;
    scrollOffset_ = 0;
    markDirty();
}

void Terminal::setTextColor(const Color& color) {
    currentTextColor_ = color;
}

void Terminal::setBackgroundColor(const Color& color) {
    currentBgColor_ = color;
}

void Terminal::setBold(bool bold) {
    currentBold_ = bold;
}

void Terminal::setDim(bool dim) {
    currentDim_ = dim;
}

void Terminal::setItalic(bool italic) {
    currentItalic_ = italic;
}

void Terminal::setUnderline(bool underline) {
    currentUnderline_ = underline;
}

void Terminal::setInverse(bool inverse) {
    currentInverse_ = inverse;
}

void Terminal::resetStyles() {
    currentTextColor_ = Color::White();
    currentBgColor_ = Color::Transparent();
    currentBold_ = false;
    currentDim_ = false;
    currentItalic_ = false;
    currentUnderline_ = false;
    currentInverse_ = false;
}

void Terminal::moveCursor(int x, int y) {
    cursorX_ = std::max(0, x);
    cursorY_ = std::max(0, y);
}

void Terminal::moveCursorUp(int lines) {
    cursorY_ = std::max(0, cursorY_ - lines);
}

void Terminal::moveCursorDown(int lines) {
    cursorY_ += lines;
}

void Terminal::moveCursorLeft(int cols) {
    cursorX_ = std::max(0, cursorX_ - cols);
}

void Terminal::moveCursorRight(int cols) {
    cursorX_ += cols;
}

void Terminal::saveCursor() {
    cursorStack_.push_back({cursorX_, cursorY_});
}

void Terminal::restoreCursor() {
    if (!cursorStack_.empty()) {
        CursorState state = cursorStack_.back();
        cursorStack_.pop_back();
        cursorX_ = state.x;
        cursorY_ = state.y;
    }
}

void Terminal::scrollToTop() {
    scrollOffset_ = static_cast<int>(lines_.size()) - getVisibleLines();
}

void Terminal::scrollToBottom() {
    scrollOffset_ = 0;
}

void Terminal::scrollBy(int lines) {
    scrollOffset_ += lines;
    scrollOffset_ = std::max(0, scrollOffset_);
    scrollOffset_ = std::min(scrollOffset_, static_cast<int>(lines_.size()) - getVisibleLines());
}

void Terminal::render(RenderEngine& renderer) {
    if (!isVisibleInHierarchy()) return;

    float lineHeight = style_.fontSize + 4;
    float x = bounds_.x + style_.padding.left;
    float y = bounds_.y + style_.padding.top;
    
    // Draw background
    renderer.drawRect(bounds_, Color(0.1f, 0.1f, 0.18f, 1.0f));
    
    // Draw lines
    int startLine = std::max(0, static_cast<int>(lines_.size()) - getVisibleLines() - scrollOffset_);
    int endLine = std::min(static_cast<int>(lines_.size()), startLine + getVisibleLines() + 1);
    
    for (int i = startLine; i < endLine && i < static_cast<int>(lines_.size()); ++i) {
        const TerminalLine& line = lines_[i];
        float lineY = y + (i - startLine) * lineHeight;
        
        renderer.drawText(line.text, x, lineY + style_.fontSize, 
                          Color(line.textColor.r, line.textColor.g, line.textColor.b, 1.0f),
                          style_.fontFamily, style_.fontSize);
    }
    
    // Draw current line
    if (!currentLine_.empty()) {
        float lineY = y + (endLine - startLine) * lineHeight;
        renderer.drawText(currentLine_, x, lineY + style_.fontSize,
                          Color(currentTextColor_.r, currentTextColor_.g, currentTextColor_.b, 1.0f),
                          style_.fontFamily, style_.fontSize);
    }
    
    // Draw cursor
    if (showCursor_ && hasFocus()) {
        float cursorX = x + cursorX_ * style_.fontSize * 0.5f;
        float cursorY = y + (std::max(0, static_cast<int>(lines_.size()) - getVisibleLines() - scrollOffset_) 
                            + cursorY_ - startLine) * lineHeight;
        renderer.drawRect(Rect(cursorX, cursorY, 2, lineHeight - 2), Color::White());
    }
}

Vec2 Terminal::measure(float availableWidth, float availableHeight) {
    return Vec2(availableWidth, std::min(300.0f, availableHeight));
}

bool Terminal::onKeyDown(const Core::KeyPressEvent& event) {
    if (!inputEnabled_) return false;
    
    if (event.keyCode == 13) { // Enter
        if (!inputBuffer_.empty()) {
            if (onCommand) onCommand(inputBuffer_);
        }
        writeln("$ " + inputBuffer_);
        inputBuffer_.clear();
        return true;
    }
    
    if (event.keyCode == 8) { // Backspace
        if (!inputBuffer_.empty()) {
            inputBuffer_.pop_back();
        }
        return true;
    }
    
    return false;
}

bool Terminal::onCharInput(const Core::CharInputEvent& event) {
    if (!inputEnabled_) return false;
    
    char c = static_cast<char>(event.codepoint);
    if (c >= 32 && c != 127) {
        inputBuffer_ += c;
    }
    
    return true;
}

bool Terminal::onScroll(const Core::ScrollEvent& event) {
    int delta = event.yOffset > 0 ? -3 : 3;
    scrollBy(delta);
    return true;
}

void Terminal::addLine(const TerminalLine& line) {
    lines_.push_back(line);
    while (lines_.size() > scrollbackSize_) {
        lines_.pop_front();
    }
}

void Terminal::ensureCursorVisible() {
    // Keep cursor in visible area
}

int Terminal::getVisibleLines() const {
    float lineHeight = style_.fontSize + 4;
    return static_cast<int>((bounds_.height - style_.padding.vertical()) / lineHeight);
}

} // namespace NexusForge::UI
