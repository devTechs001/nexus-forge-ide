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
    void setText(const std::string& text);
    
    // Text retrieval
    char at(size_t position) const;
    std::string getText() const;
    std::string substr(size_t start, size_t len) const;
    
    // Document info
    size_t length() const { return totalLength(); }
    size_t lineCount() const;
    size_t totalLength() const;
    
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
    
    // Utility
    void clear();
    size_t getMemoryUsage() const;

private:
    enum class BufferType { Original, Add };
    
    struct Piece {
        BufferType buffer;
        size_t start;     // Start offset in buffer
        size_t length;    // Length of piece
        size_t lineCount; // Cached line count
        
        Piece() : buffer(BufferType::Original), start(0), length(0), lineCount(0) {}
        Piece(BufferType b, size_t s, size_t len) 
            : buffer(b), start(s), length(len), lineCount(0) {}
    };
    
    std::string original_;      // Original text (immutable)
    std::string added_;         // Added text (append-only)
    std::vector<Piece> pieces_; // Piece table
    
    struct EditState {
        std::vector<Piece> pieces;
        std::string added;
        size_t totalLength;
        size_t totalLines;
    };
    
    std::vector<EditState> undoStack_;
    std::vector<EditState> redoStack_;
};

} // namespace NexusForge::Editor
