// ui/widgets/button.cpp
#include "button.hpp"

namespace NexusForge::UI {

Button::Button() {
    setFocusPolicy(FocusPolicy::ClickFocus);
    setCursor(CursorType::Hand);
}

Button::~Button() = default;

void Button::updateStyleForType() {
    switch (type_) {
        case Type::Primary:
            style_.backgroundColor = GradientColor::fromHex(0x4A90D9);
            style_.foregroundColor = GradientColor::fromHex(0xFFFFFF);
            style_.borderRadius = 6;
            break;
        case Type::Danger:
            style_.backgroundColor = GradientColor::fromHex(0xD94A4A);
            style_.foregroundColor = GradientColor::fromHex(0xFFFFFF);
            style_.borderRadius = 6;
            break;
        case Type::Ghost:
            style_.backgroundColor = GradientColor::fromRGBA(0, 0, 0, 0);
            style_.foregroundColor = GradientColor::fromHex(0x4A90D9);
            style_.borderRadius = 6;
            break;
        case Type::Icon:
            style_.backgroundColor = GradientColor::fromRGBA(0, 0, 0, 0);
            style_.foregroundColor = GradientColor::fromHex(0xFFFFFF);
            style_.borderRadius = 4;
            break;
        default:
            style_.backgroundColor = GradientColor::fromHex(0x3A3A4A);
            style_.foregroundColor = GradientColor::fromHex(0xFFFFFF);
            style_.borderRadius = 6;
            break;
    }
}

void Button::render(RenderEngine& renderer) {
    if (!isVisibleInHierarchy()) return;

    Color bgColor = getCurrentBackgroundColor();
    Color fgColor = getCurrentForegroundColor();

    // Draw background
    if (bgColor.a > 0 || type_ == Type::Ghost || type_ == Type::Icon) {
        if (type_ == Type::Ghost || type_ == Type::Icon) {
            if (isHovered()) {
                bgColor = Color(0.2f, 0.2f, 0.3f, 0.3f);
            }
        }
        renderer.drawRoundedRect(bounds_, bgColor, style_.borderRadius);
    }

    // Draw border for ghost buttons
    if (type_ == Type::Ghost && !isHovered()) {
        Color borderColor(0.3f, 0.3f, 0.4f, 1.0f);
        DrawCommand borderCmd;
        borderCmd.type = DrawCommandType::DrawRoundedRect;
        borderCmd.rect = bounds_;
        borderCmd.color = borderColor;
        borderCmd.cornerRadius = style_.borderRadius;
        borderCmd.thickness = 1;
        // Would render border
    }

    // Draw icon
    if (icon_ > 0) {
        float iconSizeVal = std::min(bounds_.height - 8.0f, 24.0f);
        float iconX = bounds_.x + (bounds_.width - iconSizeVal) / 2;
        if (!text_.empty()) {
            iconX = bounds_.x + 12;
        }
        float iconY = bounds_.y + (bounds_.height - iconSizeVal) / 2;
        renderer.drawImage(icon_, Rect(iconX, iconY, iconSizeVal, iconSizeVal), fgColor);
    }

    // Draw text
    if (!text_.empty()) {
        Vec2 textSize = renderer.measureText(text_, style_.fontFamily, style_.fontSize);
        float textX = bounds_.x + (bounds_.width - textSize.x) / 2;
        float iconSizeVal = std::min(bounds_.height - 8.0f, 24.0f);
        if (icon_ > 0) {
            textX = bounds_.x + 12 + iconSizeVal + 8;
        }
        float textY = bounds_.y + (bounds_.height - textSize.y) / 2 + textSize.y / 2;
        renderer.drawText(text_, textX, textY, fgColor, style_.fontFamily, style_.fontSize);
    }

    // Draw focus indicator
    if (hasFocus()) {
        Color focusColor(0.3f, 0.5f, 0.9f, 0.5f);
        renderer.drawRoundedRect(bounds_, focusColor, style_.borderRadius);
    }
}

Vec2 Button::measure(float availableWidth, float availableHeight) {
    float width = style_.padding.left + style_.padding.right;
    float height = style_.padding.top + style_.padding.bottom;

    float contentWidth = 0;
    float contentHeight = 0;

    // Measure icon
    if (icon_ > 0) {
        contentWidth += 24;
        contentHeight = std::max(contentHeight, 24.0f);
    }

    // Measure text
    if (!text_.empty()) {
        Vec2 textSize(0, 0); // Would measure text
        float textWidth = static_cast<float>(text_.length()) * style_.fontSize * 0.5f;
        contentWidth += textWidth;
        contentHeight = std::max(contentHeight, style_.fontSize);

        // Add spacing between icon and text
        if (icon_ > 0) {
            contentWidth += 8;
        }
    }

    width += contentWidth;
    height = std::max(height, contentHeight) + 8;

    // Apply constraints
    width = std::clamp(width, constraints_.minWidth, 
                       std::min(constraints_.maxWidth, availableWidth));
    height = std::clamp(height, constraints_.minHeight,
                        std::min(constraints_.maxHeight, availableHeight));

    return Vec2(width, height);
}

bool Button::onMouseDown(const Core::MouseButtonPressEvent& event) {
    if (isEnabled()) {
        setState(WidgetState::Pressed);
        return true;
    }
    return false;
}

bool Button::onMouseUp(const Core::MouseButtonReleaseEvent& event) {
    if (isEnabled() && isPressed()) {
        setState(WidgetState::Hovered);
        return true;
    }
    return false;
}

bool Button::onClick(const Core::MouseButtonReleaseEvent& event) {
    if (isEnabled() && onClick_) {
        onClick_();
        return true;
    }
    return false;
}

bool Button::onKeyDown(const Core::KeyPressEvent& event) {
    // Handle Enter and Space keys
    if (isEnabled() && (event.keyCode == 13 || event.keyCode == 32)) {
        if (onClick_) {
            onClick_();
        }
        return true;
    }
    return false;
}

} // namespace NexusForge::UI
