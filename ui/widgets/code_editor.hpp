// ui/widgets/code_editor.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include "../../editor/buffer/text_buffer.hpp"
#include "../../editor/syntax/syntax_highlighter.hpp"

#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace NexusForge::UI {

struct CursorPosition {
    size_t line = 0;
    size_t column = 0;
    bool operator==(const CursorPosition& other) const {
        return line == other.line && column == other.column;
    }
};

struct SelectionRange {
    CursorPosition start;
    CursorPosition end;
    bool isEmpty() const { return start == end; }
    CursorPosition getMin() const { 
        return (start.line < end.line || (start.line == end.line && start.column <= end.column)) ? start : end; 
    }
    CursorPosition getMax() const { 
        return (start.line < end.line || (start.line == end.line && start.column <= end.column)) ? end : start; 
    }
};

struct CursorInfo {
    CursorPosition position;
    SelectionRange selection;
};

struct FindOptions {
    bool caseSensitive = true;
    bool wholeWord = false;
    bool useRegex = false;
};

struct GutterMarker {
    size_t line;
    std::string type;
    std::string icon;
    Color color;
    std::string tooltip;
};

struct InlineDecoration {
    CursorPosition start;
    CursorPosition end;
    Color backgroundColor;
    Color textColor;
    std::string type;
};

struct EditorOptions {
    std::string fontFamily = "JetBrains Mono";
    float fontSize = 14.0f;
    float lineHeight = 1.5f;
    bool showLineNumbers = true;
    bool showMinimap = true;
    bool wordWrap = false;
    bool autoIndent = true;
    bool autoPairs = true;
    bool tabsToSpaces = true;
    int tabSize = 4;
    std::string cursorStyle = "line";
    float cursorBlinkRate = 530;
};

class CodeEditor : public Widget {
public:
    using Ptr = std::shared_ptr<CodeEditor>;

    CodeEditor();
    ~CodeEditor() override;

    // Document management
    void openFile(const std::string& path);
    void openBuffer(std::shared_ptr<Editor::TextBuffer> buffer);
    void newFile(const std::string& language = "");
    bool saveFile(const std::string& path = "");
    void closeFile();

    std::shared_ptr<Editor::TextBuffer> getBuffer() const { return buffer_; }
    const std::string& getFilePath() const { return filePath_; }

    // Content
    std::string getText() const;
    std::string getSelectedText() const;
    void setText(const std::string& text);
    void insertText(const std::string& text);
    void deleteSelection();
    std::string getLine(size_t lineNum) const;
    size_t getLineCount() const;
    void insertLine(size_t lineNum, const std::string& text);
    void deleteLine(size_t lineNum);
    void duplicateLine();
    void moveLinesUp();
    void moveLinesDown();

    // Cursor operations
    CursorPosition getCursorPosition() const;
    void setCursorPosition(const CursorPosition& pos);
    void setCursorPosition(size_t line, size_t column);
    void moveCursor(int lineDelta, int columnDelta);
    void moveCursorToLineEnd();
    void moveCursorToDocumentStart();
    void moveCursorToDocumentEnd();
    void moveCursorWordLeft();
    void moveCursorWordRight();
    void addCursor(const CursorPosition& pos);
    void addCursorAbove();
    void addCursorBelow();
    void addCursorsToLineEnds();
    void clearSecondaryCursors();

    // Selection
    SelectionRange getSelection() const;
    void setSelection(const SelectionRange& range);
    void selectAll();
    void selectLine(size_t lineNum);

    // Find & Replace
    std::vector<SelectionRange> findAll(const std::string& pattern, bool caseSensitive = false);
    std::vector<SelectionRange> findAll(const std::string& pattern, const FindOptions& options);
    bool findNext(const std::string& pattern);
    bool findNext(const std::string& pattern, const FindOptions& options);
    bool findPrevious(const std::string& pattern, const FindOptions& options);
    int replaceAll(const std::string& pattern, const std::string& replacement);
    int replaceAll(const std::string& pattern, const std::string& replacement, const FindOptions& options);
    bool replaceNext(const std::string& pattern, const std::string& replacement, const FindOptions& options);

    // Scrolling
    void scrollToLine(size_t line, bool center = true);
    void scrollToCursor();
    void scrollBy(float deltaX, float deltaY);
    size_t getFirstVisibleLine() const;
    size_t getLastVisibleLine() const;
    void setScrollPosition(float x, float y);

