// editor/features/multi_cursor.cpp
// Multi-Cursor Implementation

#include "multi_cursor.hpp"
#include "../buffer/text_buffer.hpp"
#include <algorithm>
#include <cctype>
#include <set>

namespace NexusForge::Editor {

MultiCursorManager::MultiCursorManager() : buffer_(nullptr) {
}

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
    if (buffer_ == nullptr || buffer_->getLineCount() == 0) return;

    cursors_.clear();
    size_t lastLine = buffer_->getLineCount() - 1;
    size_t lastCol = buffer_->getLineLength(lastLine);
    Cursor cursor(0, 0, lastLine, lastCol);
    cursor.line = lastLine;
    cursor.col = lastCol;
    cursor.selectionStart = {0, 0};
    cursor.selectionEnd = {lastLine, lastCol};
    cursors_.push_back(cursor);
}

void MultiCursorManager::selectLine(size_t line) {
    if (buffer_ == nullptr || line >= buffer_->getLineCount()) return;

    cursors_.clear();
    size_t lineLen = buffer_->getLineLength(line);
    Cursor cursor(line, 0, line, lineLen);
    cursor.line = line;
    cursor.col = lineLen;
    cursor.selectionStart = {line, 0};
    cursor.selectionEnd = {line, lineLen};
    cursors_.push_back(cursor);
}

void MultiCursorManager::selectWord(size_t line, size_t col) {
    if (buffer_ == nullptr || line >= buffer_->getLineCount()) return;

    std::string lineText = buffer_->getLine(line);
    if (col > lineText.length()) col = lineText.length();

    // Find word boundaries
    size_t start = col;
    size_t end = col;

    while (start > 0 && !std::isspace(static_cast<unsigned char>(lineText[start - 1])) &&
           !std::ispunct(static_cast<unsigned char>(lineText[start - 1]))) {
        start--;
    }

    while (end < lineText.length() && !std::isspace(static_cast<unsigned char>(lineText[end])) &&
           !std::ispunct(static_cast<unsigned char>(lineText[end]))) {
        end++;
    }

    cursors_.clear();
    Cursor cursor(line, start, line, end);
    cursor.line = line;
    cursor.col = end;
    cursor.selectionStart = {line, start};
    cursor.selectionEnd = {line, end};
    cursors_.push_back(cursor);
}

void MultiCursorManager::matchOccurrences(const std::string& word) {
    if (buffer_ == nullptr || word.empty()) return;

    cursors_.clear();

    for (size_t line = 0; line < buffer_->getLineCount(); ++line) {
        std::string lineText = buffer_->getLine(line);
        size_t pos = 0;

        while ((pos = lineText.find(word, pos)) != std::string::npos) {
            Cursor cursor(line, pos, line, pos + word.length());
            cursor.line = line;
            cursor.col = pos + word.length();
            cursor.selectionStart = {line, pos};
            cursor.selectionEnd = {line, pos + word.length()};
            cursors_.push_back(cursor);
            pos += word.length();
        }
    }
}

void MultiCursorManager::insertText(const std::string& text) {
    if (buffer_ == nullptr || cursors_.empty()) return;

    // Sort cursors in reverse order to insert from bottom to top
    std::sort(cursors_.begin(), cursors_.end(),
        [](const Cursor& a, const Cursor& b) {
            if (a.line != b.line) return a.line > b.line;
            return a.col > b.col;
        });

    for (auto& cursor : cursors_) {
        buffer_->insert(cursor.line, cursor.col, text);
        cursor.col += text.length();
        cursor.selectionStart = {cursor.line, cursor.col};
        cursor.selectionEnd = {cursor.line, cursor.col};
    }

    normalizeCursors();
}

