// editor/buffer/piece_table.hpp
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace NexusForge::Editor {

// Piece table for efficient text editing
class PieceTable {
public:
    PieceTable();
    explicit PieceTable(const std::string& initialText);
    ~PieceTable();

    // Editing operations
    void insert(size_t position, const std::string& text);
    void remove(size_t position, size_t length);
    void replace(size_t position, size_t length, const std::string& text);
    
    // Text retrieval
    char at(size_t position) const;
    std::string getText() const;
    std::string getTextRange(size_t start, size_t end) const;
    
    // Document info
    size_t length() const { return totalLength_; }
    size_t lineCount() const;
    
    // Line operations
    std::string getLine(size_t lineNum) const;
    size_t lineToOffset(size_t line, size_t column) const;
    std::pair<size_t, size_t> offsetToLine(size_t offset) const;
    
    // Undo/Redo
    void beginUndo();
    void endUndo();
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;

private:
    enum class BufferType { Original, Added };
    
    struct Piece {
        BufferType buffer;
        size_t start;     // Start offset in buffer
        size_t length;    // Length of piece
        size_t lineCount; // Cached line count
        
        Piece(BufferType b, size_t s, size_t len) 
            : buffer(b), start(s), length(len), lineCount(0) {}
    };
    
    std::string original_;      // Original text (immutable)
    std::string added_;         // Added text (append-only)
    std::vector<Piece> pieces_; // Piece table
    
    size_t totalLength_ = 0;
    size_t totalLines_ = 1;
    
    struct EditState {
        std::vector<Piece> pieces;
        std::string added;
        size_t totalLength;
        size_t totalLines;
    };
    
    std::vector<EditState> undoStack_;
    std::vector<EditState> redoStack_;
    bool inUndoGroup_ = false;
    EditState undoGroupStart_;
    
    void recalculateLength();
    void recalculateLines();
    size_t findPieceIndex(size_t position) const;
    void updateLineCache(size_t pieceIndex);
};

} // namespace NexusForge::Editor
