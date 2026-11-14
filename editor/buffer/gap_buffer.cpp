// editor/buffer/gap_buffer.cpp
// Gap Buffer Implementation

#include "gap_buffer.hpp"
#include <algorithm>
#include <cstring>

namespace NexusForge::Editor {

GapBuffer::GapBuffer(size_t initialCapacity)
    : gapStart_(0), gapEnd_(0), length_(0) {
    if (initialCapacity > 0) {
        buffer_.resize(initialCapacity);
        gapEnd_ = initialCapacity;
    }
}

void GapBuffer::insert(char c) {
    ensureGapSize(1);
    buffer_[gapStart_++] = c;
    length_++;
}

void GapBuffer::insert(const std::string& text) {
    ensureGapSize(text.length());
    for (char c : text) {
        buffer_[gapStart_++] = c;
    }
    length_ += text.length();
}

void GapBuffer::insert(size_t pos, char c) {
    moveGap(pos);
    insert(c);
}

void GapBuffer::insert(size_t pos, const std::string& text) {
    moveGap(pos);
    insert(text);
}

void GapBuffer::remove() {
    if (gapEnd_ < buffer_.size()) {
        gapEnd_++;
        length_--;
    }
}

char GapBuffer::remove(size_t pos) {
    if (pos >= length()) return '\0';
    moveGap(pos);
    char removed = buffer_[gapEnd_];
    gapEnd_++;
    length_--;
    return removed;
}

std::string GapBuffer::remove(size_t pos, size_t len) {
    if (pos >= length() || len == 0) return {};
    len = std::min(len, length() - pos);
    std::string result = getText(pos, pos + len);
    moveGap(pos);
    gapEnd_ += len;
    length_ -= len;
    return result;
}

void GapBuffer::removeRange(size_t start, size_t end) {
    if (start < end && end <= length()) {
        remove(start, end - start);
    }
}

char GapBuffer::at(size_t position) const {
    if (position >= length()) return '\0';
    if (position < gapStart_) {
        return buffer_[position];
    }
    return buffer_[position + (gapEnd_ - gapStart_)];
}

void GapBuffer::move(size_t position) {
    if (position > length()) position = length();
    moveGap(position);
}

void GapBuffer::moveLeft() {
    if (gapStart_ > 0) {
        moveGap(gapStart_ - 1);
    }
}

void GapBuffer::moveRight() {
    if (gapStart_ < length()) {
        moveGap(gapStart_ + 1);
    }
}

void GapBuffer::moveLineUp() {
    auto [line, col] = offsetToPosition(gapStart_);
    if (line > 0) {
        col = std::min(col, getLineLength(line - 1));
        move(positionToOffset(line - 1, col));
    }
}

void GapBuffer::moveLineDown() {
    auto [line, col] = offsetToPosition(gapStart_);
    if (line + 1 < getLineCount()) {
        col = std::min(col, getLineLength(line + 1));
        move(positionToOffset(line + 1, col));
    }
}

std::string GapBuffer::getText() const {
    return getText(0, length());
}

std::string GapBuffer::getText(size_t start, size_t end) const {
    if (start >= end || start >= length()) return {};
    size_t len = std::min(end, length()) - start;
    std::string result;
    result.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        result += at(start + i);
    }
    return result;
}

std::string GapBuffer::getLine(size_t lineNum) const {
    if (lineNum >= getLineCount()) return {};
    size_t start = lineStart(lineNum);
    size_t end = lineEnd(lineNum);
    return getText(start, end);
}

size_t GapBuffer::getLineCount() const {
    size_t count = 1;
    for (size_t i = 0; i < length(); ++i) {
        if (at(i) == '\n') count++;
    }
    return count;
}

size_t GapBuffer::getLineLength(size_t lineNum) const {
    if (lineNum >= getLineCount()) return 0;
    size_t start = lineStart(lineNum);
    size_t end = lineEnd(lineNum);
    return end - start;
}

size_t GapBuffer::getLineOffset(size_t lineNum) const {
    return lineStart(lineNum);
}

size_t GapBuffer::positionToOffset(size_t line, size_t column) const {
    size_t pos = 0;
    for (size_t i = 0; i < line && pos < length(); ++i) {
        while (pos < length() && at(pos) != '\n') pos++;
        if (pos < length()) pos++;
    }
    return std::min(pos + column, length());
}

