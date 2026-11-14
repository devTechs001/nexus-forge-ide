// ui/widgets/text_input.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include <string>
#include <functional>
#include <memory>

namespace NexusForge::UI {

class TextInput : public Widget {
public:
    using Ptr = std::shared_ptr<TextInput>;

    TextInput();
    ~TextInput() override;

    // Text
    void setText(const std::string& text);
    const std::string& getText() const { return text_; }
    void setPlaceholder(const std::string& placeholder) { placeholder_ = placeholder; }
    const std::string& getPlaceholder() const { return placeholder_; }

    // Input type
    enum class Type { Text, Password, Email, Number, Search };
    void setType(Type type) { type_ = type; }
    Type getType() const { return type_; }

    // Options
    void setReadOnly(bool readOnly) { readOnly_ = readOnly; }
    bool isReadOnly() const { return readOnly_; }
    void setMaxLength(int maxLength) { maxLength_ = maxLength; }
    int getMaxLength() const { return maxLength_; }
    void setAutoFocus(bool autoFocus) { autoFocus_ = autoFocus; }
    void setSelectAllOnFocus(bool selectAll) { selectAllOnFocus_ = selectAll; }

    // Cursor
    size_t getCursorPos() const { return cursorPos_; }
    void setCursorPos(size_t pos);
    size_t getSelectionStart() const { return selectionStart_; }
    size_t getSelectionEnd() const { return selectionEnd_; }
    void setSelection(size_t start, size_t end);
    std::string getSelectedText() const;

    // Events
    std::function<void(const std::string&)> onTextChange;
    std::function<void(const std::string&)> onSubmit;
    std::function<void()> onFocusChanged;

    // Widget overrides
    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;
    bool handleEvent(Core::Event& event) override;
    bool onKeyDown(const Core::KeyPressEvent& event) override;
    bool onCharInput(const Core::CharInputEvent& event) override;
    bool onMouseDown(const Core::MouseButtonPressEvent& event) override;
    bool onFocusGained() override;
    bool onFocusLost() override;
    void update(double deltaTime) override;

private:
    std::string text_;
    std::string placeholder_;
    std::string displayText_;
    Type type_ = Type::Text;
    
    size_t cursorPos_ = 0;
    size_t selectionStart_ = 0;
    size_t selectionEnd_ = 0;
    float scrollOffset_ = 0;
    
    bool readOnly_ = false;
    int maxLength_ = 0;
    bool autoFocus_ = false;
    bool selectAllOnFocus_ = true;
    
    double cursorBlinkTimer_ = 0;
    bool cursorVisible_ = true;
    static constexpr double CURSOR_BLINK_INTERVAL = 0.53;

    void updateDisplayText();
    void ensureCursorVisible();
    void insertText(const std::string& text);
    void deleteSelection();
    void moveCursor(int delta, bool select);
    float getTextOffset(size_t pos) const;
    size_t getPosFromX(float x) const;
};

} // namespace NexusForge::UI
