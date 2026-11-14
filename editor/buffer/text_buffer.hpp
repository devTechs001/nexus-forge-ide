// editor/buffer/text_buffer.hpp
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>

namespace NexusForge::Editor {

// Text change event
struct TextChange {
    enum class Type { Insert, Delete, Replace };
    Type type;
    size_t line;
    size_t column;
    std::string text;
    std::string oldText;
};

// Undo/Redo action
struct UndoAction {
    enum class Type { Insert, Delete };
    Type type;
    size_t line;
    size_t column;
    std::string text;
};

// Text Buffer
class TextBuffer {
public:
    TextBuffer();
    ~TextBuffer();

    // Content
    void setText(const std::string& text);
    std::string getText() const;
    std::string getLine(size_t lineNum) const;
    size_t getLineCount() const { return lines_.size(); }
    size_t getLineLength(size_t lineNum) const;
    size_t getTotalLength() const;

    // Character access
    char getChar(size_t line, size_t col) const;
    std::string getRange(size_t startLine, size_t startCol, size_t endLine, size_t endCol) const;

    // Editing
    void insert(size_t line, size_t col, const std::string& text);
    void remove(size_t line, size_t col, size_t length);
    void removeRange(size_t startLine, size_t startCol, size_t endLine, size_t endCol);
    void replace(size_t line, size_t col, size_t length, const std::string& newText);

    // Line operations
    void insertLine(size_t lineNum, const std::string& text);
    void deleteLine(size_t lineNum);
    void appendLine(const std::string& text = "");

    // Undo/Redo
    void undo();
    void redo();
    bool canUndo() const { return !undoStack_.empty(); }
    bool canRedo() const { return !redoStack_.empty(); }
    void clearUndoHistory();

    // Events
    using TextChangedCallback = std::function<void(const TextChange&)>;
    void onTextChanged(TextChangedCallback callback);

    // File operations
    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path);
    const std::string& getFilePath() const { return filePath_; }
    bool isModified() const { return modified_; }
    void setModified(bool modified) { modified_ = modified; }

    // Line info
    size_t getFirstNonWhitespaceColumn(size_t lineNum) const;
    bool isLineEmpty(size_t lineNum) const;

private:
    std::vector<std::string> lines_;
    std::string filePath_;
    bool modified_ = false;
    
    std::vector<UndoAction> undoStack_;
    std::vector<UndoAction> redoStack_;
    
    mutable std::mutex mutex_;
    std::vector<TextChangedCallback> callbacks_;

    void notifyChange(const TextChange& change);
    void pushUndo(UndoAction::Type type, size_t line, size_t col, const std::string& text);
    void ensureLineExists(size_t lineNum);
};

} // namespace NexusForge::Editor
