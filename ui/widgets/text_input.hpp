// ui/widgets/text_input.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include <string>
#include <functional>

namespace NexusForge::UI {

// Input types
enum class InputType {
    Text,
    Password,
    Email,
    Number,
    Integer,
    Phone,
    Url,
    Search,
    Multiline
};

// Text input widget
class TextInput : public Widget {
public:
    using Ptr = std::shared_ptr<TextInput>;

    TextInput();
    ~TextInput() override;

    // Value
    void setText(const std::string& text);
    const std::string& getText() const { return text_; }
    void clear();

    // Placeholder
    void setPlaceholder(const std::string& placeholder) { placeholder_ = placeholder; markDirty(); }
    const std::string& getPlaceholder() const { return placeholder_; }

    // Input type
    void setInputType(InputType type) { inputType_ = type; }
    InputType getInputType() const { return inputType_; }

    // Validation
    void setMaxLength(size_t length) { maxLength_ = length; }
    size_t getMaxLength() const { return maxLength_; }
    void setPattern(const std::string& pattern) { pattern_ = pattern; }
    const std::string& getPattern() const { return pattern_; }
    bool isValid() const;

    // Read-only
    void setReadOnly(bool readOnly) { readOnly_ = readOnly; }
    bool isReadOnly() const { return readOnly_; }

    // Selection
    void setSelection(size_t start, size_t end);
    size_t getSelectionStart() const { return selectionStart_; }
    size_t getSelectionEnd() const { return selectionEnd_; }
    std::string getSelectedText() const;
    void selectAll();

    // Cursor
    size_t getCursorPosition() const { return cursorPosition_; }
    void setCursorPosition(size_t pos);

    // Callbacks
    std::function<void(const std::string&)> onTextChanged;
    std::function<void(const std::string&)> onSubmitted;
    std::function<void()> onFocusGainedCallback;
    std::function<void()> onFocusLostCallback;

    // Rendering
    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;
    void update(double deltaTime) override;

    // Events
    bool onKeyDown(const Core::KeyPressEvent& event) override;
    bool onCharInput(const Core::CharInputEvent& event) override;
    bool onMouseDown(const Core::MouseButtonPressEvent& event) override;
    bool onFocusGained() override;
    bool onFocusLost() override;

protected:
    void updateCursorBlink(double deltaTime);
    void ensureCursorVisible();
    void notifyTextChanged();
    std::string getDisplayText() const;

    std::string text_;
    std::string placeholder_;
    InputType inputType_ = InputType::Text;
    size_t maxLength_ = 0;
    std::string pattern_;
    bool readOnly_ = false;

    size_t cursorPosition_ = 0;
    size_t selectionStart_ = 0;
    size_t selectionEnd_ = 0;

    bool focused_ = false;
    bool cursorVisible_ = true;
    double cursorBlinkTimer_ = 0;
    float scrollOffset_ = 0;
};

// Numeric input
class NumberInput : public TextInput {
public:
    NumberInput();

    void setValue(double value);
    double getValue() const;

    void setMinValue(double min) { minValue_ = min; }
    void setMaxValue(double max) { maxValue_ = max; }
    void setStep(double step) { step_ = step; }
    void setDecimals(int decimals) { decimals_ = decimals; }

    std::function<void(double)> onValueChanged;

private:
    double minValue_ = 0;
    double maxValue_ = 1000000;
    double step_ = 1;
    int decimals_ = 2;
    double value_ = 0;
};

} // namespace NexusForge::UI
