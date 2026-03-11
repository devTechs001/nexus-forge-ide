// editor/buffer/rope_buffer.hpp
#pragma once

#include <string>
#include <memory>
#include <vector>

namespace NexusForge::Editor {

// Rope data structure for efficient large file editing
class RopeBuffer {
public:
    RopeBuffer();
    explicit RopeBuffer(const std::string& text);
    ~RopeBuffer();

    // Basic operations
    void insert(size_t position, const std::string& text);
    void remove(size_t position, size_t length);
    char at(size_t position) const;
    
    // Text retrieval
    std::string substr(size_t start, size_t length) const;
    std::string toString() const;
    
    // Document info
    size_t length() const { return length_; }
    bool empty() const { return length_ == 0; }
    
    // Line operations
    size_t getLineCount() const;
    std::string getLine(size_t lineNum) const;
    size_t lineToOffset(size_t line, size_t column) const;
    std::pair<size_t, size_t> offsetToLine(size_t offset) const;
    
    // Undo support
    void saveState();
    void undo();
    void redo();
    bool canUndo() const { return !undoStack_.empty(); }
    bool canRedo() const { return !redoStack_.empty(); }

private:
    struct Node {
        std::string text;
        size_t length;
        size_t depth;
        std::shared_ptr<Node> left;
        std::shared_ptr<Node> right;
        
        Node(const std::string& t) : text(t), length(t.size()), depth(0) {}
        Node(std::shared_ptr<Node> l, std::shared_ptr<Node> r);
        
        size_t totalLength() const;
    };
    
    std::shared_ptr<Node> root_;
    size_t length_ = 0;
    
    static constexpr size_t MAX_LEAF_SIZE = 512;
    
    std::vector<std::shared_ptr<Node>> undoStack_;
    std::vector<std::shared_ptr<Node>> redoStack_;
    
    std::shared_ptr<Node> concat(std::shared_ptr<Node> left, std::shared_ptr<Node> right);
    std::shared_ptr<Node> insert(std::shared_ptr<Node> node, size_t pos, const std::string& text);
    std::shared_ptr<Node> remove(std::shared_ptr<Node> node, size_t pos, size_t len);
    char at(std::shared_ptr<Node> node, size_t pos) const;
    void collectText(std::shared_ptr<Node> node, std::string& result) const;
    size_t calculateLength(std::shared_ptr<Node> node) const;
};

} // namespace NexusForge::Editor
