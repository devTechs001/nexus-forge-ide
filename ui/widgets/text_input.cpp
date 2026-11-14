// ui/widgets/text_input.cpp
#include "text_input.hpp"
#include <algorithm>
#include <cmath>

namespace NexusForge::UI {

TextInput::TextInput() {
    setFocusPolicy(FocusPolicy::StrongFocus);
    style_.backgroundColor = GradientColor::fromHex(0x2A2A3E);
    style_.foregroundColor = GradientColor::fromHex(0xE8E8F0);
    style_.borderWidth = 1;
    style_.borderColor = GradientColor::fromHex(0x4A4A6A);
    style_.borderRadius = 6;
    style_.padding = EdgeInsets(8, 6);
}

TextInput::~TextInput() = default;

void TextInput::setText(const std::string& text) {
    text_ = text;
    if (cursorPos_ > text_.length()) {
        cursorPos_ = text_.length();
    }
    updateDisplayText();
}

void TextInput::setCursorPos(size_t pos) {
    cursorPos_ = std::min(pos, text_.length());
    selectionStart_ = cursorPos_;
    selectionEnd_ = cursorPos_;
    ensureCursorVisible();
}

void TextInput::setSelection(size_t start, size_t end) {
    size_t clampedStart = std::min(start, text_.length());
    size_t clampedEnd = std::min(end, text_.length());
    selectionStart_ = std::min(clampedStart, clampedEnd);
    selectionEnd_ = std::max(clampedStart, clampedEnd);
    cursorPos_ = clampedEnd;
    ensureCursorVisible();
}

std::string TextInput::getSelectedText() const {
    if (selectionStart_ == selectionEnd_) return "";
    return text_.substr(selectionStart_, selectionEnd_ - selectionStart_);
}

void TextInput::render(RenderEngine& renderer) {
    if (!isVisibleInHierarchy()) return;

    // Draw background
    Color bgColor = getCurrentBackgroundColor();
    if (hasFocus()) {
        bgColor = Color(0.15f, 0.15f, 0.25f, 1.0f);
    }
    renderer.drawRoundedRect(bounds_, bgColor, style_.borderRadius);

    // Draw border
    Color borderColor;
    if (hasFocus()) {
        borderColor = Color(0.3f, 0.5f, 0.9f, 1.0f);
    } else if (isHovered()) {
        borderColor = Color(0.4f, 0.4f, 0.5f, 1.0f);
    } else {
        borderColor = Color(0.3f, 0.3f, 0.4f, 1.0f);
    }
    
    DrawCommand borderCmd;
    borderCmd.type = DrawCommandType::DrawRoundedRect;
    borderCmd.rect = bounds_;
    borderCmd.color = borderColor;
    borderCmd.cornerRadius = style_.borderRadius;
    borderCmd.thickness = style_.borderWidth;
    // Would render border

    // Draw text
    float textX = bounds_.x + style_.padding.left - scrollOffset_;
    float textY = bounds_.y + bounds_.height / 2 + style_.fontSize / 3;
    Color textColor = isEnabled() ? Color(0.9f, 0.9f, 0.95f, 1.0f) : Color(0.4f, 0.4f, 0.5f, 1.0f);

    // Draw placeholder if empty
    if (text_.empty() && !placeholder_.empty() && !hasFocus()) {
        renderer.drawText(placeholder_, textX, textY, Color(0.5f, 0.5f, 0.6f, 1.0f), 
                          style_.fontFamily, style_.fontSize);
    }

    // Draw selection
    if (selectionStart_ != selectionEnd_) {
        float selX = getTextOffset(selectionStart_);
        float selW = getTextOffset(selectionEnd_) - selX;
        renderer.drawRect(Rect(bounds_.x + selX, bounds_.y + 4, selW, bounds_.height - 8), 
                          Color(0.3f, 0.5f, 0.9f, 0.5f));
    }

    // Draw text content
    if (!displayText_.empty()) {
        renderer.drawText(displayText_, textX, textY, textColor, 
                          style_.fontFamily, style_.fontSize);
    }

    // Draw cursor
    if (hasFocus() && cursorVisible_) {
        float cursorX = getTextOffset(cursorPos_);
        renderer.drawRect(Rect(bounds_.x + cursorX, bounds_.y + 4, 2, bounds_.height - 8), 
                          Color(0.9f, 0.9f, 0.95f, 1.0f));
    }
}

Vec2 TextInput::measure(float availableWidth, float availableHeight) {
    float height = style_.padding.vertical() + style_.fontSize + 4;
    float width = std::max(100.0f, constraints_.preferredWidth);
    
    if (constraints_.preferredWidth < 0) {
        width = std::min(availableWidth, 300.0f);
    }
    
    return Vec2(width, std::max(height, constraints_.minHeight));
}

bool TextInput::handleEvent(Core::Event& event) {
    return Widget::handleEvent(event);
}

bool TextInput::onKeyDown(const Core::KeyPressEvent& event) {
    if (readOnly_) return false;

    // Handle keyboard shortcuts
    bool ctrl = (event.modifiers & 2) != 0;
    
    if (ctrl) {
        switch (event.keyCode) {
            case 65: // A - Select all
                setSelection(0, text_.length());
                return true;
            case 67: // C - Copy
                // Would copy to clipboard
                return true;
            case 86: // V - Paste
                // Would paste from clipboard
                return true;
            case 88: // X - Cut
                // Would cut to clipboard
                deleteSelection();
                return true;
        }
    }
    
    switch (event.keyCode) {
        case 13: // Enter
            if (onSubmit) onSubmit(text_);
            return true;
        case 37: // Left arrow
            moveCursor(-1, (event.modifiers & 1) != 0); // Shift for selection
            return true;
        case 39: // Right arrow
            moveCursor(1, (event.modifiers & 1) != 0);
            return true;
        case 8: // Backspace
            if (selectionStart_ != selectionEnd_) {
                deleteSelection();
            } else if (cursorPos_ > 0) {
                text_.erase(cursorPos_ - 1, 1);
                cursorPos_--;
                selectionStart_ = cursorPos_;
                selectionEnd_ = cursorPos_;
                if (onTextChange) onTextChange(text_);
            }
            updateDisplayText();
            return true;
        case 46: // Delete
            if (selectionStart_ != selectionEnd_) {
                deleteSelection();
            } else if (cursorPos_ < text_.length()) {
                text_.erase(cursorPos_, 1);
                if (onTextChange) onTextChange(text_);
            }
            updateDisplayText();
            return true;
        case 36: // Home
            setCursorPos(0);
            return true;
        case 35: // End
            setCursorPos(text_.length());
            return true;
    }
    
    return false;
}

bool TextInput::onCharInput(const Core::CharInputEvent& event) {
    if (readOnly_) return false;
    
    char c = static_cast<char>(event.codepoint);
    if (c >= 32 && c != 127) {
        insertText(std::string(1, c));
    }
    
    return true;
}

bool TextInput::onMouseDown(const Core::MouseButtonPressEvent& event) {
    requestFocus();
    float clickX = event.x - bounds_.x - style_.padding.left;
    cursorPos_ = getPosFromX(clickX + scrollOffset_);
    selectionStart_ = cursorPos_;
    selectionEnd_ = cursorPos_;
    return true;
}

bool TextInput::onFocusGained() {
    Widget::onFocusGained();
    cursorBlinkTimer_ = 0;
    cursorVisible_ = true;
    if (selectAllOnFocus_) {
        setSelection(0, text_.length());
    }
    if (onFocusChanged) onFocusChanged();
    return true;
}

bool TextInput::onFocusLost() {
    Widget::onFocusLost();
    cursorVisible_ = false;
    if (onFocusChanged) onFocusChanged();
    return true;
}

void TextInput::update(double deltaTime) {
    Widget::update(deltaTime);
    
    if (hasFocus()) {
        cursorBlinkTimer_ += deltaTime;
        if (cursorBlinkTimer_ >= CURSOR_BLINK_INTERVAL) {
            cursorVisible_ = !cursorVisible_;
            cursorBlinkTimer_ = 0;
        }
    }
}

void TextInput::updateDisplayText() {
    displayText_ = text_;
    
    if (type_ == Type::Password) {
        displayText_ = std::string(text_.length(), '*');
    }
    
    ensureCursorVisible();
}

void TextInput::ensureCursorVisible() {
    float cursorX = getTextOffset(cursorPos_);
    float visibleWidth = bounds_.width - style_.padding.horizontal() - 4;
    
    if (cursorX < scrollOffset_) {
        scrollOffset_ = cursorX;
    } else if (cursorX > scrollOffset_ + visibleWidth) {
        scrollOffset_ = cursorX - visibleWidth;
    }
}

void TextInput::insertText(const std::string& newText) {
    if (readOnly_) return;
    
    if (selectionStart_ != selectionEnd_) {
        deleteSelection();
    }
    
    if (maxLength_ > 0 && static_cast<int>(text_.length() + newText.length()) > maxLength_) {
        return;
    }
    
    text_.insert(cursorPos_, newText);
    cursorPos_ += newText.length();
    selectionStart_ = cursorPos_;
    selectionEnd_ = cursorPos_;
    
    updateDisplayText();
    if (onTextChange) onTextChange(text_);
}

void TextInput::deleteSelection() {
    if (selectionStart_ == selectionEnd_) return;
    
    size_t start = std::min(selectionStart_, selectionEnd_);
    size_t end = std::max(selectionStart_, selectionEnd_);
    
    text_.erase(start, end - start);
    cursorPos_ = start;
    selectionStart_ = start;
    selectionEnd_ = start;
    
    updateDisplayText();
    if (onTextChange) onTextChange(text_);
}

void TextInput::moveCursor(int delta, bool select) {
    int newPos = static_cast<int>(cursorPos_) + delta;
    newPos = std::max(0, std::min(static_cast<int>(text_.length()), newPos));
    
    if (select) {
        selectionEnd_ = newPos;
    } else {
        selectionStart_ = newPos;
        selectionEnd_ = newPos;
    }
    cursorPos_ = newPos;
    ensureCursorVisible();
}

float TextInput::getTextOffset(size_t pos) const {
    return static_cast<float>(pos) * style_.fontSize * 0.5f;
}

size_t TextInput::getPosFromX(float x) const {
    float charWidth = style_.fontSize * 0.5f;
    if (x <= 0.0f || charWidth <= 0.0f) return 0;
    size_t pos = static_cast<size_t>(std::round(x / charWidth));
    return std::min(pos, text_.length());
}

} // namespace NexusForge::UI