void MultiCursorManager::deleteSelection() {
    if (buffer_ == nullptr || cursors_.empty()) return;

    // Sort cursors in reverse order directly so updates persist
    std::sort(cursors_.begin(), cursors_.end(),
        [](const Cursor& a, const Cursor& b) {
            if (a.line != b.line) return a.line > b.line;
            return std::max(a.selectionStart.col, a.col) > std::max(b.selectionStart.col, b.col);
        });

    for (auto& cursor : cursors_) {
        Position p1 = cursor.selectionStart;
        Position p2 = {cursor.line, cursor.col};
        if (p2.line < p1.line || (p2.line == p1.line && p2.col < p1.col)) {
            std::swap(p1, p2);
        }

        if (p1.line != p2.line || p1.col != p2.col) {
            buffer_->removeRange(p1.line, p1.col, p2.line, p2.col);
            cursor.line = p1.line;
            cursor.col = p1.col;
            cursor.selectionStart = p1;
            cursor.selectionEnd = p1;
        }
    }

    normalizeCursors();
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

void MultiCursorManager::setCursorPosition(size_t line, size_t column) {
    cursors_.clear();
    cursors_.emplace_back(line, column, line, column, column);
}

void MultiCursorManager::addCursorAtLineEnd(size_t line) {
    size_t col = buffer_ ? buffer_->getLineLength(line) : 0;
    addCursor(line, col);
}

void MultiCursorManager::addCursorsAtAllLineEnds() {
    addCursorsAtLineEnds();
}

void MultiCursorManager::removeCursor(size_t index) {
    if (index < cursors_.size()) {
        cursors_.erase(cursors_.begin() + index);
    }
}

void MultiCursorManager::removeCursorAbove() {
    if (cursors_.size() > 1) cursors_.erase(cursors_.begin());
}

void MultiCursorManager::removeCursorBelow() {
    if (cursors_.size() > 1) cursors_.pop_back();
}

void MultiCursorManager::clearAllCursors() {
    cursors_.clear();
}

void MultiCursorManager::moveUp() { moveAllCursors(-1, 0); }
void MultiCursorManager::moveDown() { moveAllCursors(1, 0); }
void MultiCursorManager::moveLeft() { moveAllCursors(0, -1); }
void MultiCursorManager::moveRight() { moveAllCursors(0, 1); }
void MultiCursorManager::moveWordLeft() { moveAllCursors(0, -1); }
void MultiCursorManager::moveWordRight() { moveAllCursors(0, 1); }
void MultiCursorManager::moveLineStart() {
    for (auto& c : cursors_) { c.col = 0; c.selectionStart = {c.line, 0}; c.selectionEnd = {c.line, 0}; }
}
void MultiCursorManager::moveLineEnd() {
    if (!buffer_) return;
    for (auto& c : cursors_) {
        c.col = buffer_->getLineLength(c.line);
        c.selectionStart = {c.line, c.col};
        c.selectionEnd = {c.line, c.col};
    }
}
void MultiCursorManager::moveDocumentStart() { setCursorPosition(0, 0); }
void MultiCursorManager::moveDocumentEnd() {
    if (!buffer_ || buffer_->getLineCount() == 0) return;
    size_t last = buffer_->getLineCount() - 1;
    setCursorPosition(last, buffer_->getLineLength(last));
}

void MultiCursorManager::selectUp() { moveAllCursors(-1, 0); }
void MultiCursorManager::selectDown() { moveAllCursors(1, 0); }
void MultiCursorManager::selectLeft() { moveAllCursors(0, -1); }
void MultiCursorManager::selectRight() { moveAllCursors(0, 1); }
void MultiCursorManager::selectWord() {
    if (!cursors_.empty()) selectWord(cursors_[0].line, cursors_[0].col);
}
void MultiCursorManager::selectLine() {
    if (!cursors_.empty()) selectLine(cursors_[0].line);
}
void MultiCursorManager::expandSelection() {}
void MultiCursorManager::shrinkSelection() {}

void MultiCursorManager::startColumnSelection(size_t startLine, size_t startColumn) {
    columnSelection_ = true;
    columnSelectionStart_ = startLine;
    columnSelectionEnd_ = startLine;
    setCursorPosition(startLine, startColumn);
}

void MultiCursorManager::extendColumnSelection(size_t endLine, size_t endColumn) {
    columnSelectionEnd_ = endLine;
    size_t minLine = std::min(columnSelectionStart_, columnSelectionEnd_);
    size_t maxLine = std::max(columnSelectionStart_, columnSelectionEnd_);
    cursors_.clear();
    for (size_t l = minLine; l <= maxLine; ++l) {
        addCursor(l, endColumn);
    }
}

std::vector<std::pair<size_t, size_t>> MultiCursorManager::getColumnSelectionRanges() const {
    std::vector<std::pair<size_t, size_t>> ranges;
    for (const auto& c : cursors_) {
        ranges.emplace_back(c.line, c.col);
    }
    return ranges;
}

void MultiCursorManager::forEachCursor(OperationFunc op) {
    if (!op) return;
    for (const auto& c : cursors_) {
        op(c.line, c.col);
    }
}

void MultiCursorManager::forEachSelection(RangeOperationFunc op) {
    if (!op) return;
    for (const auto& c : cursors_) {
        op(c.selectionStart.line, c.selectionStart.col, c.selectionEnd.line, c.selectionEnd.col);
    }
}

void MultiCursorManager::mergeOverlappingCursors() {
    normalizeCursors();
}

void MultiCursorManager::sortCursors() {
    normalizeCursors();
}

void MultiCursorManager::ensureValidCursors(size_t maxLines, size_t maxColumns) {
    for (auto& c : cursors_) {
        if (maxLines > 0 && c.line >= maxLines) c.line = maxLines - 1;
        if (c.col > maxColumns) c.col = maxColumns;
    }
}

bool MultiCursorManager::cursorsOverlap(size_t a, size_t b) const {
    if (a >= cursors_.size() || b >= cursors_.size()) return false;
    return cursors_[a].line == cursors_[b].line && cursors_[a].col == cursors_[b].col;
}

} // namespace NexusForge::Editor