std::pair<size_t, size_t> GapBuffer::offsetToPosition(size_t offset) const {
    size_t line = 0;
    size_t col = 0;
    for (size_t i = 0; i < offset && i < length(); ++i) {
        if (at(i) == '\n') {
            line++;
            col = 0;
        } else {
            col++;
        }
    }
    return {line, col};
}

size_t GapBuffer::lineStart(size_t lineNum) const {
    return positionToOffset(lineNum, 0);
}

size_t GapBuffer::lineEnd(size_t lineNum) const {
    size_t pos = positionToOffset(lineNum, 0);
    while (pos < length() && at(pos) != '\n') pos++;
    return pos;
}

void GapBuffer::setCursorPosition(size_t position) {
    move(position);
}

void GapBuffer::saveState() {
    undoStack_.push_back({gapStart_, gapEnd_, length_, lineCount_, buffer_});
}

void GapBuffer::restoreState() {
    if (!undoStack_.empty()) {
        State s = undoStack_.back();
        undoStack_.pop_back();
        gapStart_ = s.gapStart;
        gapEnd_ = s.gapEnd;
        length_ = s.length;
        lineCount_ = s.lineCount;
        buffer_ = std::move(s.buffer);
    }
}

void GapBuffer::clear() {
    buffer_.clear();
    gapStart_ = 0;
    gapEnd_ = 0;
    length_ = 0;
    lineCount_ = 1;
}

void GapBuffer::ensureGapSize(size_t needed) {
    size_t gapSize = gapEnd_ - gapStart_;
    if (gapSize >= needed) return;
    size_t neededCapacity = buffer_.size() + needed - gapSize;
    grow(neededCapacity);
}

void GapBuffer::grow(size_t minCapacity) {
    size_t newCapacity = std::max(buffer_.size() * 2, minCapacity);
    reserve(newCapacity);
}

void GapBuffer::reserve(size_t newCapacity) {
    if (newCapacity <= buffer_.size()) return;
    std::vector<char> newBuffer(newCapacity);
    size_t afterGap = buffer_.size() - gapEnd_;
    size_t newGapEnd = newCapacity - afterGap;
    std::copy(buffer_.begin(), buffer_.begin() + gapStart_, newBuffer.begin());
    std::copy(buffer_.begin() + gapEnd_, buffer_.end(), newBuffer.begin() + newGapEnd);
    gapEnd_ = newGapEnd;
    buffer_ = std::move(newBuffer);
}

void GapBuffer::shrinkToFit() {
    size_t actualSize = length();
    if (actualSize < buffer_.size()) {
        std::vector<char> newBuffer(actualSize);
        std::copy(buffer_.begin(), buffer_.begin() + gapStart_, newBuffer.begin());
        std::copy(buffer_.begin() + gapEnd_, buffer_.end(), newBuffer.begin() + gapStart_);
        buffer_ = std::move(newBuffer);
        gapEnd_ = gapStart_;
    }
}

void GapBuffer::moveGap(size_t newPosition) {
    if (newPosition == gapStart_) return;
    if (newPosition > length()) newPosition = length();

    if (newPosition < gapStart_) {
        size_t count = gapStart_ - newPosition;
        std::memmove(buffer_.data() + gapEnd_ - count,
                     buffer_.data() + newPosition,
                     count);
        gapStart_ = newPosition;
        gapEnd_ -= count;
    } else {
        size_t count = newPosition - gapStart_;
        std::memmove(buffer_.data() + gapStart_,
                     buffer_.data() + gapEnd_,
                     count);
        gapStart_ += count;
        gapEnd_ += count;
    }
}

void GapBuffer::optimize() {
    moveGap(length());
}

int GapBuffer::compare(const GapBuffer& other) const {
    size_t minSize = std::min(length(), other.length());
    for (size_t i = 0; i < minSize; ++i) {
        unsigned char a = static_cast<unsigned char>(at(i));
        unsigned char b = static_cast<unsigned char>(other.at(i));
        if (a != b) {
            return (a < b) ? -1 : 1;
        }
    }
    if (length() < other.length()) return -1;
    if (length() > other.length()) return 1;
    return 0;
}

bool GapBuffer::operator==(const GapBuffer& other) const {
    if (length() != other.length()) return false;
    for (size_t i = 0; i < length(); ++i) {
        if (at(i) != other.at(i)) return false;
    }
    return true;
}

bool GapBuffer::operator!=(const GapBuffer& other) const {
    return !(*this == other);
}

bool GapBuffer::operator<(const GapBuffer& other) const {
    return compare(other) < 0;
}

} // namespace NexusForge::Editor
