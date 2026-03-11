// editor/buffer/text_buffer.hpp
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>

namespace NexusForge::Editor {

// Text change for undo/redo
struct TextChange {
    size_t startLine;
    size_t startColumn;
    size_t endLine;
    size_t endColumn;
    std::string oldText;
    std::string newText;

    bool isEmpty() const { return oldText.empty() && newText.empty(); }
};

// Undo action
struct UndoAction {
    enum class Type { Insert, Delete, Replace };
    Type type;
    std::vector<TextChange> changes;
    std::string description;
    bool mergeable = false;
};

// Line information
struct LineInfo {
    size_t lineNumber;
    size_t offset;      // Character offset from start of document
    size_t length;      // Length of line (excluding newline)
    std::string text;   // Cached text (optional)
};

// Text buffer statistics
struct BufferStats {
    size_t lineCount = 0;
    size_t charCount = 0;
    size_t byteCount = 0;
    size_t undoStackDepth = 0;
    size_t redoStackDepth = 0;
};

// Text Buffer - Core document storage
class TextBuffer {
public:
    TextBuffer();
    explicit TextBuffer(const std::string& initialText);
    ~TextBuffer();

    // Document info
    size_t getLineCount() const { return lines_.size(); }
    size_t getCharCount() const { return charCount_; }
    size_t getByteCount() const;
    const BufferStats& getStats() const { return stats_; }

    // Line access
    const std::string& getLine(size_t lineNum) const;
    std::string getLineSafe(size_t lineNum) const;
    size_t getLineLength(size_t lineNum) const;
    size_t getLineOffset(size_t lineNum) const;

    // Character access
    char getChar(size_t line, size_t column) const;
    char getCharAtOffset(size_t offset) const;

    // Position conversion
    size_t offsetToLine(size_t offset) const;
    size_t offsetToColumn(size_t offset, size_t line) const;
    size_t positionToOffset(size_t line, size_t column) const;
    std::pair<size_t, size_t> offsetToPosition(size_t offset) const;

    // Text retrieval
    std::string getText() const;
    std::string getTextRange(size_t startLine, size_t startCol,
                             size_t endLine, size_t endCol) const;
    std::string getLineRange(size_t startLine, size_t endLine) const;

    // Editing operations
    void insert(size_t line, size_t column, const std::string& text);
    void insertRange(size_t startLine, size_t startCol,
                     size_t endLine, size_t endCol, const std::string& text);
    void remove(size_t line, size_t column, size_t length);
    void removeRange(size_t startLine, size_t startCol,
                     size_t endLine, size_t endCol);
    void replace(size_t startLine, size_t startCol,
                 size_t endLine, size_t endCol, const std::string& text);

    // Bulk operations
    void setText(const std::string& text);
    void append(const std::string& text);
    void prepend(const std::string& text);

    // Line operations
    void insertLine(size_t lineNum, const std::string& text = "");
    void removeLine(size_t lineNum);
    void removeLines(size_t startLine, size_t endLine);
    void duplicateLine(size_t lineNum);
    void swapLines(size_t line1, size_t line2);

    // Undo/Redo
    void beginUndoGroup(const std::string& description = "");
    void endUndoGroup();
    void undo();
    void redo();
    void clearUndoStack();
    void clearRedoStack();
    void clearUndoRedo();

    bool canUndo() const { return !undoStack_.empty(); }
    bool canRedo() const { return !redoStack_.empty(); }
    bool isInUndoGroup() const { return undoGroupDepth_ > 0; }

    // Modified state
    bool isModified() const { return modified_; }
    void setModified(bool modified) { modified_ = modified; }
    void markClean() { modified_ = false; }
    void markDirty() { modified_ = true; }

    // Events
    using ChangeCallback = std::function<void(const TextChange&)>;
    using ContentChangedCallback = std::function<void()>;

    size_t addChangeListener(ChangeCallback callback);
    void removeChangeListener(size_t id);
    void addContentChangedListener(ContentChangedCallback callback);

