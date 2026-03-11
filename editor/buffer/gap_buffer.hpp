// editor/buffer/gap_buffer.hpp
#pragma once

#include <string>
#include <vector>
#include <cstddef>

namespace NexusForge::Editor {

// Gap Buffer - Efficient text buffer for editors
class GapBuffer {
public:
    GapBuffer(size_t initialCapacity = 4096);
    ~GapBuffer() = default;

    // Basic operations
    void insert(char c);
    void insert(const std::string& text);
    void insert(size_t position, char c);
    void insert(size_t position, const std::string& text);

    void remove();  // Remove char at gap
    void remove(size_t position, size_t length);
    void removeRange(size_t start, size_t end);

    char at(size_t position) const;
    char operator[](size_t position) const { return at(position); }

    // Navigation
    void move(size_t position);
    void moveLeft();
    void moveRight();
    void moveLineUp();
    void moveLineDown();

    // Content access
    std::string getText() const;
    std::string getText(size_t start, size_t end) const;
    std::string getLine(size_t lineNum) const;
    size_t getLineCount() const;
    size_t getLineLength(size_t lineNum) const;
    size_t getLineOffset(size_t lineNum) const;

    // Position utilities
    size_t positionToOffset(size_t line, size_t column) const;
    std::pair<size_t, size_t> offsetToPosition(size_t offset) const;
    size_t lineStart(size_t lineNum) const;
    size_t lineEnd(size_t lineNum) const;

    // Cursor
    size_t cursorPosition() const { return gapStart_; }
    void setCursorPosition(size_t position);

    // Document info
    size_t length() const { return length_; }
    size_t capacity() const { return buffer_.size(); }
    bool empty() const { return length_ == 0; }

    // Undo support
    void saveState();
    void restoreState();

    // Clear
    void clear();

private:
    std::vector<char> buffer_;
    size_t gapStart_ = 0;
    size_t gapEnd_ = 0;
    size_t length_ = 0;
    size_t lineCount_ = 1;

    // State for undo
    struct State {
        size_t gapStart;
        size_t gapEnd;
        size_t length;
        size_t lineCount;
        std::vector<char> buffer;
    };
    std::vector<State> undoStack_;

    void ensureGapSize(size_t needed);
    void grow(size_t minCapacity);
    void moveGap(size_t newPosition);
    void recalculateLineCount();
    void updateLineCount(size_t start, size_t end, const std::string& newText);
};

} // namespace NexusForge::Editor
