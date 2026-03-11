// ui/widgets/code_editor.cpp
#include "code_editor.hpp"

namespace NexusForge::UI {

CodeEditor::CodeEditor() = default;

CodeEditor::~CodeEditor() = default;

void CodeEditor::openFile(const std::string& path) {
    filePath_ = path;
    // Load file content
}

void CodeEditor::openBuffer(std::shared_ptr<Editor::TextBuffer> buffer) {
    buffer_ = buffer;
}

void CodeEditor::newFile(const std::string& language) {
    languageId_ = language;
    buffer_ = std::make_shared<Editor::TextBuffer>();
}

bool CodeEditor::saveFile(const std::string& path) {
    // Save file implementation
    return true;
}

void CodeEditor::closeFile() {
    filePath_.clear();
    buffer_.reset();
    languageId_.clear();
}

std::string CodeEditor::getText() const {
    if (buffer_) return buffer_->getText();
    return "";
}

std::string CodeEditor::getSelectedText() const {
    if (!buffer_ || cursors_.empty()) return "";
    
    auto& cursor = cursors_[0];
    if (cursor.selection.isEmpty()) {
        return "";
    }
    
    auto min = cursor.selection.getMin();
    auto max = cursor.selection.getMax();
    return buffer_->getTextRange(min.line, min.column, max.line, max.column);
}

void CodeEditor::setText(const std::string& text) {
    if (buffer_) {
        buffer_->setText(text);
    }
}

void CodeEditor::insertText(const std::string& text) {
    if (buffer_ && !cursors_.empty()) {
        auto& cursor = cursors_[0];
        buffer_->insert(cursor.position.line, cursor.position.column, text);
    }
}

void CodeEditor::deleteSelection() {
    if (buffer_ && !cursors_.empty()) {
        auto& cursor = cursors_[0];
        if (!cursor.selection.isEmpty()) {
            auto min = cursor.selection.getMin();
            auto max = cursor.selection.getMax();
            buffer_->removeRange(min.line, min.column, max.line, max.column);
        }
    }
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
        buffer_->removeLine(lineNum);
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
    if (!cursors_.empty()) return cursors_[0].position;
    return CursorPosition{};
}

void CodeEditor::setCursorPosition(const CursorPosition& pos) {
    if (!cursors_.empty()) {
        cursors_[0].position = pos;
    }
}

void CodeEditor::setCursorPosition(size_t line, size_t column) {
    setCursorPosition(CursorPosition{line, column});
}

void CodeEditor::moveCursor(int lineDelta, int columnDelta) {
    if (!cursors_.empty()) {
        auto& cursor = cursors_[0];
        cursor.position.line = std::max(0, static_cast<int>(cursor.position.line) + lineDelta);
        cursor.position.column = std::max(0, static_cast<int>(cursor.position.column) + columnDelta);
    }
}

void CodeEditor::moveCursorToLineEnd() {
    if (!buffer_ || cursors_.empty()) return;
    
    auto& cursor = cursors_[0];
    size_t lineLength = buffer_->getLineLength(cursor.position.line);
    cursor.position.column = lineLength;
}

void CodeEditor::moveCursorToDocumentStart() {
    if (cursors_.empty()) return;
    cursors_[0].position = CursorPosition{0, 0};
}

void CodeEditor::moveCursorToDocumentEnd() {
    if (!buffer_ || cursors_.empty()) return;
    cursors_[0].position = CursorPosition{buffer_->getLineCount() - 1, 0};
}

void CodeEditor::moveCursorWordLeft() {
    // Move cursor to previous word
}

void CodeEditor::moveCursorWordRight() {
    // Move cursor to next word
}

void CodeEditor::addCursor(const CursorPosition& pos) {
    CursorInfo cursor;
    cursor.position = pos;
    cursors_.push_back(cursor);
}

void CodeEditor::addCursorAbove() {
    if (cursors_.empty()) return;
    
    CursorPosition pos = cursors_.back().position;
    if (pos.line > 0) {
        pos.line--;
        addCursor(pos);
    }
}

void CodeEditor::addCursorBelow() {
    if (cursors_.empty()) return;
    
    CursorPosition pos = cursors_.back().position;
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
    if (!cursors_.empty()) return cursors_[0].selection;
    return SelectionRange{};
}

void CodeEditor::setSelection(const SelectionRange& range) {
    if (!cursors_.empty()) {
        cursors_[0].selection = range;
    }
}

void CodeEditor::selectAll() {
    if (!buffer_) return;
    
    SelectionRange range;
    range.start = CursorPosition{0, 0};
    range.end = CursorPosition{buffer_->getLineCount() - 1, 0};
    setSelection(range);
}

void CodeEditor::selectLine(size_t lineNum) {
    SelectionRange range;
    range.start = CursorPosition{lineNum, 0};
    range.end = CursorPosition{lineNum + 1, 0};
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

std::vector<SelectionRange> CodeEditor::findAll(
    const std::string& pattern, const FindOptions& options) {
    
    std::vector<SelectionRange> results;
    if (!buffer_) return results;
    
    // Find all occurrences of pattern
    std::string text = buffer_->getText();
    size_t pos = 0;
    size_t line = 0, column = 0;
    
    while ((pos = text.find(pattern, pos)) != std::string::npos) {
        // Calculate line and column
        SelectionRange range;
        range.start = CursorPosition{line, column};
        range.end = CursorPosition{line, column + pattern.length()};
        results.push_back(range);
        
        pos += pattern.length();
        column += pattern.length();
    }
    
    return results;
}

bool CodeEditor::findNext(const std::string& pattern, const FindOptions& options) {
    // Find next occurrence
    return false;
}

bool CodeEditor::findPrevious(const std::string& pattern, const FindOptions& options) {
    // Find previous occurrence
    return false;
}

int CodeEditor::replaceAll(const std::string& pattern, const std::string& replacement,
                            const FindOptions& options) {
    if (!buffer_) return 0;
    
    int count = 0;
    std::string text = buffer_->getText();
    size_t pos = 0;
    
    while ((pos = text.find(pattern, pos)) != std::string::npos) {
        text.replace(pos, pattern.length(), replacement);
        pos += replacement.length();
        count++;
    }
    
    buffer_->setText(text);
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
    scrollX_ += deltaX;
    scrollY_ += deltaY;
}

size_t CodeEditor::getFirstVisibleLine() const {
    return static_cast<size_t>(scrollY_ / lineHeight_);
}

size_t CodeEditor::getLastVisibleLine() const {
    return getFirstVisibleLine() + static_cast<size_t>(bounds_.height / lineHeight_);
}

void CodeEditor::setScrollPosition(float x, float y) {
    scrollX_ = x;
    scrollY_ = y;
}

void CodeEditor::setLanguage(const std::string& languageId) {
    languageId_ = languageId;
    if (highlighter_) {
        highlighter_->setLanguage(languageId);
    }
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
        float y = (line - firstLine) * lineHeight_ - scrollY_;
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
        float y = (line - firstLine) * lineHeight_ - scrollY_;
        std::string lineText = getLine(line);
        renderer.drawText(lineText, gutterWidth_ + 10, y + lineHeight_ - 4,
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
        float x = gutterWidth_ + 10 + cursor.position.column * charWidth_ - scrollX_;
        float y = cursor.position.line * lineHeight_ - scrollY_;
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
    pos.line = static_cast<size_t>((y + scrollY_) / lineHeight_);
    pos.column = static_cast<size_t>((x - gutterWidth_ - 10 + scrollX_) / charWidth_);
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

// Line Number Gutter
LineNumberGutter::LineNumberGutter(CodeEditor* editor) : editor_(editor) {}

void LineNumberGutter::render(RenderEngine& renderer) {
    // Render line numbers
}

// Minimap Widget
Minimap::Minimap(CodeEditor* editor) : editor_(editor) {}

void Minimap::render(RenderEngine& renderer) {
    // Render minimap
}

bool Minimap::onMouseDown(const Core::MouseButtonPressEvent& event) {
    dragging_ = true;
    return true;
}

bool Minimap::onMouseMove(const Core::MouseMoveEvent& event) {
    if (dragging_) {
        // Scroll editor based on mouse position
    }
    return true;
}

} // namespace NexusForge::UI
