// editor/buffer/rope_buffer.cpp
// Rope Buffer Implementation

#include "rope_buffer.hpp"
#include <algorithm>
#include <cmath>

namespace NexusForge::Editor {

RopeBuffer::Node::Node(std::shared_ptr<Node> l, std::shared_ptr<Node> r)
    : left(std::move(l)), right(std::move(r)) {
    length = (this->left ? this->left->length : 0) + (this->right ? this->right->length : 0);
    depth = std::max(this->left ? this->left->depth : 0,
                     this->right ? this->right->depth : 0) + 1;
}

size_t RopeBuffer::Node::totalLength() const {
    return length;
}

RopeBuffer::RopeBuffer() : root_(nullptr), length_(0) {}

RopeBuffer::RopeBuffer(const std::string& text) : root_(nullptr), length_(0) {
    if (!text.empty()) {
        root_ = buildTree(text, 0, text.length());
        length_ = text.length();
    }
}

RopeBuffer::~RopeBuffer() = default;

std::shared_ptr<RopeBuffer::Node> RopeBuffer::buildTree(const std::string& text, size_t start, size_t end) {
    size_t len = end - start;
    if (len <= MAX_LEAF_SIZE) {
        return std::make_shared<Node>(text.substr(start, len));
    }
    size_t mid = start + len / 2;
    return std::make_shared<Node>(buildTree(text, start, mid), buildTree(text, mid, end));
}

void RopeBuffer::insert(size_t position, const std::string& text) {
    if (text.empty()) return;
    position = std::min(position, length_);
    root_ = insert(root_, position, text);
    length_ += text.length();
}

void RopeBuffer::remove(size_t position, size_t length) {
    if (!root_ || position >= length_ || length == 0) return;
    length = std::min(length, length_ - position);
    root_ = remove(root_, position, length);
    length_ -= length;
}

char RopeBuffer::at(size_t position) const {
    if (!root_ || position >= length_) return '\0';
    return at(root_, position);
}

std::string RopeBuffer::substr(size_t start, size_t len) const {
    if (!root_ || start >= length_) return {};
    len = std::min(len, length_ - start);
    std::string result;
    result.reserve(len);
    collectText(root_, start, len, result);
    return result;
}

std::string RopeBuffer::toString() const {
    std::string result;
    result.reserve(length_);
    collectText(root_, 0, length_, result);
    return result;
}

size_t RopeBuffer::getLineCount() const {
    size_t count = 1;
    for (size_t i = 0; i < length_; ++i) {
        if (at(i) == '\n') count++;
    }
    return count;
}

std::string RopeBuffer::getLine(size_t lineNum) const {
    size_t offset = lineToOffset(lineNum, 0);
    size_t end = offset;
    while (end < length_ && at(end) != '\n') end++;
    return substr(offset, end - offset);
}

size_t RopeBuffer::lineToOffset(size_t line, size_t column) const {
    size_t pos = 0;
    for (size_t i = 0; i < line && pos < length_; ++i) {
        while (pos < length_ && at(pos) != '\n') pos++;
        if (pos < length_) pos++;
    }
    return std::min(pos + column, length_);
}

std::pair<size_t, size_t> RopeBuffer::offsetToLine(size_t offset) const {
    size_t line = 0, col = 0;
    for (size_t i = 0; i < offset && i < length_; ++i) {
        if (at(i) == '\n') { line++; col = 0; }
        else { col++; }
    }
    return {line, col};
}

void RopeBuffer::saveState() {
    undoStack_.push_back(root_);
}

void RopeBuffer::undo() {
    if (!undoStack_.empty()) {
        redoStack_.push_back(root_);
        root_ = undoStack_.back();
        undoStack_.pop_back();
        length_ = root_ ? root_->length : 0;
    }
}

void RopeBuffer::redo() {
    if (!redoStack_.empty()) {
        undoStack_.push_back(root_);
        root_ = redoStack_.back();
        redoStack_.pop_back();
        length_ = root_ ? root_->length : 0;
    }
}

std::shared_ptr<RopeBuffer::Node> RopeBuffer::concat(std::shared_ptr<Node> left, std::shared_ptr<Node> right) {
    if (!left) return right;
    if (!right) return left;
    return std::make_shared<Node>(std::move(left), std::move(right));
}

std::shared_ptr<RopeBuffer::Node> RopeBuffer::insert(std::shared_ptr<Node> node, size_t pos, const std::string& text) {
    if (!node) return std::make_shared<Node>(text);
    if (pos >= node->length) return concat(node, std::make_shared<Node>(text));

    if (!node->left && !node->right) {
        if (node->text.length() + text.length() <= MAX_LEAF_SIZE) {
            node->text.insert(pos, text);
            node->length = node->text.length();
            return node;
        }
        auto left = std::make_shared<Node>(node->text.substr(0, pos));
        auto mid = std::make_shared<Node>(text);
        auto right = std::make_shared<Node>(node->text.substr(pos));
        return concat(concat(left, mid), right);
    }

    if (pos < node->left->length) {
        return concat(insert(node->left, pos, text), node->right);
    }
    return concat(node->left, insert(node->right, pos - node->left->length, text));
}

std::shared_ptr<RopeBuffer::Node> RopeBuffer::remove(std::shared_ptr<Node> node, size_t pos, size_t len) {
    if (!node || len == 0) return node;

    if (!node->left && !node->right) {
        node->text.erase(pos, len);
        node->length = node->text.length();
        return node->text.empty() ? nullptr : node;
    }

    size_t leftLen = node->left ? node->left->length : 0;
    if (pos + len <= leftLen) {
        return concat(remove(node->left, pos, len), node->right);
    } else if (pos >= leftLen) {
        return concat(node->left, remove(node->right, pos - leftLen, len));
    } else {
        size_t leftPart = leftLen - pos;
        return concat(remove(node->left, pos, leftPart),
                      remove(node->right, 0, len - leftPart));
    }
}

char RopeBuffer::at(std::shared_ptr<Node> node, size_t pos) const {
    if (!node) return '\0';
    if (!node->left && !node->right) {
        return pos < node->text.length() ? node->text[pos] : '\0';
    }
    size_t leftLen = node->left ? node->left->length : 0;
    if (pos < leftLen) return at(node->left, pos);
    return at(node->right, pos - leftLen);
}

void RopeBuffer::collectText(std::shared_ptr<Node> node, size_t start, size_t len, std::string& result) const {
    if (!node || len == 0) return;
    if (!node->left && !node->right) {
        result.append(node->text, start, len);
        return;
    }
    size_t leftLen = node->left ? node->left->length : 0;
    if (start < leftLen) {
        size_t chunk = std::min(len, leftLen - start);
        collectText(node->left, start, chunk, result);
        len -= chunk;
        start = 0;
    } else {
        start -= leftLen;
    }
    if (len > 0) {
        collectText(node->right, start, len, result);
    }
}

size_t RopeBuffer::calculateLength(std::shared_ptr<Node> node) const {
    return node ? node->length : 0;
}

} // namespace NexusForge::Editor
