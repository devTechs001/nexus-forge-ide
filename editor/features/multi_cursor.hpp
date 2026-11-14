// editor/features/multi_cursor.hpp
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <utility>

namespace NexusForge::Editor {

class TextBuffer;

// Cursor state
struct CursorState {
    size_t line = 0;
    size_t column = 0;
    size_t preferredColumn = 0;  // For vertical movement
    size_t selectionStart = 0;
    size_t selectionEnd = 0;
    bool active = true;
};

// Multi-cursor manager
class MultiCursorManager {
public:
    struct Position {
        size_t line = 0;
        size_t col = 0;
    };

    struct Cursor {
        size_t line = 0;
        size_t col = 0;
        Position selectionStart{0, 0};
        Position selectionEnd{0, 0};
        size_t preferredCol = 0;

        Cursor() = default;
        Cursor(size_t l, size_t c, size_t selEndLine, size_t selEndCol, size_t pref = 0)
            : line(l), col(c), selectionStart{l, c}, selectionEnd{selEndLine, selEndCol}, preferredCol(pref) {}
    };

    MultiCursorManager();
    explicit MultiCursorManager(TextBuffer* buffer);
    ~MultiCursorManager();

    void setBuffer(TextBuffer* buffer) { buffer_ = buffer; }

    // Cursor management
    void setCursorPosition(size_t line, size_t column);
    void setCursorPosition(size_t index, size_t line, size_t col);
    void addCursor(size_t line, size_t column);
    void addCursorAbove();
    void addCursorBelow();
    void addCursorAtLineEnd(size_t line);
    void addCursorsAtLineEnds();
    void addCursorsAtAllLineEnds();
    void removeCursor(size_t index);
    void removeCursorAbove();
    void removeCursorBelow();
    void clearSecondaryCursors();
    void clearAllCursors();

    // Access
    const std::vector<Cursor>& getCursors() const;
    Cursor getPrimaryCursor() const;
    void setPrimaryCursor(const Cursor& cursor);
    size_t getCursorCount() const;
    bool hasMultipleCursors() const;
    bool hasSelection() const;

    // Movement
    void moveCursor(size_t index, int lineDelta, int colDelta);
    void moveAllCursors(int lineDelta, int colDelta);
    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();
    void moveWordLeft();
    void moveWordRight();
    void moveLineStart();
    void moveLineEnd();
    void moveDocumentStart();
    void moveDocumentEnd();

    // Selection
    void startSelection(size_t index);
    void extendSelection(size_t index, size_t line, size_t col);
    void clearSelection(size_t index);
    void selectUp();
    void selectDown();
    void selectLeft();
    void selectRight();
    void selectWord();
    void selectWord(size_t line, size_t col);
    void selectLine();
    void selectLine(size_t line);
    void selectAll();
    void expandSelection();
    void shrinkSelection();
    void matchOccurrences(const std::string& word);

    // Editing
    void insertText(const std::string& text);
    void deleteSelection();

    // Column selection (box selection)
    void startColumnSelection(size_t startLine, size_t startColumn);
    void extendColumnSelection(size_t endLine, size_t endColumn);
    bool isColumnSelection() const { return columnSelection_; }
    std::vector<std::pair<size_t, size_t>> getColumnSelectionRanges() const;

    // Operations with multiple cursors
    using OperationFunc = std::function<void(size_t line, size_t column)>;
    void forEachCursor(OperationFunc op);

    using RangeOperationFunc = std::function<void(size_t startLine, size_t startCol,
                                                   size_t endLine, size_t endCol)>;
    void forEachSelection(RangeOperationFunc op);

    // Merge / sort
    void normalizeCursors();
    void mergeOverlappingCursors();
    void sortCursors();

private:
    TextBuffer* buffer_ = nullptr;
    std::vector<Cursor> cursors_;
    bool columnSelection_ = false;
    size_t columnSelectionStart_ = 0;
    size_t columnSelectionEnd_ = 0;

    void ensureValidCursors(size_t maxLines, size_t maxColumns);
    bool cursorsOverlap(size_t a, size_t b) const;
};

} // namespace NexusForge::Editor
