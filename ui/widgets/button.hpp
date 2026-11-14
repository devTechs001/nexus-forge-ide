// ui/widgets/button.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include <functional>
#include <string>
#include <memory>

namespace NexusForge::UI {

class Button : public Widget {
public:
    using Ptr = std::shared_ptr<Button>;

    Button();
    ~Button() override;

    // Text
    void setText(const std::string& text) { text_ = text; markDirty(); }
    const std::string& getText() const { return text_; }

    // Icon
    void setIcon(TextureHandle icon) { icon_ = icon; markDirty(); }
    TextureHandle getIcon() const { return icon_; }

    // Button type
    enum class Type { Normal, Primary, Danger, Ghost, Icon };
    void setType(Type type) { type_ = type; updateStyleForType(); }
    Type getType() const { return type_; }

    // Rendering
    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;

    // Events
    bool onMouseDown(const Core::MouseButtonPressEvent& event) override;
    bool onMouseUp(const Core::MouseButtonReleaseEvent& event) override;
    bool onClick(const Core::MouseButtonReleaseEvent& event) override;
    bool onKeyDown(const Core::KeyPressEvent& event) override;

private:
    std::string text_;
    TextureHandle icon_ = 0;
    Type type_ = Type::Normal;

    void updateStyleForType();
};

} // namespace NexusForge::UI
