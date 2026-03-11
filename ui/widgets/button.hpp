// ui/widgets/button.hpp
#pragma once

#include "../framework/widget_system.hpp"

namespace NexusForge::UI {

// Button types
enum class ButtonType {
    Normal,
    Primary,
    Secondary,
    Danger,
    Success,
    Warning,
    Link,
    Icon
};

// Button widget
class Button : public Widget {
public:
    using Ptr = std::shared_ptr<Button>;

    Button();
    ~Button() override;

    // Text
    void setText(const std::string& text) { text_ = text; markDirty(); }
    const std::string& getText() const { return text_; }

    // Icon
    void setIcon(const std::string& icon) { icon_ = icon; markDirty(); }
    const std::string& getIcon() const { return icon_; }

    // Type
    void setButtonType(ButtonType type) { buttonType_ = type; updateStyle(); }
    ButtonType getButtonType() const { return buttonType_; }

    // State
    void setToggleable(bool toggleable) { toggleable_ = toggleable; }
    bool isToggleable() const { return toggleable_; }
    void setToggled(bool toggled) { toggled_ = toggled; updateStyle(); }
    bool isToggled() const { return toggled_; }

    // Enabled
    void setEnabled(bool enabled) override;

    // Callbacks
    std::function<void()> onClick;
    std::function<void(bool)> onToggled;

    // Rendering
    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;

    // Events
    bool onMouseDown(const Core::MouseButtonPressEvent& event) override;
    bool onMouseUp(const Core::MouseButtonReleaseEvent& event) override;
    bool onMouseEnter(const Core::MouseMoveEvent& event) override;
    bool onMouseLeave(const Core::MouseMoveEvent& event) override;
    bool onKeyDown(const Core::KeyPressEvent& event) override;

protected:
    void updateStyle();
    Color getBackgroundColor() const;
    Color getForegroundColor() const;

    std::string text_;
    std::string icon_;
    ButtonType buttonType_ = ButtonType::Normal;
    bool toggleable_ = false;
    bool toggled_ = false;
    bool pressed_ = false;
};

// Toggle button
class ToggleButton : public Button {
public:
    ToggleButton();

    void setChecked(bool checked) { setToggled(checked); }
    bool isChecked() const { return isToggled(); }

    std::function<void(bool)> onChecked;
};

// IconButton
class IconButton : public Button {
public:
    IconButton();

    void setIconSize(float size) { iconSize_ = size; markDirty(); }
    float getIconSize() const { return iconSize_; }

    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;

private:
    float iconSize_ = 16.0f;
};

} // namespace NexusForge::UI
