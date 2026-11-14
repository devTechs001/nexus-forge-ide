// terminal/terminal_emulator.cpp
// Terminal Emulator Implementation

#include "terminal_emulator.hpp"
#include <algorithm>
#include <cstring>
#include <cctype>
#include <sstream>

namespace NexusForge::Terminal {

void TerminalScreen::resize(size_t newColumns, size_t newRows) {
    columns = std::max<size_t>(1, newColumns);
    rows = std::max<size_t>(1, newRows);
    cells.assign(columns * rows, TerminalCell{});
    cursorX = std::min(cursorX, columns - 1);
    cursorY = std::min(cursorY, rows - 1);
}

void TerminalScreen::clear() {
    cells.assign(columns * rows, TerminalCell{});
    cursorX = 0;
    cursorY = 0;
}

void TerminalScreen::clearLine(size_t y) {
    if (y >= rows) return;
    for (size_t x = 0; x < columns; ++x) {
        at(x, y) = TerminalCell{};
    }
}

void TerminalScreen::clearRegion(size_t x1, size_t y1, size_t x2, size_t y2) {
    for (size_t y = y1; y <= y2 && y < rows; ++y) {
        for (size_t x = x1; x <= x2 && x < columns; ++x) {
            at(x, y) = TerminalCell{};
        }
    }
}

void ScrollbackBuffer::addLine(std::vector<TerminalCell> line) {
    lines.push_back(std::move(line));
    if (lines.size() > maxLines) {
        lines.erase(lines.begin());
    }
}

void ScrollbackBuffer::clear() {
    lines.clear();
    scrollOffset = 0;
}

ColorTable::ColorTable() {
    for (int i = 0; i < 256; ++i) colors[i] = 0xFFFFFFFFu;
}

uint32_t ColorTable::getColor(uint8_t index) const { return colors[index]; }
void ColorTable::setColor(uint8_t index, uint32_t color) { colors[index] = color; }

TerminalEmulator::TerminalEmulator() : pty_(nullptr) {
    rows_ = 24;
    cols_ = 80;
    scrollTop_ = 0;
    scrollBottom_ = rows_ - 1;

    // Initialize screen buffer
    screen_.resize(rows_);
    for (auto& row : screen_) {
        row.resize(cols_, Cell{});
    }

    // Default colors
    defaultFg_ = Color(0.9f, 0.9f, 0.9f);
    defaultBg_ = Color(0.1f, 0.1f, 0.12f);

    // ANSI colors
    ansiColors_ = {
        Color(0, 0, 0),         // Black
        Color(0.8f, 0, 0),      // Red
        Color(0, 0.8f, 0),      // Green
        Color(0.8f, 0.8f, 0),   // Yellow
        Color(0, 0, 0.8f),      // Blue
        Color(0.8f, 0, 0.8f),   // Magenta
        Color(0, 0.8f, 0.8f),   // Cyan
        Color(0.8f, 0.8f, 0.8f),// White
        Color(0.3f, 0.3f, 0.3f),// Bright Black
        Color(1.0f, 0.3f, 0.3f),// Bright Red
        Color(0.3f, 1.0f, 0.3f),// Bright Green
        Color(1.0f, 1.0f, 0.3f),// Bright Yellow
        Color(0.3f, 0.3f, 1.0f),// Bright Blue
        Color(1.0f, 0.3f, 1.0f),// Bright Magenta
        Color(0.3f, 1.0f, 1.0f),// Bright Cyan
        Color(1.0f, 1.0f, 1.0f) // Bright White
    };

    currentFg_ = defaultFg_;
    currentBg_ = defaultBg_;
}

TerminalEmulator::~TerminalEmulator() {
    shutdown();
}

bool TerminalEmulator::initialize(int cols, int rows) {
    cols_ = std::clamp(cols, 1, 10000);
    rows_ = std::clamp(rows, 1, 10000);
    scrollTop_ = 0;
    scrollBottom_ = rows_ - 1;
    cursorX_ = 0;
    cursorY_ = 0;

    screen_.clear();
    screen_.resize(static_cast<size_t>(rows_));
    for (auto& row : screen_) {
        row.assign(static_cast<size_t>(cols_), Cell{});
    }

    return true;
}

void TerminalEmulator::shutdown() {
    if (pty_) {
        delete pty_;
        pty_ = nullptr;
    }
}

bool TerminalEmulator::spawn(const std::string& shell, 
                              const std::vector<std::string>& args,
                              const std::string& cwd,
                              const std::unordered_map<std::string, std::string>& env) {
    pty_ = new PTYHandler();
    
    if (!pty_->spawn(shell, args, cwd, env, cols_, rows_)) {
        delete pty_;
        pty_ = nullptr;
        return false;
    }
    
    // Connect output callback
    pty_->setOutputCallback([this](const std::string& data) {
        write(data);
    });
    
    return true;
}

void TerminalEmulator::write(const std::string& data) {
    for (char c : data) {
        processChar(c);
    }
    
    if (onScreenChanged) {
        onScreenChanged();
    }
}

void TerminalEmulator::write(const char* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        processChar(data[i]);
    }
    
