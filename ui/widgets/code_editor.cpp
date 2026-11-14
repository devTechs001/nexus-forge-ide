// ui/widgets/code_editor.cpp
#include "code_editor.hpp"
#include "../../editor/syntax/syntax_highlighter.hpp"
#include <algorithm>
#include <cctype>

namespace NexusForge::UI {

CodeEditor::CodeEditor() {
    cursors_.push_back(CursorPosition{0, 0});
    buffer_ = std::make_shared<Editor::TextBuffer>();
}
CodeEditor::~CodeEditor() = default;

void CodeEditor::openFile(const std::string& path) {
    filePath_ = path;
    if (!buffer_) {
        buffer_ = std::make_shared<Editor::TextBuffer>();
    }
    buffer_->loadFromFile(path);
    cursors_.assign(1, CursorPosition{0, 0});
    selection_ = SelectionRange{};
    scrollX_ = 0.0f;
    scrollY_ = 0.0f;
    markDirty();
}

void CodeEditor::openBuffer(std::shared_ptr<Editor::TextBuffer> buffer) {
    buffer_ = std::move(buffer);
    if (cursors_.empty()) cursors_.push_back(CursorPosition{0, 0});
    markDirty();
}

void CodeEditor::newFile(const std::string& language) {
    languageId_ = language;
    filePath_.clear();
    buffer_ = std::make_shared<Editor::TextBuffer>();
    cursors_.assign(1, CursorPosition{0, 0});
    selection_ = SelectionRange{};
    markDirty();
}

bool CodeEditor::saveFile(const std::string& path) {
    if (!buffer_) return false;
    std::string target = path.empty() ? filePath_ : path;
    if (target.empty()) return false;
    if (!buffer_->saveToFile(target)) return false;
    filePath_ = target;
    if (onSaved) onSaved();
    return true;
}

void CodeEditor::closeFile() {
    filePath_.clear();
    buffer_.reset();
    languageId_.clear();
    cursors_.assign(1, CursorPosition{0, 0});
    selection_ = SelectionRange{};
}

std::string CodeEditor::getText() const {
    if (buffer_) return buffer_->getText();
    return "";
}

std::string CodeEditor::getSelectedText() const {
    if (!buffer_ || selection_.isEmpty()) return "";

    auto min = selection_.getMin();
    auto max = selection_.getMax();
    return buffer_->getRange(min.line, min.column, max.line, max.column);
}

void CodeEditor::setText(const std::string& text) {
    if (!buffer_) buffer_ = std::make_shared<Editor::TextBuffer>();
    buffer_->setText(text);
    cursors_.assign(1, CursorPosition{0, 0});
    selection_ = SelectionRange{};
    if (onTextChanged) onTextChanged();
    markDirty();
}

void CodeEditor::insertText(const std::string& text) {
    if (!buffer_) buffer_ = std::make_shared<Editor::TextBuffer>();
    if (cursors_.empty()) cursors_.push_back(CursorPosition{0, 0});

    if (!selection_.isEmpty()) {
        deleteSelection();
    }

    auto& cursor = cursors_[0];
    buffer_->insert(cursor.line, cursor.column, text);
    for (char c : text) {
        if (c == '\n') {
            cursor.line++;
            cursor.column = 0;
        } else {
            cursor.column++;
        }
    }
    cursor_ = cursor;
    if (onTextChanged) onTextChanged();
    if (onCursorChanged) onCursorChanged(cursor);
    markDirty();
}

void CodeEditor::deleteSelection() {
    if (!buffer_ || selection_.isEmpty()) return;
    if (cursors_.empty()) cursors_.push_back(CursorPosition{0, 0});

    auto min = selection_.getMin();
    auto max = selection_.getMax();
    buffer_->removeRange(min.line, min.column, max.line, max.column);
    cursors_[0] = min;
    cursor_ = min;
    selection_ = SelectionRange{min, min};
    if (onTextChanged) onTextChanged();
    if (onCursorChanged) onCursorChanged(min);
    markDirty();
}

std::string CodeEditor::getLine(size_t lineNum) const {
    if (buffer_) return buffer_->getLine(lineNum);
    return "";
}

size_t CodeEditor::getLineCount() const {
    if (buffer_) return buffer_->getLineCount();
    return 0;
}

void CodeEditor::insertLine(size_t lineNum, const std::string& text) {
    if (buffer_) {
        buffer_->insertLine(lineNum, text);
    }
}

void CodeEditor::deleteLine(size_t lineNum) {
    if (buffer_) {
        buffer_->deleteLine(lineNum);
    }
}

void CodeEditor::duplicateLine() {
    // Duplicate current line
}

void CodeEditor::moveLinesUp() {
    // Move selected lines up
}

void CodeEditor::moveLinesDown() {
    // Move selected lines down
}

CursorPosition CodeEditor::getCursorPosition() const {
    if (!cursors_.empty()) return cursors_[0];
    return CursorPosition{};
}

void CodeEditor::setCursorPosition(const CursorPosition& pos) {
    if (cursors_.empty()) cursors_.push_back(pos);
    else cursors_[0] = pos;
    cursor_ = pos;
    if (onCursorChanged) onCursorChanged(pos);
}

void CodeEditor::setCursorPosition(size_t line, size_t column) {
    setCursorPosition(CursorPosition{line, column});
}

void CodeEditor::moveCursor(int lineDelta, int columnDelta) {
    if (cursors_.empty()) cursors_.push_back(CursorPosition{0, 0});
    auto& cursor = cursors_[0];
    int newLine = std::max(0, static_cast<int>(cursor.line) + lineDelta);
    int newCol = std::max(0, static_cast<int>(cursor.column) + columnDelta);
    if (buffer_ && buffer_->getLineCount() > 0) {
        newLine = std::min(newLine, static_cast<int>(buffer_->getLineCount() - 1));
        newCol = std::min(newCol, static_cast<int>(buffer_->getLineLength(static_cast<size_t>(newLine))));
    }
    cursor.line = static_cast<size_t>(newLine);
    cursor.column = static_cast<size_t>(newCol);
    cursor_ = cursor;
    if (onCursorChanged) onCursorChanged(cursor);
}

void CodeEditor::moveCursorToLineEnd() {
    if (!buffer_ || cursors_.empty()) return;

    auto& cursor = cursors_[0];
    size_t lineLength = buffer_->getLineLength(cursor.line);
    cursor.column = lineLength;
    cursor_ = cursor;
}

void CodeEditor::moveCursorToDocumentStart() {
    if (cursors_.empty()) cursors_.push_back(CursorPosition{0, 0});
    cursors_[0] = CursorPosition{0, 0};
    cursor_ = cursors_[0];
}

void CodeEditor::moveCursorToDocumentEnd() {
    if (!buffer_) return;
    if (cursors_.empty()) cursors_.push_back(CursorPosition{0, 0});
    size_t lineCount = buffer_->getLineCount();
    if (lineCount == 0) {
        cursors_[0] = CursorPosition{0, 0};
    } else {
        size_t lastLine = lineCount - 1;
        cursors_[0] = CursorPosition{lastLine, buffer_->getLineLength(lastLine)};
    }
    cursor_ = cursors_[0];
}

void CodeEditor::moveCursorWordLeft() {
    // Move cursor to previous word
}

void CodeEditor::moveCursorWordRight() {
    // Move cursor to next word
}

void CodeEditor::addCursor(const CursorPosition& pos) {
    cursors_.push_back(pos);
}

void CodeEditor::addCursorAbove() {
    if (cursors_.empty()) return;

    CursorPosition pos = cursors_.back();
    if (pos.line > 0) {
        pos.line--;
        addCursor(pos);
    }
}

void CodeEditor::addCursorBelow() {
    if (cursors_.empty()) return;

    CursorPosition pos = cursors_.back();
    pos.line++;
    addCursor(pos);
}

void CodeEditor::addCursorsToLineEnds() {
    // Add cursor at end of each selected line
}

void CodeEditor::clearSecondaryCursors() {
    if (cursors_.size() > 1) {
        cursors_.erase(cursors_.begin() + 1, cursors_.end());
    }
}

SelectionRange CodeEditor::getSelection() const {
    return selection_;
}

void CodeEditor::setSelection(const SelectionRange& range) {
    selection_ = range;
}

void CodeEditor::selectAll() {
    if (!buffer_) return;
    size_t lineCount = buffer_->getLineCount();
    if (lineCount == 0) {
        setSelection(SelectionRange{});
        return;
    }

    size_t lastLine = lineCount - 1;
    SelectionRange range;
    range.start = CursorPosition{0, 0};
    range.end = CursorPosition{lastLine, buffer_->getLineLength(lastLine)};
    setSelection(range);
}

void CodeEditor::selectLine(size_t lineNum) {
    SelectionRange range;
    range.start = CursorPosition{lineNum, 0};
    range.end = CursorPosition{lineNum, buffer_ ? buffer_->getLineLength(lineNum) : 0};
    setSelection(range);
}

void CodeEditor::selectWord() {
    // Select word at cursor
}

void CodeEditor::selectBracketContents() {
    // Select contents of brackets at cursor
}

void CodeEditor::expandSelection() {
    // Expand current selection
}

void CodeEditor::shrinkSelection() {
    // Shrink current selection
}

std::vector<SelectionRange> CodeEditor::findAll(const std::string& pattern, bool caseSensitive) {
    FindOptions opts;
    opts.caseSensitive = caseSensitive;
    return findAll(pattern, opts);
}

std::vector<SelectionRange> CodeEditor::findAll(
    const std::string& pattern, const FindOptions& options) {

    std::vector<SelectionRange> results;
    if (!buffer_ || pattern.empty()) return results;

    std::string text = buffer_->getText();
    std::string searchHaystack = text;
    std::string searchNeedle = pattern;

    if (!options.caseSensitive) {
        auto toLower = [](unsigned char c) { return static_cast<char>(std::tolower(c)); };
        std::transform(searchHaystack.begin(), searchHaystack.end(), searchHaystack.begin(), toLower);
        std::transform(searchNeedle.begin(), searchNeedle.end(), searchNeedle.begin(), toLower);
    }

    size_t pos = 0;
    size_t scanPos = 0;
    size_t line = 0;
    size_t column = 0;

    auto advanceTo = [&](size_t target) {
        while (scanPos < target && scanPos < text.size()) {
            if (text[scanPos] == '\n') {
                line++;
                column = 0;
            } else {
                column++;
            }
            scanPos++;
        }
    };

    while ((pos = searchHaystack.find(searchNeedle, pos)) != std::string::npos) {
        advanceTo(pos);
        CursorPosition start{line, column};
        advanceTo(pos + pattern.length());
        CursorPosition end{line, column};
        results.push_back(SelectionRange{start, end});
        pos += pattern.length();
    }

    return results;
}

bool CodeEditor::findNext(const std::string& pattern) {
    return findNext(pattern, FindOptions{});
}

bool CodeEditor::findNext(const std::string& pattern, const FindOptions& options) {
    auto matches = findAll(pattern, options);
    if (matches.empty()) return false;
    CursorPosition cur = getCursorPosition();
    for (const auto& m : matches) {
        if (m.start.line > cur.line || (m.start.line == cur.line && m.start.column > cur.column)) {
            setSelection(m);
            setCursorPosition(m.end);
            return true;
        }
    }
    setSelection(matches.front());
    setCursorPosition(matches.front().end);
    return true;
}

bool CodeEditor::findPrevious(const std::string& pattern, const FindOptions& options) {
    auto matches = findAll(pattern, options);
    if (matches.empty()) return false;
    CursorPosition cur = getCursorPosition();
    for (auto it = matches.rbegin(); it != matches.rend(); ++it) {
        if (it->start.line < cur.line || (it->start.line == cur.line && it->start.column < cur.column)) {
            setSelection(*it);
            setCursorPosition(it->start);
            return true;
        }
    }
    setSelection(matches.back());
    setCursorPosition(matches.back().start);
    return true;
}

int CodeEditor::replaceAll(const std::string& pattern, const std::string& replacement) {
    return replaceAll(pattern, replacement, FindOptions{});
}

int CodeEditor::replaceAll(const std::string& pattern, const std::string& replacement,
                            const FindOptions& options) {
    (void)options;
    if (!buffer_ || pattern.empty()) return 0;

    int count = 0;
    std::string text = buffer_->getText();
    size_t pos = 0;

    while ((pos = text.find(pattern, pos)) != std::string::npos) {
        text.replace(pos, pattern.length(), replacement);
        pos += replacement.length();
        count++;
    }

    if (count > 0) {
        buffer_->setText(text);
        if (onTextChanged) onTextChanged();
    }
    return count;
}

bool CodeEditor::replaceNext(const std::string& pattern, const std::string& replacement,
                              const FindOptions& options) {
    // Replace next occurrence
    return false;
}

void CodeEditor::scrollToLine(size_t line, bool center) {
    // Scroll to line
}

void CodeEditor::scrollToCursor() {
    // Scroll to cursor position
}

void CodeEditor::scrollBy(float deltaX, float deltaY) {
    scrollX_ = std::max(0.0f, scrollX_ + deltaX);
    scrollY_ = std::max(0.0f, scrollY_ + deltaY);
}

size_t CodeEditor::getFirstVisibleLine() const {
    if (lineHeight_ <= 0.0f || scrollY_ <= 0.0f) return 0;
    return static_cast<size_t>(scrollY_ / lineHeight_);
}

size_t CodeEditor::getLastVisibleLine() const {
    if (lineHeight_ <= 0.0f) return getFirstVisibleLine();
    return getFirstVisibleLine() + static_cast<size_t>(std::max(0.0f, bounds_.height) / lineHeight_);
}

void CodeEditor::setScrollPosition(float x, float y) {
    scrollX_ = std::max(0.0f, x);
    scrollY_ = std::max(0.0f, y);
}

void CodeEditor::setLanguage(const std::string& languageId) {
    languageId_ = languageId;
    // SyntaxHighlighter will be implemented separately
}

void CodeEditor::triggerCompletion() {
    // Trigger autocomplete
}

void CodeEditor::triggerSignatureHelp() {
    // Show signature help
}

void CodeEditor::goToDefinition() {
    // Go to definition
}

void CodeEditor::findReferences() {
    // Find references
}

void CodeEditor::rename(const std::string& newName) {
    // Rename symbol
}

void CodeEditor::formatDocument() {
    // Format entire document
}

void CodeEditor::formatSelection() {
    // Format selection
}

void CodeEditor::quickFix() {
    // Show quick fixes
}

void CodeEditor::refactor() {
    // Show refactor options
}

void CodeEditor::foldRegion(size_t startLine) {
    // Fold code region
}

void CodeEditor::unfoldRegion(size_t line) {
    // Unfold code region
}

void CodeEditor::foldAll() {
    // Fold all regions
}

void CodeEditor::unfoldAll() {
    // Unfold all regions
}

void CodeEditor::toggleFold(size_t line) {
    // Toggle fold at line
}

void CodeEditor::addGutterMarker(const GutterMarker& marker) {
    gutterMarkers_.push_back(marker);
}

void CodeEditor::removeGutterMarker(size_t line, const std::string& type) {
    gutterMarkers_.erase(
        std::remove_if(gutterMarkers_.begin(), gutterMarkers_.end(),
            [line, &type](const GutterMarker& m) {
                return m.line == line && m.type == type;
            }),
        gutterMarkers_.end()
    );
}

void CodeEditor::clearGutterMarkers() {
    gutterMarkers_.clear();
}

void CodeEditor::addDecoration(const InlineDecoration& decoration) {
    decorations_.push_back(decoration);
}

void CodeEditor::removeDecoration(const CursorPosition& pos, const std::string& type) {
    decorations_.erase(
        std::remove_if(decorations_.begin(), decorations_.end(),
            [&pos, &type](const InlineDecoration& d) {
                return d.start == pos && d.type == type;
            }),
        decorations_.end()
    );
}

void CodeEditor::clearDecorations() {
    decorations_.clear();
}

void CodeEditor::undo() {
    if (buffer_) buffer_->undo();
}

void CodeEditor::redo() {
    if (buffer_) buffer_->redo();
}

bool CodeEditor::canUndo() const {
    return buffer_ && buffer_->canUndo();
}

bool CodeEditor::canRedo() const {
    return buffer_ && buffer_->canRedo();
}

void CodeEditor::toggleLineComment() {
    // Toggle line comment
}

void CodeEditor::toggleBlockComment() {
    // Toggle block comment
}

void CodeEditor::indent() {
    // Indent selection
}

void CodeEditor::outdent() {
    // Outdent selection
}

void CodeEditor::render(RenderEngine& renderer) {
    if (!isVisibleInHierarchy()) return;

    renderer.pushTransform();
    renderer.translate(bounds_.x, bounds_.y);

    // Render background
    renderer.drawRect(Rect(0, 0, bounds_.width, bounds_.height), Color(0.1f, 0.1f, 0.12f, 1.0f));

    // Render gutter
    renderGutter(renderer);

    // Render content
    renderContent(renderer);

    // Render minimap
    if (options_.showMinimap) {
        renderMinimap(renderer);
    }

    // Render cursors
    renderCursors(renderer);

    renderer.popTransform();
}

Vec2 CodeEditor::measure(float availableWidth, float availableHeight) {
    return Vec2(availableWidth, availableHeight);
}

void CodeEditor::update(double deltaTime) {
    Widget::update(deltaTime);

    // Update cursor blink
    cursorBlinkTimer_ += deltaTime;
    if (cursorBlinkTimer_ >= options_.cursorBlinkRate / 1000.0) {
        cursorVisible_ = !cursorVisible_;
        cursorBlinkTimer_ = 0;
    }
}

bool CodeEditor::onKeyDown(const Core::KeyPressEvent& event) {
    // Handle key events
    return false;
}

bool CodeEditor::onCharInput(const Core::CharInputEvent& event) {
    // Handle character input
    return false;
}

bool CodeEditor::onMouseDown(const Core::MouseButtonPressEvent& event) {
    mouseDown_ = true;
    mouseDownPos_ = positionFromPoint(event.x, event.y);
    return false;
}

bool CodeEditor::onMouseUp(const Core::MouseButtonReleaseEvent& event) {
    mouseDown_ = false;
    return false;
}

bool CodeEditor::onMouseMove(const Core::MouseMoveEvent& event) {
    return false;
}

bool CodeEditor::onScroll(const Core::ScrollEvent& event) {
    scrollY_ -= event.yOffset * 20;
    scrollY_ = std::max(0.0f, scrollY_);
    return true;
}

bool CodeEditor::onTouchStart(const Core::TouchEvent& event) {
    return false;
}

bool CodeEditor::onTouchMove(const Core::TouchEvent& event) {
    return false;
}

bool CodeEditor::onTouchEnd(const Core::TouchEvent& event) {
    return false;
}

bool CodeEditor::onGesture(const Core::GestureEvent& event) {
    return false;
}

void CodeEditor::renderGutter(RenderEngine& renderer) {
    // Render line numbers gutter
    renderer.drawRect(Rect(0, 0, gutterWidth_, bounds_.height), Color(0.15f, 0.15f, 0.18f, 1.0f));

    // Render line numbers
    size_t firstLine = getFirstVisibleLine();
    size_t lastLine = getLastVisibleLine();

    for (size_t line = firstLine; line <= lastLine && line < getLineCount(); ++line) {
        float y = static_cast<float>(line) * lineHeight_ - scrollY_;
        std::string lineNum = std::to_string(line + 1);
        renderer.drawText(lineNum, 10, y + lineHeight_ - 4, Color(0.5f, 0.5f, 0.6f, 1.0f),
                          options_.fontFamily, options_.fontSize * 0.8f);
    }
}

void CodeEditor::renderContent(RenderEngine& renderer) {
    // Render text content
    size_t firstLine = getFirstVisibleLine();
    size_t lastLine = getLastVisibleLine();

    for (size_t line = firstLine; line <= lastLine && line < getLineCount(); ++line) {
        float y = static_cast<float>(line) * lineHeight_ - scrollY_;
        std::string lineText = getLine(line);
        renderer.drawText(lineText, gutterWidth_ + 10 - scrollX_, y + lineHeight_ - 4,
                          Color(1, 1, 1, 1), options_.fontFamily, options_.fontSize);
    }
}

void CodeEditor::renderMinimap(RenderEngine& renderer) {
    // Render minimap
    float minimapX = bounds_.width - minimapWidth_;
    renderer.drawRect(Rect(minimapX, 0, minimapWidth_, bounds_.height),
                      Color(0.08f, 0.08f, 0.1f, 1.0f));
}

void CodeEditor::renderCursors(RenderEngine& renderer) {
    if (!cursorVisible_) return;

    for (const auto& cursor : cursors_) {
        float x = gutterWidth_ + 10 + cursor.column * charWidth_ - scrollX_;
        float y = cursor.line * lineHeight_ - scrollY_;
        renderer.drawRect(Rect(x, y, 2, lineHeight_), Color(1, 1, 1, 1));
    }
}

void CodeEditor::renderSelection(RenderEngine& renderer) {
    // Render text selection
}

void CodeEditor::renderLineHighlight(RenderEngine& renderer) {
    // Render active line highlight
}

void CodeEditor::renderBracketMatching(RenderEngine& renderer) {
    // Render bracket matching highlight
}

void CodeEditor::renderIndentGuides(RenderEngine& renderer) {
    // Render indent guides
}

void CodeEditor::renderDecorations(RenderEngine& renderer) {
    // Render decorations
}

void CodeEditor::renderScrollbars(RenderEngine& renderer) {
    // Render scrollbars
}

CursorPosition CodeEditor::positionFromPoint(float x, float y) const {
    CursorPosition pos;
    float adjY = std::max(0.0f, y + scrollY_);
    float adjX = std::max(0.0f, x - gutterWidth_ - 10.0f + scrollX_);
    pos.line = (lineHeight_ > 0.0f) ? static_cast<size_t>(adjY / lineHeight_) : 0;
    pos.column = (charWidth_ > 0.0f) ? static_cast<size_t>(adjX / charWidth_) : 0;
    if (buffer_ && buffer_->getLineCount() > 0) {
        pos.line = std::min(pos.line, buffer_->getLineCount() - 1);
        pos.column = std::min(pos.column, buffer_->getLineLength(pos.line));
    }
    return pos;
}

Vec2 CodeEditor::pointFromPosition(const CursorPosition& pos) const {
    return Vec2(
        gutterWidth_ + 10 + pos.column * charWidth_ - scrollX_,
        pos.line * lineHeight_ - scrollY_
    );
}

float CodeEditor::getContentWidth() const {
    return bounds_.width - gutterWidth_ - (options_.showMinimap ? minimapWidth_ : 0);
}

float CodeEditor::getContentHeight() const {
    return getLineCount() * lineHeight_;
}

size_t CodeEditor::getVisibleLineCount() const {
    return static_cast<size_t>(bounds_.height / lineHeight_);
}

void CodeEditor::updateBracketMatching() {
    // Update bracket matching
}

void CodeEditor::updateScrollBounds() {
    // Update scroll bounds
}

void CodeEditor::handleAutoIndent() {
    // Handle auto-indent
}

void CodeEditor::handleAutoPairs(char c) {
    // Handle auto-pairing of brackets/quotes
}

void CodeEditor::ensureCursorVisible() {
    // Ensure cursor is visible
}

void CodeEditor::normalizeSelection() {
    // Normalize selection
}

void CodeEditor::mergeOverlappingSelections() {
    // Merge overlapping selections
}

} // namespace NexusForge::UI