    // IntelliSense
    void triggerCompletion();
    void triggerSignatureHelp();
    void goToDefinition();
    void findReferences();
    void rename(const std::string& newName);
    void formatDocument();
    void formatSelection();
    void quickFix();
    void refactor();

    // Code folding
    void foldRegion(size_t startLine);
    void unfoldRegion(size_t line);
    void foldAll();
    void unfoldAll();
    void toggleFold(size_t line);

    // Gutter markers
    void addGutterMarker(const GutterMarker& marker);
    void removeGutterMarker(size_t line, const std::string& type);
    void clearGutterMarkers();

    // Decorations
    void addDecoration(const InlineDecoration& decoration);
    void removeDecoration(const CursorPosition& pos, const std::string& type);
    void clearDecorations();

    // Comments & Indentation
    void toggleLineComment();
    void toggleBlockComment();
    void indent();
    void outdent();

    // Selection helpers
    void selectWord();
    void selectBracketContents();
    void expandSelection();
    void shrinkSelection();

    // Language
    void setLanguage(const std::string& languageId);
    const std::string& getLanguage() const { return languageId_; }

    // Undo/Redo
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;

    // Options
    void setOptions(const EditorOptions& options) { options_ = options; invalidateLayout(); }
    const EditorOptions& getOptions() const { return options_; }

    // Events
    std::function<void()> onTextChanged;
    std::function<void(const CursorPosition&)> onCursorChanged;
    std::function<void()> onSaved;

    // Widget overrides
    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;
    void update(double deltaTime) override;
    bool onKeyDown(const Core::KeyPressEvent& event) override;
    bool onCharInput(const Core::CharInputEvent& event) override;
    bool onMouseDown(const Core::MouseButtonPressEvent& event) override;
    bool onMouseUp(const Core::MouseButtonReleaseEvent& event) override;
    bool onMouseMove(const Core::MouseMoveEvent& event) override;
    bool onScroll(const Core::ScrollEvent& event) override;
    bool onTouchStart(const Core::TouchEvent& event) override;
    bool onTouchMove(const Core::TouchEvent& event) override;
    bool onTouchEnd(const Core::TouchEvent& event) override;
    bool onGesture(const Core::GestureEvent& event) override;

private:
    std::shared_ptr<Editor::TextBuffer> buffer_;
    std::string filePath_;
    std::string languageId_;

    CursorPosition cursor_;
    SelectionRange selection_;
    std::vector<CursorPosition> cursors_;  // Multi-cursor support
    std::vector<GutterMarker> gutterMarkers_;
    std::vector<InlineDecoration> decorations_;

    float scrollX_ = 0;
    float scrollY_ = 0;

    EditorOptions options_;
    float lineHeight_ = 20;
    float charWidth_ = 8;
    float gutterWidth_ = 60;
    float minimapWidth_ = 100;

    bool mouseDown_ = false;
    bool selecting_ = false;
    CursorPosition mouseDownPos_;

    bool cursorVisible_ = true;
    float cursorBlinkTimer_ = 0;

    std::unique_ptr<Editor::SyntaxHighlighter> highlighter_;

    void renderGutter(RenderEngine& renderer);
    void renderContent(RenderEngine& renderer);
    void renderMinimap(RenderEngine& renderer);
    void renderCursors(RenderEngine& renderer);
    void renderSelection(RenderEngine& renderer);
    void renderLineHighlight(RenderEngine& renderer);
    void renderBracketMatching(RenderEngine& renderer);
    void renderIndentGuides(RenderEngine& renderer);
    void renderDecorations(RenderEngine& renderer);
    void renderScrollbars(RenderEngine& renderer);

    CursorPosition positionFromPoint(float x, float y) const;
    Vec2 pointFromPosition(const CursorPosition& pos) const;
    float getContentWidth() const;
    float getContentHeight() const;
    size_t getVisibleLineCount() const;

    void ensureCursorVisible();
    void normalizeSelection();
    void mergeOverlappingSelections();
    void updateBracketMatching();
    void updateScrollBounds();
    void handleAutoIndent();
    void handleAutoPairs(char c);
};

} // namespace NexusForge::UI