    // Validation
    bool isValidPosition(size_t line, size_t column) const;
    size_t clampColumn(size_t line, size_t column) const;

    // Line endings
    enum class LineEnding { LF, CRLF, CR };
    LineEnding getLineEnding() const { return lineEnding_; }
    void setLineEnding(LineEnding ending) { lineEnding_ = ending; }
    static std::string lineEndingToString(LineEnding ending);
    static LineEnding stringToLineEnding(const std::string& str);

    // Find operations
    size_t find(const std::string& text, size_t startOffset = 0,
                bool caseSensitive = true) const;
    size_t findInLine(const std::string& text, size_t lineNum,
                      size_t startColumn = 0, bool caseSensitive = true) const;
    std::vector<size_t> findAll(const std::string& text,
                                 bool caseSensitive = true) const;

private:
    std::vector<std::string> lines_;
    size_t charCount_ = 0;
    BufferStats stats_;
    bool modified_ = false;
    LineEnding lineEnding_ = LineEnding::LF;

    // Undo/Redo
    std::vector<UndoAction> undoStack_;
    std::vector<UndoAction> redoStack_;
    std::vector<TextChange> currentGroup_;
    int undoGroupDepth_ = 0;
    std::string currentGroupDescription_;

    // Change listeners
    struct Listener {
        size_t id;
        ChangeCallback callback;
    };
    std::vector<Listener> changeListeners_;
    std::vector<ContentChangedCallback> contentChangedListeners_;
    size_t nextListenerId_ = 1;

    // Internal helpers
    void notifyChange(const TextChange& change);
    void notifyContentChanged();
    void updateStats();
    void recalculateCharCount();

    void pushUndoAction(const UndoAction& action);
    void executeUndo(const UndoAction& action);
    void executeRedo(const UndoAction& action);
    bool canMerge(const UndoAction& previous, const UndoAction& current) const;

    // Line management
    void splitLine(size_t lineNum, size_t column);
    void mergeLines(size_t lineNum);
    void ensureLineExists(size_t lineNum);

    // Text normalization
    std::string normalizeLineEndings(const std::string& text) const;
    void splitIntoLines(const std::string& text);
};

// Gap buffer implementation (alternative to simple vector)
class GapBuffer {
public:
    GapBuffer(size_t initialCapacity = 4096);

    void insert(size_t position, const std::string& text);
    void remove(size_t position, size_t length);
    char at(size_t position) const;
    std::string getText(size_t start, size_t end) const;
    std::string getAllText() const;

    size_t length() const { return length_; }
    size_t capacity() const { return buffer_.size(); }

private:
    std::vector<char> buffer_;
    size_t gapStart_ = 0;
    size_t gapEnd_ = 0;
    size_t length_ = 0;

    void moveGap(size_t newPosition);
    void grow(size_t minCapacity);
};

// Rope data structure (for very large files)
class Rope {
public:
    Rope();
    explicit Rope(const std::string& text);

    void insert(size_t position, const std::string& text);
    void remove(size_t position, size_t length);
    char at(size_t position) const;
    std::string substr(size_t start, size_t length) const;
    std::string toString() const;

    size_t length() const { return length_; }

private:
    struct Node {
        std::string text;
        size_t length;
        std::shared_ptr<Node> left;
        std::shared_ptr<Node> right;

        Node(const std::string& t) : text(t), length(t.size()) {}
    };

    std::shared_ptr<Node> root_;
    size_t length_ = 0;

    static constexpr size_t MAX_LEAF_SIZE = 512;

    std::shared_ptr<Node> concat(std::shared_ptr<Node> left,
                                  std::shared_ptr<Node> right);
    std::shared_ptr<Node> insert(std::shared_ptr<Node> node,
                                  size_t position, const std::string& text);
    std::shared_ptr<Node> remove(std::shared_ptr<Node> node,
                                  size_t position, size_t length);
};

} // namespace NexusForge::Editor
