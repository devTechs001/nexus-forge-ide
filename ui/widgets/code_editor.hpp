// ui/widgets/code_editor.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include "../../editor/buffer/text_buffer.hpp"

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

    // Cursor operations
    CursorPosition getCursorPosition() const;
    void setCursorPosition(const CursorPosition& pos);
    void moveCursor(int lineDelta, int columnDelta);

    // Selection
    SelectionRange getSelection() const;
    void setSelection(const SelectionRange& range);
    void selectAll();
    void selectLine(size_t lineNum);

    // Find & Replace
    std::vector<SelectionRange> findAll(const std::string& pattern, bool caseSensitive = false);
    bool findNext(const std::string& pattern);
    int replaceAll(const std::string& pattern, const std::string& replacement);

    // Scrolling
    void scrollToLine(size_t line, bool center = true);
    void scrollToCursor();

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
    bool onKeyDown(const Core::KeyPressEvent& event) override;
    bool onCharInput(const Core::CharInputEvent& event) override;
    bool onMouseDown(const Core::MouseButtonPressEvent& event) override;
    bool onMouseMove(const Core::MouseMoveEvent& event) override;

private:
    std::shared_ptr<Editor::TextBuffer> buffer_;
    std::string filePath_;
    std::string languageId_;

    CursorPosition cursor_;
    SelectionRange selection_;
    std::vector<CursorPosition> cursors_;  // Multi-cursor support

    float scrollX_ = 0;
    float scrollY_ = 0;

    EditorOptions options_;
    float lineHeight_ = 20;
    float charWidth_ = 8;
    float gutterWidth_ = 60;

    bool mouseDown_ = false;
    bool selecting_ = false;

    void renderGutter(RenderEngine& renderer);
    void renderContent(RenderEngine& renderer);
    void renderCursor(RenderEngine& renderer);
    void renderSelection(RenderEngine& renderer);
    
    CursorPosition positionFromPoint(float x, float y) const;
    void ensureCursorVisible();
};

} // namespace NexusForge::UI
