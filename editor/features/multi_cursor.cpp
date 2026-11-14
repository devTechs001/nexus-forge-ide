// editor/features/multi_cursor.cpp
// Multi-Cursor Implementation

#include "multi_cursor.hpp"
#include "../buffer/text_buffer.hpp"
#include <algorithm>
#include <set>

namespace NexusForge::Editor {

MultiCursorManager::MultiCursorManager(TextBuffer* buffer) : buffer_(buffer) {
}

MultiCursorManager::~MultiCursorManager() = default;

void MultiCursorManager::addCursor(size_t line, size_t col) {
    Cursor cursor{line, col, line, col, 0};
    
    // Check for duplicates
    for (const auto& c : cursors_) {
        if (c.line == line && c.col == col) {
            return;
        }
    }
    
    cursors_.push_back(cursor);
    normalizeCursors();
}

void MultiCursorManager::addCursorAbove() {
    if (cursors_.empty() || buffer_ == nullptr) return;
    
    std::vector<Cursor> newCursors;
    
    for (auto& cursor : cursors_) {
        if (cursor.line > 0) {
            Cursor newCursor = cursor;
            newCursor.line--;
            newCursor.col = std::min(newCursor.col, buffer_->getLineLength(cursor.line - 1));
            newCursors.push_back(newCursor);
        }
    }
    
    for (const auto& c : newCursors) {
        addCursor(c.line, c.col);
    }
}

void MultiCursorManager::addCursorBelow() {
    if (cursors_.empty() || buffer_ == nullptr) return;
    
    std::vector<Cursor> newCursors;
    
    for (auto& cursor : cursors_) {
        if (cursor.line < buffer_->getLineCount() - 1) {
            Cursor newCursor = cursor;
            newCursor.line++;
            newCursor.col = std::min(newCursor.col, buffer_->getLineLength(cursor.line + 1));
            newCursors.push_back(newCursor);
        }
    }
    
    for (const auto& c : newCursors) {
        addCursor(c.line, c.col);
    }
}

void MultiCursorManager::addCursorsAtLineEnds() {
    if (buffer_ == nullptr) return;
    
    cursors_.clear();
    
    for (size_t i = 0; i < buffer_->getLineCount(); ++i) {
        Cursor cursor{i, buffer_->getLineLength(i), i, buffer_->getLineLength(i), 0};
        cursors_.push_back(cursor);
    }
}

void MultiCursorManager::clearSecondaryCursors() {
    if (!cursors_.empty()) {
        // Keep only the primary cursor (first one)
        cursors_.erase(cursors_.begin() + 1, cursors_.end());
    }
}

void MultiCursorManager::moveCursor(size_t index, int lineDelta, int colDelta) {
    if (index >= cursors_.size() || buffer_ == nullptr) return;
    
    Cursor& cursor = cursors_[index];
    
    size_t newLine = cursor.line;
    size_t newCol = cursor.col;
    
    // Move lines
    if (lineDelta < 0) {
        newLine = cursor.line > 0 ? cursor.line - 1 : 0;
    } else if (lineDelta > 0) {
        newLine = std::min(cursor.line + 1, buffer_->getLineCount() - 1);
    }
    
    // Move columns
    size_t lineLen = buffer_->getLineLength(newLine);
    if (colDelta < 0) {
        newCol = cursor.col > 0 ? cursor.col - 1 : 0;
    } else if (colDelta > 0) {
        newCol = std::min(cursor.col + 1, lineLen);
    }
    
    cursor.line = newLine;
    cursor.col = newCol;
    
    // Update selection
    if (cursor.selectionStart.line != cursor.line || cursor.selectionStart.col != cursor.col) {
        cursor.selectionEnd = {cursor.line, cursor.col};
    }
}

void MultiCursorManager::moveAllCursors(int lineDelta, int colDelta) {
    for (size_t i = 0; i < cursors_.size(); ++i) {
        moveCursor(i, lineDelta, colDelta);
    }
    normalizeCursors();
}

void MultiCursorManager::setCursorPosition(size_t index, size_t line, size_t col) {
    if (index >= cursors_.size()) return;
    
    cursors_[index].line = line;
    cursors_[index].col = col;
    
    // Update selection end
    if (cursors_[index].selectionStart.line != line || 
        cursors_[index].selectionStart.col != col) {
        cursors_[index].selectionEnd = {line, col};
    }
}

void MultiCursorManager::startSelection(size_t index) {
    if (index >= cursors_.size()) return;
    
    cursors_[index].selectionStart = {cursors_[index].line, cursors_[index].col};
    cursors_[index].selectionEnd = cursors_[index].selectionStart;
}

void MultiCursorManager::extendSelection(size_t index, size_t line, size_t col) {
    if (index >= cursors_.size()) return;
    
    cursors_[index].selectionEnd = {line, col};
}

void MultiCursorManager::clearSelection(size_t index) {
    if (index >= cursors_.size()) return;
    
    cursors_[index].selectionStart = {cursors_[index].line, cursors_[index].col};
    cursors_[index].selectionEnd = cursors_[index].selectionStart;
}

void MultiCursorManager::selectAll() {
    if (buffer_ == nullptr) return;
    
    cursors_.clear();
    Cursor cursor{0, 0, 0, buffer_->getLineLength(0) - 1};
    cursors_.push_back(cursor);
}

void MultiCursorManager::selectLine(size_t line) {
    if (buffer_ == nullptr || line >= buffer_->getLineCount()) return;
    
    cursors_.clear();
    Cursor cursor{line, 0, line, buffer_->getLineLength(line)};
    cursors_.push_back(cursor);
}

void MultiCursorManager::selectWord(size_t line, size_t col) {
    if (buffer_ == nullptr || line >= buffer_->getLineCount()) return;
    
    std::string lineText = buffer_->getLine(line);
    
    // Find word boundaries
    size_t start = col;
    size_t end = col;
    
    while (start > 0 && !std::isspace(lineText[start - 1]) && 
           !std::ispunct(lineText[start - 1])) {
        start--;
    }
    
    while (end < lineText.length() && !std::isspace(lineText[end]) && 
           !std::ispunct(lineText[end])) {
        end++;
    }
    
    cursors_.clear();
    Cursor cursor{line, start, line, end};
    cursors_.push_back(cursor);
}

void MultiCursorManager::matchOccurrences(const std::string& word) {
    if (buffer_ == nullptr || word.empty()) return;
    
    cursors_.clear();
    
    for (size_t line = 0; line < buffer_->getLineCount(); ++line) {
        std::string lineText = buffer_->getLine(line);
        size_t pos = 0;
        
        while ((pos = lineText.find(word, pos)) != std::string::npos) {
            Cursor cursor{line, pos, line, pos + word.length()};
            cursors_.push_back(cursor);
            pos += word.length();
        }
    }
}

void MultiCursorManager::insertText(const std::string& text) {
    if (buffer_ == nullptr || cursors_.empty()) return;
    
    // Sort cursors in reverse order to insert from bottom to top
    std::vector<Cursor> sortedCursors = cursors_;
    std::sort(sortedCursors.begin(), sortedCursors.end(),
        [](const Cursor& a, const Cursor& b) {
            if (a.line != b.line) return a.line > b.line;
            return a.col > b.col;
        });
    
    for (const auto& cursor : sortedCursors) {
        buffer_->insertText(cursor.line, cursor.col, text);
    }
    
    // Update cursor positions
    for (auto& cursor : cursors_) {
        cursor.col += text.length();
    }
}

void MultiCursorManager::deleteSelection() {
    if (buffer_ == nullptr || cursors_.empty()) return;
    
    // Sort in reverse order
    std::vector<Cursor> sortedCursors = cursors_;
    std::sort(sortedCursors.begin(), sortedCursors.end(),
        [](const Cursor& a, const Cursor& b) {
            if (a.line != b.line) return a.line > b.line;
            return std::max(a.selectionStart.col, a.col) > std::max(b.selectionStart.col, b.col);
        });
    
    for (auto& cursor : sortedCursors) {
        size_t startCol = std::min(cursor.selectionStart.col, cursor.col);
        size_t endCol = std::max(cursor.selectionStart.col, cursor.col);
        
        if (startCol != endCol) {
            buffer_->removeText(cursor.line, startCol, cursor.line, endCol);
            cursor.col = startCol;
            cursor.selectionStart = {cursor.line, startCol};
            cursor.selectionEnd = {cursor.line, startCol};
        }
    }
}

void MultiCursorManager::normalizeCursors() {
    // Sort cursors by position
    std::sort(cursors_.begin(), cursors_.end(),
        [](const Cursor& a, const Cursor& b) {
            if (a.line != b.line) return a.line < b.line;
            return a.col < b.col;
        });
    
    // Remove duplicates
    cursors_.erase(
        std::unique(cursors_.begin(), cursors_.end(),
            [](const Cursor& a, const Cursor& b) {
                return a.line == b.line && a.col == b.col;
            }),
        cursors_.end()
    );
}

size_t MultiCursorManager::getCursorCount() const {
    return cursors_.size();
}

const std::vector<MultiCursorManager::Cursor>& MultiCursorManager::getCursors() const {
    return cursors_;
}

MultiCursorManager::Cursor MultiCursorManager::getPrimaryCursor() const {
    if (cursors_.empty()) {
        return {0, 0, 0, 0, 0};
    }
    return cursors_[0];
}

void MultiCursorManager::setPrimaryCursor(const Cursor& cursor) {
    if (cursors_.empty()) {
        cursors_.push_back(cursor);
    } else {
        cursors_[0] = cursor;
    }
}

bool MultiCursorManager::hasMultipleCursors() const {
    return cursors_.size() > 1;
}

bool MultiCursorManager::hasSelection() const {
    for (const auto& cursor : cursors_) {
        if (cursor.selectionStart.line != cursor.line ||
            cursor.selectionStart.col != cursor.col) {
            return true;
        }
    }
    return false;
}

} // namespace NexusForge::Editor
