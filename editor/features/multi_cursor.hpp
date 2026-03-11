// editor/features/multi_cursor.hpp
#pragma once

#include <vector>
#include <functional>
#include <optional>

namespace NexusForge::Editor {

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
    MultiCursorManager();
    ~MultiCursorManager();

    // Cursor management
    void setCursorPosition(size_t line, size_t column);
    void addCursor(size_t line, size_t column);
    void addCursorAbove();
    void addCursorBelow();
    void addCursorAtLineEnd(size_t line);
    void addCursorsAtAllLineEnds();
    void removeCursor(size_t index);
    void removeCursorAbove();
    void removeCursorBelow();
    void clearSecondaryCursors();
    void clearAllCursors();

    // Access
    const std::vector<CursorState>& getCursors() const { return cursors_; }
    CursorState& getPrimaryCursor() { return cursors_.front(); }
    const CursorState& getPrimaryCursor() const { return cursors_.front(); }
    size_t getCursorCount() const { return cursors_.size(); }
    bool hasMultipleCursors() const { return cursors_.size() > 1; }

    // Movement
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
    void selectUp();
    void selectDown();
    void selectLeft();
    void selectRight();
    void selectWord();
    void selectLine();
    void selectAll();
    void expandSelection();
    void shrinkSelection();

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

    // Merge overlapping cursors
    void mergeOverlappingCursors();

    // Sort cursors by position
    void sortCursors();

private:
    std::vector<CursorState> cursors_;
    bool columnSelection_ = false;
    size_t columnSelectionStart_ = 0;
    size_t columnSelectionEnd_ = 0;

    void ensureValidCursors(size_t maxLines, size_t maxColumns);
    bool cursorsOverlap(size_t a, size_t b) const;
};

} // namespace NexusForge::Editor