    if (onScreenChanged) {
        onScreenChanged();
    }
}

void TerminalEmulator::input(const std::string& data) {
    if (pty_) {
        pty_->write(data);
    }
}

void TerminalEmulator::inputKey(int keyCode, int modifiers) {
    std::string sequence = getKeyPressSequence(keyCode, modifiers);
    if (!sequence.empty()) {
        input(sequence);
    }
}

void TerminalEmulator::resize(int cols, int rows) {
    cols_ = std::clamp(cols, 1, 10000);
    rows_ = std::clamp(rows, 1, 10000);

    // Resize screen buffer without fill-copying temporary rows
    std::vector<std::vector<Cell>> newScreen(static_cast<size_t>(rows_));
    for (int i = 0; i < rows_; ++i) {
        newScreen[i].assign(static_cast<size_t>(cols_), Cell{});
        if (i < static_cast<int>(screen_.size())) {
            int copyCols = std::min(cols_, static_cast<int>(screen_[i].size()));
            for (int j = 0; j < copyCols; ++j) {
                newScreen[i][j] = screen_[i][j];
            }
        }
    }

    screen_ = std::move(newScreen);
    scrollTop_ = 0;
    scrollBottom_ = rows_ - 1;
    cursorX_ = std::clamp(cursorX_, 0, cols_ - 1);
    cursorY_ = std::clamp(cursorY_, 0, rows_ - 1);
    savedCursorX_ = std::clamp(savedCursorX_, 0, cols_ - 1);
    savedCursorY_ = std::clamp(savedCursorY_, 0, rows_ - 1);

    if (pty_) {
        pty_->resize(cols_, rows_);
    }
}

void TerminalEmulator::clear() {
    for (auto& row : screen_) {
        for (auto& cell : row) {
            cell = Cell{};
        }
    }

    cursorX_ = 0;
    cursorY_ = 0;
    scrollTop_ = 0;
    scrollBottom_ = std::max(0, rows_ - 1);
}

void TerminalEmulator::scrollUp(int lines) {
    if (lines <= 0 || screen_.empty() || scrollTop_ < 0 ||
        scrollBottom_ >= static_cast<int>(screen_.size()) || scrollTop_ > scrollBottom_) {
        return;
    }
    for (int i = 0; i < lines; ++i) {
        screen_.erase(screen_.begin() + scrollTop_);
        std::vector<Cell> newRow(cols_, Cell{});
        screen_.insert(screen_.begin() + scrollBottom_, std::move(newRow));
    }
}

void TerminalEmulator::scrollDown(int lines) {
    if (lines <= 0 || screen_.empty() || scrollTop_ < 0 ||
        scrollBottom_ >= static_cast<int>(screen_.size()) || scrollTop_ > scrollBottom_) {
        return;
    }
    for (int i = 0; i < lines; ++i) {
        screen_.erase(screen_.begin() + scrollBottom_);
        std::vector<Cell> newRow(cols_, Cell{});
        screen_.insert(screen_.begin() + scrollTop_, std::move(newRow));
    }
}

