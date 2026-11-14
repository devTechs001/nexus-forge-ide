// editor/buffer/piece_table.cpp
// Piece Table Implementation

#include "piece_table.hpp"
#include <algorithm>
#include <cstring>

namespace NexusForge::Editor {

PieceTable::PieceTable() {
    original_ = "";
    added_.reserve(4096);
}

PieceTable::PieceTable(const std::string& initialText) {
    original_ = initialText;
    added_.reserve(4096);
    if (!initialText.empty()) {
        pieces_.emplace_back(BufferType::Original, 0, initialText.length());
    }
}

PieceTable::~PieceTable() = default;

void PieceTable::replace(size_t position, size_t length, const std::string& text) {
    remove(position, length);
    insert(position, text);
}

void PieceTable::setText(const std::string& text) {
    pieces_.clear();
    original_ = text;
    added_.clear();
    
    if (!text.empty()) {
        Piece piece;
        piece.buffer = BufferType::Original;
        piece.start = 0;
        piece.length = text.length();
        pieces_.push_back(piece);
    }
}

std::string PieceTable::getText() const {
    std::string result;
    result.reserve(totalLength());
    
    for (const auto& piece : pieces_) {
        if (piece.buffer == BufferType::Original) {
            result += original_.substr(piece.start, piece.length);
        } else {
            result += added_.substr(piece.start, piece.length);
        }
    }
    
    return result;
}

void PieceTable::insert(size_t pos, const std::string& text) {
    if (text.empty()) return;
    
    // Add text to add buffer
    size_t addStart = added_.length();
    added_ += text;
    
    // Find the piece to split
    size_t currentPos = 0;
    size_t pieceIndex = 0;
    
    for (; pieceIndex < pieces_.size(); ++pieceIndex) {
        if (currentPos + pieces_[pieceIndex].length >= pos) {
            break;
        }
        currentPos += pieces_[pieceIndex].length;
    }
    
    if (pieceIndex >= pieces_.size()) {
        // Append at end
        Piece piece;
        piece.buffer = BufferType::Add;
        piece.start = addStart;
        piece.length = text.length();
        pieces_.push_back(piece);
    } else {
        Piece& currentPiece = pieces_[pieceIndex];
        size_t offsetInPiece = pos - currentPos;
        
        if (offsetInPiece == 0) {
            // Insert before this piece
            Piece newPiece;
            newPiece.buffer = BufferType::Add;
            newPiece.start = addStart;
            newPiece.length = text.length();
            pieces_.insert(pieces_.begin() + pieceIndex, newPiece);
        } else if (offsetInPiece == currentPiece.length) {
            // Insert after this piece
            Piece newPiece;
            newPiece.buffer = BufferType::Add;
            newPiece.start = addStart;
            newPiece.length = text.length();
            pieces_.insert(pieces_.begin() + pieceIndex + 1, newPiece);
        } else {
            // Split the piece
            Piece orig = currentPiece;
            Piece newPiece;
            newPiece.buffer = BufferType::Add;
            newPiece.start = addStart;
            newPiece.length = text.length();

            // Modify current piece to be the first part
            currentPiece.length = offsetInPiece;

            // Create piece for the second part using saved original length
            Piece secondPart;
            secondPart.buffer = orig.buffer;
            secondPart.start = orig.start + offsetInPiece;
            secondPart.length = orig.length - offsetInPiece;

            pieces_.insert(pieces_.begin() + pieceIndex + 1, newPiece);
            pieces_.insert(pieces_.begin() + pieceIndex + 2, secondPart);
        }
    }
}

void PieceTable::remove(size_t position, size_t length) {
    size_t total = totalLength();
    if (length == 0 || position >= total || pieces_.empty()) return;
    size_t start = position;
    size_t end = std::min(position + length, total);
    if (start >= end) return;

    size_t currentPos = 0;
    size_t startIndex = 0, endIndex = pieces_.size() - 1;
    size_t startOffset = 0, endOffset = 0;

    // Find start piece
    for (size_t i = 0; i < pieces_.size(); ++i) {
        if (currentPos + pieces_[i].length > start) {
            startIndex = i;
            startOffset = start - currentPos;
            break;
        }
        currentPos += pieces_[i].length;
    }

    // Find end piece (restart from 0)
    currentPos = 0;
    for (size_t i = 0; i < pieces_.size(); ++i) {
        if (currentPos + pieces_[i].length >= end) {
            endIndex = i;
            endOffset = end - currentPos;
            break;
        }
        currentPos += pieces_[i].length;
    }

    if (startIndex == endIndex) {
        // Removal within single piece
        Piece& piece = pieces_[startIndex];

        if (startOffset == 0 && endOffset >= piece.length) {
            pieces_.erase(pieces_.begin() + startIndex);
        } else if (startOffset == 0) {
            piece.start += endOffset;
            piece.length -= endOffset;
        } else if (endOffset >= piece.length) {
            piece.length = startOffset;
        } else {
            Piece secondPart;
            secondPart.buffer = piece.buffer;
            secondPart.start = piece.start + endOffset;
            secondPart.length = piece.length - endOffset;

            piece.length = startOffset;
            pieces_.insert(pieces_.begin() + startIndex + 1, secondPart);
        }
    } else {
        // Removal spans multiple pieces — adjust end first so indices don't shift
        if (endOffset < pieces_[endIndex].length) {
            pieces_[endIndex].start += endOffset;
            pieces_[endIndex].length -= endOffset;
        } else {
            pieces_[endIndex].length = 0;
        }

        // Remove pieces strictly between startIndex and endIndex
        if (startIndex + 1 < endIndex) {
            pieces_.erase(pieces_.begin() + startIndex + 1, pieces_.begin() + endIndex);
        }

        // Adjust start piece
        if (startOffset > 0) {
            pieces_[startIndex].length = startOffset;
        } else {
            pieces_[startIndex].length = 0;
        }
    }

    // Remove empty pieces
    pieces_.erase(
        std::remove_if(pieces_.begin(), pieces_.end(),
            [](const Piece& p) { return p.length == 0; }),
        pieces_.end()
    );
}

std::string PieceTable::substr(size_t start, size_t len) const {
    std::string result;
    size_t currentPos = 0;
    
    for (const auto& piece : pieces_) {
        if (currentPos + piece.length <= start) {
            currentPos += piece.length;
            continue;
        }
        
        if (currentPos >= start + len) break;
        
        size_t pieceStart = std::max(start, currentPos) - currentPos;
        size_t pieceEnd = std::min(start + len, currentPos + piece.length) - currentPos;
        size_t pieceLen = pieceEnd - pieceStart;
        
        std::string pieceText;
        if (piece.buffer == BufferType::Original) {
            pieceText = original_.substr(piece.start + pieceStart, pieceLen);
        } else {
            pieceText = added_.substr(piece.start + pieceStart, pieceLen);
        }
        
        result += pieceText;
        currentPos += piece.length;
    }
    
    return result;
}

char PieceTable::at(size_t position) const {
    size_t currentPos = 0;
    for (const auto& piece : pieces_) {
        if (position < currentPos + piece.length) {
            size_t offset = position - currentPos;
            if (piece.buffer == BufferType::Original) {
                return original_[piece.start + offset];
            }
            return added_[piece.start + offset];
        }
        currentPos += piece.length;
    }
    return '\0';
}

size_t PieceTable::totalLength() const {
    size_t len = 0;
    for (const auto& piece : pieces_) {
        len += piece.length;
    }
    return len;
}

size_t PieceTable::lineCount() const {
    std::string text = getText();
    size_t count = 1;
    
    for (char c : text) {
        if (c == '\n') count++;
    }
    
    return count;
}

void PieceTable::clear() {
    pieces_.clear();
    original_.clear();
    added_.clear();
}

size_t PieceTable::lineToOffset(size_t line, size_t column) const {
    std::string text = getText();
    size_t pos = 0;
    for (size_t i = 0; i < line && pos < text.length(); ++i) {
        while (pos < text.length() && text[pos] != '\n') pos++;
        if (pos < text.length()) pos++;
    }
    return std::min(pos + column, text.length());
}

std::pair<size_t, size_t> PieceTable::offsetToLine(size_t offset) const {
    std::string text = getText();
    size_t line = 0, col = 0;
    for (size_t i = 0; i < offset && i < text.length(); ++i) {
        if (text[i] == '\n') { line++; col = 0; }
        else { col++; }
    }
    return {line, col};
}

std::string PieceTable::getLine(size_t lineNum) const {
    size_t start = lineToOffset(lineNum, 0);
    std::string text = getText();
    size_t end = start;
    while (end < text.length() && text[end] != '\n') end++;
    return text.substr(start, end - start);
}

size_t PieceTable::getMemoryUsage() const {
    return sizeof(PieceTable) + original_.capacity() + added_.capacity();
}

void PieceTable::beginUndo() {
    EditState state;
    state.pieces = pieces_;
    state.added = added_;
    state.totalLength = totalLength();
    state.totalLines = lineCount();
    undoStack_.push_back(std::move(state));
    redoStack_.clear();
}

void PieceTable::endUndo() {
    // Snapshot boundary already recorded by beginUndo()
}

void PieceTable::undo() {
    if (undoStack_.empty()) return;
    EditState current;
    current.pieces = pieces_;
    current.added = added_;
    current.totalLength = totalLength();
    current.totalLines = lineCount();
    redoStack_.push_back(std::move(current));

    EditState prev = std::move(undoStack_.back());
    undoStack_.pop_back();
    pieces_ = std::move(prev.pieces);
    added_ = std::move(prev.added);
}

void PieceTable::redo() {
    if (redoStack_.empty()) return;
    EditState current;
    current.pieces = pieces_;
    current.added = added_;
    current.totalLength = totalLength();
    current.totalLines = lineCount();
    undoStack_.push_back(std::move(current));

    EditState next = std::move(redoStack_.back());
    redoStack_.pop_back();
    pieces_ = std::move(next.pieces);
    added_ = std::move(next.added);
}

bool PieceTable::canUndo() const {
    return !undoStack_.empty();
}

bool PieceTable::canRedo() const {
    return !redoStack_.empty();
}

} // namespace NexusForge::Editor
