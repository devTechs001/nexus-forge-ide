// ui/framework/widget_system.hpp
#pragma once

#include "../renderer/render_engine.hpp"
#include "../../core/engine/event_system.hpp"

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <optional>

namespace NexusForge::UI {

// Forward declarations
class Widget;
class Container;
class Window;

// Size constraints
struct SizeConstraints {
    float minWidth = 0;
    float minHeight = 0;
    float maxWidth = std::numeric_limits<float>::max();
    float maxHeight = std::numeric_limits<float>::max();
    float preferredWidth = -1;
    float preferredHeight = -1;
};

// Alignment
enum class HAlignment { Left, Center, Right, Stretch };
enum class VAlignment { Top, Center, Bottom, Stretch };

// Padding/Margin
struct EdgeInsets {
    float left = 0, top = 0, right = 0, bottom = 0;

    EdgeInsets() = default;
    EdgeInsets(float all) : left(all), top(all), right(all), bottom(all) {}
    EdgeInsets(float horizontal, float vertical)
        : left(horizontal), top(vertical), right(horizontal), bottom(vertical) {}
    EdgeInsets(float l, float t, float r, float b)
        : left(l), top(t), right(r), bottom(b) {}

    float horizontal() const { return left + right; }
    float vertical() const { return top + bottom; }
};

// Widget state
enum class WidgetState {
    Normal,
    Hovered,
    Pressed,
    Focused,
    Disabled,
    Selected
};

// Cursor types
enum class CursorType {
    Arrow,
    IBeam,
    Hand,
    Crosshair,
    ResizeH,
    ResizeV,
    ResizeNESW,
    ResizeNWSE,
    Move,
    NotAllowed,
    Wait,
    Progress
};

// Focus policy
enum class FocusPolicy {
    NoFocus,
    ClickFocus,
    TabFocus,
    StrongFocus
};

// Widget style
struct WidgetStyle {
    Color backgroundColor = Color::Transparent();
    Color foregroundColor = Color::White();
    Color borderColor = Color(0.3f, 0.3f, 0.3f, 1.0f);
    Color hoverColor;
    Color pressedColor;
    Color disabledColor;
    Color focusColor;
    Color selectionColor;

    float borderWidth = 0;
    float borderRadius = 0;

    std::string fontFamily = "default";
    float fontSize = 14;
    bool fontBold = false;
    bool fontItalic = false;

    bool hasShadow = false;
    Color shadowColor;
    float shadowBlur = 4;
    float shadowOffsetX = 0;
    float shadowOffsetY = 2;

    EdgeInsets padding;
    float opacity = 1.0f;
    float transitionDuration = 0.15f;
};

// Animation
struct Animation {
    std::string property;
    float startValue;
    float endValue;
    float duration;
    float elapsed = 0;
    std::function<float(float)> easingFunction;
    std::function<void()> onComplete;
    bool loop = false;
    bool pingPong = false;
};

// Widget base class
class Widget : public std::enable_shared_from_this<Widget> {
public:
    using Ptr = std::shared_ptr<Widget>;
    using WeakPtr = std::weak_ptr<Widget>;

    Widget();
    virtual ~Widget();

    // Identification
    void setId(const std::string& id) { id_ = id; }
    const std::string& getId() const { return id_; }
    void addClass(const std::string& className);
    void removeClass(const std::string& className);
    bool hasClass(const std::string& className) const;

    // Geometry
    void setPosition(float x, float y);
    void setSize(float width, float height);
    void setBounds(const Rect& bounds);
    void setConstraints(const SizeConstraints& constraints);

    Vec2 getPosition() const { return Vec2(bounds_.x, bounds_.y); }
    Vec2 getSize() const { return Vec2(bounds_.width, bounds_.height); }
    const Rect& getBounds() const { return bounds_; }
    Rect getGlobalBounds() const;
    const SizeConstraints& getConstraints() const { return constraints_; }

    // Layout
    void setMargin(const EdgeInsets& margin) { margin_ = margin; }
    void setPadding(const EdgeInsets& padding) { style_.padding = padding; }
    void setAlignment(HAlignment h, VAlignment v);

    const EdgeInsets& getMargin() const { return margin_; }
    const EdgeInsets& getPadding() const { return style_.padding; }
    HAlignment getHAlignment() const { return hAlignment_; }
    VAlignment getVAlignment() const { return vAlignment_; }

    // Visibility
    void setVisible(bool visible);
    void setEnabled(bool enabled);
    bool isVisible() const { return visible_; }
    bool isEnabled() const { return enabled_; }
    bool isVisibleInHierarchy() const;