void TerminalEmulator::processChar(char c) {
    if (inEscapeSequence_) {
        processEscapeSequence(c);
        return;
    }
    
    switch (c) {
        case '\n':
            cursorY_++;
            if (cursorY_ > scrollBottom_) {
                scrollUp(1);
                cursorY_ = scrollBottom_;
            }
            break;
            
        case '\r':
            cursorX_ = 0;
            break;
            
        case '\t':
            cursorX_ = (cursorX_ + 8) & ~7;
            if (cursorX_ >= cols_) cursorX_ = cols_ - 1;
            break;
            
        case '\b':
            if (cursorX_ > 0) cursorX_--;
            break;
            
        case '\a':
            // Bell - could play sound
            break;
            
        case 0x1b:  // ESC
            inEscapeSequence_ = true;
            escapeSequence_.clear();
            break;
            
        default:
            if (static_cast<unsigned char>(c) >= 32) {
                // Printable character
                if (cursorX_ >= 0 && cursorX_ < cols_ && cursorY_ >= 0 && cursorY_ < rows_) {
                    Cell& cell = screen_[cursorY_][cursorX_];
                    cell.character = c;
                    cell.fgColor = currentFg_;
                    cell.bgColor = currentBg_;
                    cell.bold = currentBold_;
                    cell.underline = currentUnderline_;
                    cell.inverse = currentInverse_;
                }
                cursorX_++;
                if (cursorX_ >= cols_) {
                    cursorX_ = 0;
                    cursorY_++;
                    if (cursorY_ > scrollBottom_) {
                        scrollUp(1);
                        cursorY_ = scrollBottom_;
                    }
                }
            }
            break;
    }
}

void TerminalEmulator::processEscapeSequence(char c) {
    escapeSequence_ += c;

    // Check for CSI (Control Sequence Introducer)
    if (escapeSequence_ == "[") {
        return;  // Waiting for more
    }

    if (escapeSequence_.size() > 1 && escapeSequence_[0] == '[') {
        // CSI sequence
        if (std::isalpha(static_cast<unsigned char>(c))) {
            // End of sequence
            executeCSI(escapeSequence_.substr(1, escapeSequence_.size() - 2), c);
            inEscapeSequence_ = false;
            escapeSequence_.clear();
        } else if (escapeSequence_.size() > 16) {
            // Too long, abort
            inEscapeSequence_ = false;
            escapeSequence_.clear();
        }
    } else if (escapeSequence_.size() > 1) {
        // Other escape sequences
        executeEscape(escapeSequence_);
        inEscapeSequence_ = false;
        escapeSequence_.clear();
    }
}

void TerminalEmulator::executeCSI(const std::string& params, char command) {
    std::vector<int> args = parseParams(params);
    
    switch (command) {
        case 'A':  // Cursor Up
            cursorY_ -= std::max(1, args.empty() ? 1 : args[0]);
            cursorY_ = std::max(0, cursorY_);
            break;
            
        case 'B':  // Cursor Down
            cursorY_ += std::max(1, args.empty() ? 1 : args[0]);
            cursorY_ = std::min(rows_ - 1, cursorY_);
            break;
            
        case 'C':  // Cursor Forward
            cursorX_ += std::max(1, args.empty() ? 1 : args[0]);
            cursorX_ = std::min(cols_ - 1, cursorX_);
            break;
            
        case 'D':  // Cursor Back
            cursorX_ -= std::max(1, args.empty() ? 1 : args[0]);
            cursorX_ = std::max(0, cursorX_);
            break;
            
        case 'E':  // Cursor Next Line
            cursorY_ += std::max(1, args.empty() ? 1 : args[0]);
            cursorY_ = std::min(rows_ - 1, cursorY_);
            cursorX_ = 0;
            break;
            
        case 'F':  // Cursor Previous Line
            cursorY_ -= std::max(1, args.empty() ? 1 : args[0]);
            cursorY_ = std::max(0, cursorY_);
            cursorX_ = 0;
            break;
            
        case 'G':  // Cursor Character Absolute
            cursorX_ = std::max(0, std::min(cols_ - 1, (args.empty() ? 1 : args[0]) - 1));
            break;
            
        case 'H':  // Cursor Position
            if (args.size() >= 2) {
                cursorY_ = std::max(0, std::min(rows_ - 1, args[0] - 1));
                cursorX_ = std::max(0, std::min(cols_ - 1, args[1] - 1));
            } else {
                cursorY_ = 0;
                cursorX_ = 0;
            }
            break;
            
        case 'J':  // Erase Display
            eraseDisplay(args.empty() ? 0 : args[0]);
            break;
            
        case 'K':  // Erase Line
            eraseLine(args.empty() ? 0 : args[0]);
            break;
            
        case 'm':  // Select Graphic Rendition
            setGraphicsMode(args);
            break;
            
        case 'h':  // Set Mode
            break;
            
        case 'l':  // Reset Mode
            break;
            
        case 's':  // Save Cursor
            savedCursorX_ = cursorX_;
            savedCursorY_ = cursorY_;
            break;
            
        case 'u':  // Restore Cursor
            cursorX_ = savedCursorX_;
            cursorY_ = savedCursorY_;
            break;
    }
}

