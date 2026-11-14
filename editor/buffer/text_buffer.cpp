// editor/buffer/text_buffer.cpp
#include "text_buffer.hpp"
#include <fstream>
#include <sstream>

namespace NexusForge::Editor {

TextBuffer::TextBuffer() {
    lines_.push_back("");
}

TextBuffer::~TextBuffer() = default;

void TextBuffer::setText(const std::string& text) {
    std::lock_guard<std::mutex> lock(mutex_);
    lines_.clear();

    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        lines_.push_back(line);
    }
    if (!text.empty() && text.back() == '\n') {
        lines_.push_back("");
    }
    if (lines_.empty()) {
        lines_.push_back("");
    }

    modified_ = true;
}

std::string TextBuffer::getText() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string result;
    for (size_t i = 0; i < lines_.size(); ++i) {
        result += lines_[i];
        if (i < lines_.size() - 1) {
            result += '\n';
        }
    }
    return result;
}

std::string TextBuffer::getLine(size_t lineNum) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (lineNum < lines_.size()) {
        return lines_[lineNum];
    }
    return "";
}

size_t TextBuffer::getLineLength(size_t lineNum) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (lineNum < lines_.size()) {
        return lines_[lineNum].length();
    }
    return 0;
}

size_t TextBuffer::getTotalLength() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (lines_.empty()) return 0;
    size_t total = 0;
    for (const auto& line : lines_) {
        total += line.length();
    }
    total += lines_.size() - 1; // newlines between lines
    return total;
}

char TextBuffer::getChar(size_t line, size_t col) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (line < lines_.size() && col < lines_[line].length()) {
        return lines_[line][col];
    }
    return '\0';
}

std::string TextBuffer::getRangeUnlocked(size_t startLine, size_t startCol,
                                         size_t endLine, size_t endCol) const {
    if (lines_.empty() || startLine >= lines_.size() || startLine > endLine) return {};
    if (endLine >= lines_.size()) {
        endLine = lines_.size() - 1;
        endCol = lines_[endLine].length();
    }

    std::string result;
    if (startLine == endLine) {
        size_t lineLen = lines_[startLine].length();
        size_t s = std::min(startCol, lineLen);
        size_t e = std::min(endCol, lineLen);
        if (e > s) {
            result = lines_[startLine].substr(s, e - s);
        }
    } else {
        size_t s = std::min(startCol, lines_[startLine].length());
        result = lines_[startLine].substr(s);
        result += '\n';
        for (size_t i = startLine + 1; i < endLine; ++i) {
            result += lines_[i] + '\n';
        }
        size_t e = std::min(endCol, lines_[endLine].length());
        result += lines_[endLine].substr(0, e);
    }
    return result;
}

std::string TextBuffer::getRange(size_t startLine, size_t startCol,
                                  size_t endLine, size_t endCol) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return getRangeUnlocked(startLine, startCol, endLine, endCol);
}

void TextBuffer::insertUnlocked(size_t line, size_t col, const std::string& text) {
    ensureLineExists(line);
    if (text.empty()) return;

    size_t newlinePos = text.find('\n');
    if (newlinePos == std::string::npos) {
        std::string& currentLine = lines_[line];
        if (col > currentLine.length()) col = currentLine.length();
        currentLine.insert(col, text);
    } else {
        std::string& currentLine = lines_[line];
        if (col > currentLine.length()) col = currentLine.length();
        std::string before = currentLine.substr(0, col);
        std::string after = currentLine.substr(col);

        std::vector<std::string> parts;
        size_t start = 0;
        while (true) {
            size_t nl = text.find('\n', start);
            if (nl == std::string::npos) {
                parts.push_back(text.substr(start));
                break;
            }
            parts.push_back(text.substr(start, nl - start));
            start = nl + 1;
        }

        lines_[line] = before + parts[0];
        for (size_t i = 1; i < parts.size(); ++i) {
            lines_.insert(lines_.begin() + line + i, parts[i]);
        }
        lines_[line + parts.size() - 1] += after;
    }
}

std::string TextBuffer::removeUnlocked(size_t line, size_t col, size_t length) {
    if (line >= lines_.size() || length == 0) return {};
    std::string& currentLine = lines_[line];
    if (col >= currentLine.length()) return {};

    length = std::min(length, currentLine.length() - col);
    std::string removed = currentLine.substr(col, length);
    currentLine.erase(col, length);
    return removed;
}

void TextBuffer::removeTextAtUnlocked(size_t line, size_t col, const std::string& text) {
    if (line >= lines_.size() || text.empty()) return;
    size_t endLine = line;
    size_t endCol = col;
    for (char c : text) {
        if (c == '\n') {
            endLine++;
            endCol = 0;
        } else {
            endCol++;
        }
    }
    if (endLine == line) {
        removeUnlocked(line, col, endCol - col);
    } else {
        if (endLine >= lines_.size()) {
            endLine = lines_.size() - 1;
            endCol = lines_[endLine].length();
        }
        size_t s = std::min(col, lines_[line].length());
        size_t e = std::min(endCol, lines_[endLine].length());
        std::string prefix = lines_[line].substr(0, s);
        std::string suffix = lines_[endLine].substr(e);
        lines_[line] = prefix + suffix;
        lines_.erase(lines_.begin() + line + 1, lines_.begin() + endLine + 1);
    }
}

void TextBuffer::insert(size_t line, size_t col, const std::string& text) {
    if (text.empty()) return;
    TextChange change{TextChange::Type::Insert, line, col, text, ""};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        insertUnlocked(line, col, text);
        modified_ = true;
        pushUndo(UndoAction::Type::Insert, line, col, text);
    }
    notifyChange(change);
}