    // Focus
    void setFocusPolicy(FocusPolicy policy) { focusPolicy_ = policy; }
    FocusPolicy getFocusPolicy() const { return focusPolicy_; }
    void requestFocus();
    void clearFocus();
    bool hasFocus() const;

    // State
    WidgetState getState() const { return state_; }
    bool isHovered() const { return state_ == WidgetState::Hovered; }
    bool isPressed() const { return state_ == WidgetState::Pressed; }

    // Style
    void setStyle(const WidgetStyle& style) { style_ = style; markDirty(); }
    WidgetStyle& getStyle() { return style_; }
    const WidgetStyle& getStyle() const { return style_; }

    // Cursor
    void setCursor(CursorType cursor) { cursor_ = cursor; }
    CursorType getCursor() const { return cursor_; }

    // Tooltip
    void setTooltip(const std::string& tooltip) { tooltip_ = tooltip; }
    const std::string& getTooltip() const { return tooltip_; }

    // Hierarchy
    void setParent(Widget* parent);
    Widget* getParent() const { return parent_; }
    virtual void addChild(Ptr child);
    virtual void removeChild(Ptr child);
    virtual void removeChild(const std::string& id);
    void removeFromParent();
    const std::vector<Ptr>& getChildren() const { return children_; }
    Widget* findChild(const std::string& id) const;

    // Rendering
    virtual void render(RenderEngine& renderer);
    virtual void renderChildren(RenderEngine& renderer);
    void markDirty() { dirty_ = true; }
    bool isDirty() const { return dirty_; }

    // Layout
    virtual Vec2 measure(float availableWidth, float availableHeight);
    virtual void arrange(const Rect& finalRect);
    void invalidateLayout();

    // Update
    virtual void update(double deltaTime);
    virtual void updateAnimations(double deltaTime);

    // Event handling
    virtual bool handleEvent(Core::Event& event);
    virtual bool onMouseEnter(const Core::MouseMoveEvent& event);
    virtual bool onMouseLeave(const Core::MouseMoveEvent& event);
    virtual bool onMouseMove(const Core::MouseMoveEvent& event);
    virtual bool onMouseDown(const Core::MouseButtonPressEvent& event);
    virtual bool onMouseUp(const Core::MouseButtonReleaseEvent& event);
    virtual bool onClick(const Core::MouseButtonReleaseEvent& event);
    virtual bool onScroll(const Core::ScrollEvent& event);
    virtual bool onKeyDown(const Core::KeyPressEvent& event);
    virtual bool onKeyUp(const Core::KeyReleaseEvent& event);
    virtual bool onCharInput(const Core::CharInputEvent& event);
    virtual bool onFocusGained();
    virtual bool onFocusLost();

    // Callbacks
    std::function<void()> onClick_;
    std::function<void()> onHover_;
    std::function<void()> onFocus_;
    std::function<void()> onBlur_;

    // Animations
    void startAnimation(const Animation& animation);
    void stopAnimation(const std::string& property);
    void stopAllAnimations();

    // Accessibility
    void setAccessibleName(const std::string& name) { accessibleName_ = name; }
    void setAccessibleDescription(const std::string& desc) { accessibleDescription_ = desc; }

protected:
    virtual void onBoundsChanged(const Rect& oldBounds, const Rect& newBounds) {}
    virtual void onStyleChanged() {}
    virtual void onChildAdded(Ptr child) {}
    virtual void onChildRemoved(Ptr child) {}

    void setState(WidgetState state);

    std::string id_;
    std::vector<std::string> classes_;
    Rect bounds_;
    SizeConstraints constraints_;
    EdgeInsets margin_;
    HAlignment hAlignment_ = HAlignment::Left;
    VAlignment vAlignment_ = VAlignment::Top;

    Widget* parent_ = nullptr;
    std::vector<Ptr> children_;

    WidgetStyle style_;
    WidgetState state_ = WidgetState::Normal;

    bool visible_ = true;
    bool enabled_ = true;
    bool dirty_ = true;
    bool layoutDirty_ = true;

    FocusPolicy focusPolicy_ = FocusPolicy::NoFocus;
    CursorType cursor_ = CursorType::Arrow;
    std::string tooltip_;

    std::string accessibleName_;
    std::string accessibleDescription_;

    std::vector<Animation> animations_;
};

// Container widget
class Container : public Widget {
public:
    using Ptr = std::shared_ptr<Container>;

    Container();
    ~Container() override;

    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;
    void arrange(const Rect& finalRect) override;

protected:
    virtual void layoutChildren(const Rect& contentRect);
};

} // namespace NexusForge::UI