void TerminalEmulator::executeEscape(const std::string& seq) {
    if (seq == "7") {
        // Save cursor
        savedCursorX_ = cursorX_;
        savedCursorY_ = cursorY_;
    } else if (seq == "8") {
        // Restore cursor
        cursorX_ = savedCursorX_;
        cursorY_ = savedCursorY_;
    } else if (seq == "D") {
        // Index
        cursorY_++;
        if (cursorY_ > scrollBottom_) {
            scrollUp(1);
            cursorY_ = scrollBottom_;
        }
    } else if (seq == "M") {
        // Reverse Index
        cursorY_--;
        if (cursorY_ < scrollTop_) {
            scrollDown(1);
            cursorY_ = scrollTop_;
        }
    } else if (seq == "E") {
        // Next Line
        cursorY_++;
        if (cursorY_ > scrollBottom_) {
            scrollUp(1);
            cursorY_ = scrollBottom_;
        }
        cursorX_ = 0;
    }
}

std::vector<int> TerminalEmulator::parseParams(const std::string& params) {
    std::vector<int> result;
    
    if (params.empty()) return result;
    
    std::stringstream ss(params);
    std::string token;
    
    while (std::getline(ss, token, ';')) {
        if (!token.empty()) {
            try {
                result.push_back(std::stoi(token));
            } catch (...) {
                result.push_back(0);
            }
        } else {
            result.push_back(0);
        }
    }
    
    return result;
}

void TerminalEmulator::eraseDisplay(int mode) {
    if (rows_ <= 0 || cols_ <= 0) return;
    int cy = std::clamp(cursorY_, 0, rows_ - 1);
    int cx = std::clamp(cursorX_, 0, cols_ - 1);

    switch (mode) {
        case 0:  // Erase from cursor to end
            for (int y = cy; y < rows_; ++y) {
                int startX = (y == cy) ? cx : 0;
                for (int x = startX; x < cols_; ++x) {
                    screen_[y][x] = Cell{};
                }
            }
            break;

        case 1:  // Erase from start to cursor
            for (int y = 0; y <= cy; ++y) {
                int endX = (y == cy) ? std::min(cols_, cx + 1) : cols_;
                for (int x = 0; x < endX; ++x) {
                    screen_[y][x] = Cell{};
                }
            }
            break;

        case 2:  // Erase all
            clear();
            break;
    }
}

void TerminalEmulator::eraseLine(int mode) {
    if (cursorY_ < 0 || cursorY_ >= rows_ || cols_ <= 0) return;
    int cx = std::clamp(cursorX_, 0, cols_ - 1);

    switch (mode) {
        case 0:  // Erase from cursor to end of line
            for (int x = cx; x < cols_; ++x) {
                screen_[cursorY_][x] = Cell{};
            }
            break;

        case 1:  // Erase from start of line to cursor
            for (int x = 0; x <= cx; ++x) {
                screen_[cursorY_][x] = Cell{};
            }
            break;

        case 2:  // Erase entire line
            for (int x = 0; x < cols_; ++x) {
                screen_[cursorY_][x] = Cell{};
            }
            break;
    }
}