void TextBuffer::remove(size_t line, size_t col, size_t length) {
    std::string removed;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        removed = removeUnlocked(line, col, length);
        if (removed.empty()) return;
        modified_ = true;
        pushUndo(UndoAction::Type::Delete, line, col, removed);
    }
    notifyChange({TextChange::Type::Delete, line, col, "", removed});
}

void TextBuffer::removeRange(size_t startLine, size_t startCol,
                              size_t endLine, size_t endCol) {
    std::string removed;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (lines_.empty() || startLine >= lines_.size() || startLine > endLine) return;
        if (endLine >= lines_.size()) {
            endLine = lines_.size() - 1;
            endCol = lines_[endLine].length();
        }
        removed = getRangeUnlocked(startLine, startCol, endLine, endCol);
        if (removed.empty()) return;

        if (startLine == endLine) {
            size_t s = std::min(startCol, lines_[startLine].length());
            size_t e = std::min(endCol, lines_[startLine].length());
            if (e > s) {
                lines_[startLine].erase(s, e - s);
            }
        } else {
            size_t s = std::min(startCol, lines_[startLine].length());
            size_t e = std::min(endCol, lines_[endLine].length());
            std::string prefix = lines_[startLine].substr(0, s);
            std::string suffix = lines_[endLine].substr(e);
            lines_[startLine] = prefix + suffix;
            lines_.erase(lines_.begin() + startLine + 1, lines_.begin() + endLine + 1);
        }

        modified_ = true;
        pushUndo(UndoAction::Type::Delete, startLine, startCol, removed);
    }
    notifyChange({TextChange::Type::Delete, startLine, startCol, "", removed});
}

void TextBuffer::replace(size_t line, size_t col, size_t length,
                          const std::string& newText) {
    remove(line, col, length);
    insert(line, col, newText);
}

void TextBuffer::insertLine(size_t lineNum, const std::string& text) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (lineNum > lines_.size()) {
        lineNum = lines_.size();
    }
    lines_.insert(lines_.begin() + lineNum, text);
    modified_ = true;
}

void TextBuffer::deleteLine(size_t lineNum) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (lineNum < lines_.size() && lines_.size() > 1) {
        lines_.erase(lines_.begin() + lineNum);
        modified_ = true;
    } else if (lines_.size() == 1) {
        lines_[0] = "";
        modified_ = true;
    }
}

void TextBuffer::appendLine(const std::string& text) {
    std::lock_guard<std::mutex> lock(mutex_);
    lines_.push_back(text);
    modified_ = true;
}

void TextBuffer::undo() {
    TextChange change{};
    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (undoStack_.empty()) return;

        UndoAction action = undoStack_.back();
        undoStack_.pop_back();

        if (action.type == UndoAction::Type::Insert) {
            removeTextAtUnlocked(action.line, action.column, action.text);
            change = {TextChange::Type::Delete, action.line, action.column, "", action.text};
        } else {
            insertUnlocked(action.line, action.column, action.text);
            change = {TextChange::Type::Insert, action.line, action.column, action.text, ""};
        }

        redoStack_.push_back(action);
        modified_ = true;
        changed = true;
    }
    if (changed) notifyChange(change);
}

void TextBuffer::redo() {
    TextChange change{};
    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (redoStack_.empty()) return;

        UndoAction action = redoStack_.back();
        redoStack_.pop_back();

        if (action.type == UndoAction::Type::Insert) {
            insertUnlocked(action.line, action.column, action.text);
            change = {TextChange::Type::Insert, action.line, action.column, action.text, ""};
        } else {
            removeTextAtUnlocked(action.line, action.column, action.text);
            change = {TextChange::Type::Delete, action.line, action.column, "", action.text};
        }

        undoStack_.push_back(action);
        modified_ = true;
        changed = true;
    }
    if (changed) notifyChange(change);
}

void TextBuffer::clearUndoHistory() {
    std::lock_guard<std::mutex> lock(mutex_);
    undoStack_.clear();
    redoStack_.clear();
}

void TextBuffer::onTextChanged(TextChangedCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_.push_back(std::move(callback));
}

bool TextBuffer::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    setText(buffer.str());
    
    filePath_ = path;
    modified_ = false;
    return true;
}

bool TextBuffer::saveToFile(const std::string& path) {
    std::ofstream file(path.empty() ? filePath_ : path);
    if (!file.is_open()) return false;
    
    file << getText();
    modified_ = false;
    return true;
}

size_t TextBuffer::getFirstNonWhitespaceColumn(size_t lineNum) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (lineNum >= lines_.size()) return 0;
    
    const std::string& line = lines_[lineNum];
    for (size_t i = 0; i < line.length(); ++i) {
        if (!std::isspace(static_cast<unsigned char>(line[i]))) {
            return i;
        }
    }
    return line.length();
}

bool TextBuffer::isLineEmpty(size_t lineNum) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (lineNum >= lines_.size()) return true;
    
    const std::string& line = lines_[lineNum];
    for (char c : line) {
        if (!std::isspace(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

void TextBuffer::notifyChange(const TextChange& change) {
    std::vector<TextChangedCallback> snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot = callbacks_;
    }
    for (auto& cb : snapshot) {
        if (cb) cb(change);
    }
}

void TextBuffer::pushUndo(UndoAction::Type type, size_t line, size_t col, 
                           const std::string& text) {
    undoStack_.push_back({type, line, col, text});
    // Limit undo stack size
    if (undoStack_.size() > 1000) {
        undoStack_.erase(undoStack_.begin());
    }
    redoStack_.clear();
}

void TextBuffer::ensureLineExists(size_t lineNum) {
    while (lines_.size() <= lineNum) {
        lines_.push_back("");
    }
}

} // namespace NexusForge::Editor
