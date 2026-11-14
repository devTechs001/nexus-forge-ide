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
    size_t total = 0;
    for (const auto& line : lines_) {
        total += line.length() + 1; // +1 for newline
    }
    return total;
}

char TextBuffer::getChar(size_t line, size_t col) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (line < lines_.size() && col < lines_[line].length()) {
        return lines_[line][col];
    }
    return '\0';
}

std::string TextBuffer::getRange(size_t startLine, size_t startCol, 
                                  size_t endLine, size_t endCol) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string result;
    
    if (startLine == endLine) {
        if (startLine < lines_.size()) {
            size_t len = std::min(endCol, lines_[startLine].length()) - startCol;
            result = lines_[startLine].substr(startCol, len);
        }
    } else {
        // Multi-line range
        if (startLine < lines_.size()) {
            result = lines_[startLine].substr(startCol);
            result += '\n';
        }
        for (size_t i = startLine + 1; i < endLine && i < lines_.size(); ++i) {
            result += lines_[i] + '\n';
        }
        if (endLine < lines_.size()) {
            result += lines_[endLine].substr(0, endCol);
        }
    }
    
    return result;
}

void TextBuffer::insert(size_t line, size_t col, const std::string& text) {
    std::lock_guard<std::mutex> lock(mutex_);
    ensureLineExists(line);
    
    if (text.empty()) return;
    
    // Handle newlines in text
    size_t newlinePos = text.find('\n');
    if (newlinePos == std::string::npos) {
        // Simple insert
        std::string& currentLine = lines_[line];
        if (col > currentLine.length()) col = currentLine.length();
        currentLine.insert(col, text);
    } else {
        // Multi-line insert
        std::string firstLine = text.substr(0, newlinePos);
        std::string rest = text.substr(newlinePos + 1);
        
        std::string& currentLine = lines_[line];
        if (col > currentLine.length()) col = currentLine.length();
        
        std::string before = currentLine.substr(0, col);
        std::string after = currentLine.substr(col);
        
        lines_[line] = before + firstLine;
        
        // Insert remaining lines
        std::istringstream stream(rest);
        std::string insertLine;
        while (std::getline(stream, insertLine)) {
            lines_.insert(lines_.begin() + line + 1, insertLine);
        }
        
        // Append the after part to the last inserted line
        if (!after.empty() && lines_.size() > line) {
            lines_.back() += after;
        }
    }
    
    modified_ = true;
    notifyChange({TextChange::Type::Insert, line, col, text, ""});
    pushUndo(UndoAction::Type::Delete, line, col, text);
}

void TextBuffer::remove(size_t line, size_t col, size_t length) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (line >= lines_.size()) return;
    
    std::string& currentLine = lines_[line];
    if (col >= currentLine.length()) return;
    
    length = std::min(length, currentLine.length() - col);
    std::string removed = currentLine.substr(col, length);
    currentLine.erase(col, length);
    
    modified_ = true;
    notifyChange({TextChange::Type::Delete, line, col, "", removed});
    pushUndo(UndoAction::Type::Insert, line, col, removed);
}

void TextBuffer::removeRange(size_t startLine, size_t startCol, 
                              size_t endLine, size_t endCol) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string removed = getRange(startLine, startCol, endLine, endCol);
    
    if (startLine == endLine) {
        remove(startLine, startCol, endCol - startCol);
    } else {
        // Delete full lines between
        std::string firstLineEnd = lines_[startLine].substr(startCol);
        std::string lastLineStart = lines_[endLine].substr(0, endCol);
        lines_[startLine] = firstLineEnd + lastLineStart;
        
        // Remove lines between
        lines_.erase(lines_.begin() + startLine + 1, 
                     lines_.begin() + endLine + 1);
        
        modified_ = true;
        notifyChange({TextChange::Type::Delete, startLine, startCol, "", removed});
    }
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
    insertLine(lines_.size(), text);
}

void TextBuffer::undo() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (undoStack_.empty()) return;
    
    UndoAction action = undoStack_.back();
    undoStack_.pop_back();
    
    if (action.type == UndoAction::Type::Insert) {
        remove(action.line, action.column, action.text.length());
    } else {
        insert(action.line, action.column, action.text);
    }
    
    redoStack_.push_back(action);
}

void TextBuffer::redo() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (redoStack_.empty()) return;
    
    UndoAction action = redoStack_.back();
    redoStack_.pop_back();
    
    if (action.type == UndoAction::Type::Insert) {
        insert(action.line, action.column, action.text);
    } else {
        remove(action.line, action.column, action.text.length());
    }
    
    undoStack_.push_back(action);
}

void TextBuffer::clearUndoHistory() {
    std::lock_guard<std::mutex> lock(mutex_);
    undoStack_.clear();
    redoStack_.clear();
}

void TextBuffer::onTextChanged(TextChangedCallback callback) {
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
    for (auto& cb : callbacks_) {
        cb(change);
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