void TerminalEmulator::setGraphicsMode(const std::vector<int>& params) {
    if (params.empty()) {
        // Reset
        currentFg_ = defaultFg_;
        currentBg_ = defaultBg_;
        currentBold_ = false;
        currentUnderline_ = false;
        currentInverse_ = false;
        return;
    }
    
    for (int param : params) {
        switch (param) {
            case 0:  // Reset
                currentFg_ = defaultFg_;
                currentBg_ = defaultBg_;
                currentBold_ = false;
                currentUnderline_ = false;
                currentInverse_ = false;
                break;
            case 1:  // Bold
                currentBold_ = true;
                break;
            case 4:  // Underline
                currentUnderline_ = true;
                break;
            case 7:  // Inverse
                currentInverse_ = true;
                break;
            case 22:  // Normal intensity
                currentBold_ = false;
                break;
            case 24:  // No underline
                currentUnderline_ = false;
                break;
            case 27:  // No inverse
                currentInverse_ = false;
                break;
            case 30: case 31: case 32: case 33:
            case 34: case 35: case 36: case 37:
                // Foreground colors
                currentFg_ = ansiColors_[param - 30];
                break;
            case 40: case 41: case 42: case 43:
            case 44: case 45: case 46: case 47:
                // Background colors
                currentBg_ = ansiColors_[param - 40];
                break;
            case 90: case 91: case 92: case 93:
            case 94: case 95: case 96: case 97:
                // Bright foreground colors
                currentFg_ = ansiColors_[param - 90 + 8];
                break;
            case 100: case 101: case 102: case 103:
            case 104: case 105: case 106: case 107:
                // Bright background colors
                currentBg_ = ansiColors_[param - 100 + 8];
                break;
        }
    }
}

std::string TerminalEmulator::getKeyPressSequence(int keyCode, int modifiers) {
    bool ctrl = (modifiers & 1) != 0;
    bool shift = (modifiers & 2) != 0;
    bool alt = (modifiers & 4) != 0;
    
    std::string prefix = alt ? "\x1b" : "";
    
    switch (keyCode) {
        case 8:   // Backspace
            return ctrl ? "\x17" : prefix + "\x7f";
        case 9:   // Tab
            return shift ? "\x1b[Z" : "\t";
        case 13:  // Enter
            return "\r";
        case 27:  // Escape
            return "\x1b";
        case 32:  // Space
            return ctrl ? std::string(1, '\0') : " ";
        case 37:  // Left
            return prefix + (ctrl ? "\x1b[1;5D" : "\x1b[D");
        case 38:  // Up
            return prefix + (ctrl ? "\x1b[1;5A" : "\x1b[A");
        case 39:  // Right
            return prefix + (ctrl ? "\x1b[1;5C" : "\x1b[C");
        case 40:  // Down
            return prefix + (ctrl ? "\x1b[1;5B" : "\x1b[B");
        case 33:  // Page Up
            return prefix + "\x1b[5~";
        case 34:  // Page Down
            return prefix + "\x1b[6~";
        case 35:  // Home
            return prefix + "\x1b[H";
        case 36:  // End
            return prefix + "\x1b[F";
        case 45:  // Insert
            return prefix + "\x1b[2~";
        case 46:  // Delete
            return prefix + "\x1b[3~";
        case 112: case 113: case 114: case 115:
            // F1-F4 (VT100 SS3)
            return prefix + "\x1bO" + std::string(1, static_cast<char>('P' + (keyCode - 112)));
        case 116: return prefix + "\x1b[15~"; // F5
        case 117: return prefix + "\x1b[17~"; // F6
        case 118: return prefix + "\x1b[18~"; // F7
        case 119: return prefix + "\x1b[19~"; // F8
        case 120: return prefix + "\x1b[20~"; // F9
        case 121: return prefix + "\x1b[21~"; // F10
        case 122: return prefix + "\x1b[23~"; // F11
        case 123: return prefix + "\x1b[24~"; // F12
    }

    return "";
}

const std::vector<std::vector<TerminalEmulator::Cell>>& TerminalEmulator::getScreen() const {
    return screen_;
}

int TerminalEmulator::getCursorX() const {
    return cursorX_;
}

int TerminalEmulator::getCursorY() const {
    return cursorY_;
}

void TerminalEmulator::setCursorVisible(bool visible) {
    cursorVisible_ = visible;
}

bool TerminalEmulator::isCursorVisible() const {
    return cursorVisible_;
}

int TerminalEmulator::getRows() const {
    return rows_;
}

int TerminalEmulator::getCols() const {
    return cols_;
}

bool TerminalEmulator::isProcessRunning() const {
    return pty_ && pty_->isRunning();
}

void TerminalEmulator::kill() {
    if (pty_) {
        pty_->kill();
    }
}

void TerminalEmulator::copySelection() {
    // Copy selected text to clipboard
}

void TerminalEmulator::paste() {
    // Paste from clipboard
}

std::unique_ptr<PTYHandler> createPTYHandler() {
    return std::make_unique<PTYHandler>();
}

} // namespace NexusForge::Terminal
