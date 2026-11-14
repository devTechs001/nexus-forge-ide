// ui/framework/widget_system.cpp
#include "widget_system.hpp"
#include <algorithm>
#include <cmath>

namespace NexusForge::UI {

// Widget implementation
Widget::Widget() = default;

Widget::~Widget() {
    for (auto& child : children_) {
        if (child) {
            child->parent_ = nullptr;
        }
    }
}

void Widget::setPosition(float x, float y) {
    Rect oldBounds = bounds_;
    bounds_.x = x;
    bounds_.y = y;
    if (oldBounds != bounds_) {
        onBoundsChanged(oldBounds, bounds_);
        markDirty();
    }
}

void Widget::setSize(float width, float height) {
    Rect oldBounds = bounds_;
    bounds_.width = width;
    bounds_.height = height;
    if (oldBounds != bounds_) {
        onBoundsChanged(oldBounds, bounds_);
        markDirty();
    }
}

void Widget::setBounds(const Rect& bounds) {
    Rect oldBounds = bounds_;
    bounds_ = bounds;
    if (oldBounds != bounds_) {
        onBoundsChanged(oldBounds, bounds_);
        markDirty();
    }
}

void Widget::setConstraints(const SizeConstraints& constraints) {
    constraints_ = constraints;
    invalidateLayout();
}

Rect Widget::getGlobalBounds() const {
    Rect global = bounds_;
    const Widget* parent = parent_;
    while (parent) {
        global.x += parent->bounds_.x;
        global.y += parent->bounds_.y;
        parent = parent->parent_;
    }
    return global;
}

void Widget::setAlignment(HAlignment h, VAlignment v) {
    hAlignment_ = h;
    vAlignment_ = v;
    invalidateLayout();
}

bool Widget::isVisibleInHierarchy() const {
    if (!visible_) return false;
    const Widget* parent = parent_;
    while (parent) {
        if (!parent->visible_) return false;
        parent = parent->parent_;
    }
    return true;
}

void Widget::requestFocus() {
    if (focusPolicy_ != FocusPolicy::NoFocus) {
        onFocusGained();
    }
}

void Widget::clearFocus() {
    onFocusLost();
}

bool Widget::hasFocus() const {
    return state_ == WidgetState::Focused;
}

void Widget::setParent(Widget* parent) {
    parent_ = parent;
}

void Widget::addChild(Ptr child) {
    if (!child) return;
    children_.push_back(child);
    child->setParent(this);
    onChildAdded(child);
    invalidateLayout();
}

void Widget::removeChild(Ptr child) {
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        (*it)->setParent(nullptr);
        onChildRemoved(*it);
        children_.erase(it);
        invalidateLayout();
    }
}

void Widget::removeChild(const std::string& id) {
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if (*it && (*it)->getId() == id) {
            (*it)->setParent(nullptr);
            onChildRemoved(*it);
            children_.erase(it);
            invalidateLayout();
            break;
        }
    }
}

void Widget::removeFromParent() {
    if (parent_) {
        parent_->removeChild(shared_from_this());
    }
}

Widget* Widget::findChild(const std::string& id) const {
    for (const auto& child : children_) {
        if (child && child->getId() == id) {
            return child.get();
        }
    }
    return nullptr;
}

void Widget::render(RenderEngine& renderer) {
    if (!isVisibleInHierarchy()) return;

    // Render background
    if (style_.backgroundColor.isGradient) {
        // Draw gradient background
        Gradient gradient;
        gradient.type = GradientType::Linear;
        for (const auto& stop : style_.backgroundColor.stops) {
            gradient.stops.push_back({stop.position, Color(stop.r, stop.g, stop.b, stop.a)});
        }
        gradient.startPoint = {bounds_.x, bounds_.y};
        gradient.endPoint = {bounds_.x + bounds_.width, bounds_.y + bounds_.height};
        
        DrawCommand cmd;
        cmd.type = DrawCommandType::DrawGradient;
        cmd.rect = bounds_;
        cmd.gradient = gradient;
        cmd.useGradient = true;
        // renderer.executeCommand(cmd);
    } else {
        auto color = getCurrentBackgroundColor();
        if (color.a > 0) {
            if (style_.borderRadius > 0) {
                renderer.drawRoundedRect(bounds_, color, style_.borderRadius);
            } else {
                renderer.drawRect(bounds_, color);
            }
        }
    }

    // Render border
    if (style_.borderWidth > 0) {
        auto borderColor = getCurrentForegroundColor();
        renderer.drawRoundedRect(bounds_, borderColor, style_.borderRadius);
    }

    renderChildren(renderer);
}

void Widget::renderChildren(RenderEngine& renderer) {
    for (auto& child : children_) {
        if (child) {
            child->render(renderer);
        }
    }
}

Vec2 Widget::measure(float availableWidth, float availableHeight) {
    float width = constraints_.preferredWidth >= 0 ? 
                  constraints_.preferredWidth : availableWidth;
    float height = constraints_.preferredHeight >= 0 ? 
                   constraints_.preferredHeight : availableHeight;

    width = std::clamp(width, constraints_.minWidth, 
                       std::min(constraints_.maxWidth, availableWidth));
    height = std::clamp(height, constraints_.minHeight, 
                        std::min(constraints_.maxHeight, availableHeight));

    return Vec2(width, height);
}

void Widget::arrange(const Rect& finalRect) {
    Rect oldBounds = bounds_;
    bounds_ = finalRect;
    if (oldBounds != bounds_) {
        onBoundsChanged(oldBounds, bounds_);
    }
    markDirty();
}

void Widget::invalidateLayout() {
    layoutDirty_ = true;
    markDirty();
    if (parent_) {
        parent_->invalidateLayout();
    }
}

void Widget::update(double deltaTime) {
    updateAnimations(deltaTime);
    for (auto& child : children_) {
        if (child) {
            child->update(deltaTime);
        }
    }
}

void Widget::updateAnimations(double deltaTime) {
    for (auto it = animations_.begin(); it != animations_.end();) {
        it->elapsed += deltaTime;
        float t = std::min(it->elapsed / it->duration, 1.0f);
        
        if (it->easingFunction) {
            t = it->easingFunction(t);
        }
        
        float value = it->startValue + (it->endValue - it->startValue) * t;
        // Apply animation value to property
        
        if (t >= 1.0f) {
            if (it->onComplete) {
                it->onComplete();
            }
            if (it->loop) {
                it->elapsed = 0;
                if (it->pingPong) {
                    std::swap(it->startValue, it->endValue);
                }
            } else {
                it = animations_.erase(it);
                continue;
            }
        }
        ++it;
    }
}

bool Widget::handleEvent(Core::Event& event) {
    if (!isEnabled()) return false;

    // Try children first (bottom-up)
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if (*it && (*it)->handleEvent(event)) {
            return true;
        }
    }

    return false;
}

bool Widget::onMouseEnter(const Core::MouseMoveEvent& event) {
    setState(WidgetState::Hovered);
    if (onHover_) onHover_();
    return true;
}

bool Widget::onMouseLeave(const Core::MouseMoveEvent& event) {
    if (state_ == WidgetState::Hovered) {
        setState(WidgetState::Normal);
    }
    return true;
}

bool Widget::onMouseMove(const Core::MouseMoveEvent& event) {
    if (dragging_ && onDrag_) {
        Vec2 delta = Vec2(event.x, event.y) - dragStartPos_;
        onDrag_(delta);
    }
    return false;
}

bool Widget::onMouseDown(const Core::MouseButtonPressEvent& event) {
    setState(WidgetState::Pressed);
    mouseDown_ = true;
    dragStartPos_ = Vec2(event.x, event.y);
    
    if (focusPolicy_ == FocusPolicy::ClickFocus) {
        requestFocus();
    }
    
    return true;
}

bool Widget::onMouseUp(const Core::MouseButtonReleaseEvent& event) {
    if (mouseDown_) {
        setState(WidgetState::Hovered);
        mouseDown_ = false;
        if (dragging_ && onDragEnd_) {
            onDragEnd_(Vec2(event.x, event.y) - dragStartPos_);
            dragging_ = false;
        }
    }
    return false;
}

bool Widget::onClick(const Core::MouseButtonReleaseEvent& event) {
    if (onClick_) {
        onClick_();
    }
    return true;
}

bool Widget::onDoubleClick(const Core::MouseButtonPressEvent& event) {
    if (onDoubleClick_) {
        onDoubleClick_();
    }
    return true;
}

bool Widget::onScroll(const Core::ScrollEvent& event) {
    return false;
}

bool Widget::onKeyDown(const Core::KeyPressEvent& event) {
    return false;
}

bool Widget::onKeyUp(const Core::KeyReleaseEvent& event) {
    return false;
}

bool Widget::onCharInput(const Core::CharInputEvent& event) {
    return false;
}

bool Widget::onFocusGained() {
    setState(WidgetState::Focused);
    if (onFocus_) onFocus_();
    return true;
}

bool Widget::onFocusLost() {
    if (state_ == WidgetState::Focused) {
        setState(WidgetState::Normal);
    }
    if (onBlur_) onBlur_();
    return true;
}

bool Widget::onTouchStart(const Core::TouchEvent& event) {
    return onMouseDown(Core::MouseButtonPressEvent(0, event.touches[0].x, event.touches[0].y));
}

bool Widget::onTouchMove(const Core::TouchEvent& event) {
    return onMouseMove(Core::MouseMoveEvent(event.touches[0].x, event.touches[0].y));
}

bool Widget::onTouchEnd(const Core::TouchEvent& event) {
    return onMouseUp(Core::MouseButtonReleaseEvent(0, event.touches[0].x, event.touches[0].y));
}

bool Widget::onGesture(const Core::GestureEvent& event) {
    return false;
}

void Widget::startAnimation(const Animation& animation) {
    animations_.push_back(animation);
}

void Widget::stopAnimation(const std::string& property) {
    animations_.erase(
        std::remove_if(animations_.begin(), animations_.end(),
            [&property](const Animation& a) { return a.property == property; }),
        animations_.end()
    );
}

void Widget::stopAllAnimations() {
    animations_.clear();
}

void Widget::setState(WidgetState state) {
    if (state_ != state) {
        state_ = state;
        markDirty();
        onStyleChanged();
    }
}

Color Widget::getCurrentBackgroundColor() const {
    Color base;
    
    if (!enabled_ && style_.disabledColor.stops.size() > 0) {
        const auto& s = style_.disabledColor.stops[0];
        base = Color(s.r, s.g, s.b, s.a);
    } else if (state_ == WidgetState::Pressed && style_.pressedColor.stops.size() > 0) {
        const auto& s = style_.pressedColor.stops[0];
        base = Color(s.r, s.g, s.b, s.a);
    } else if (state_ == WidgetState::Hovered && style_.hoverColor.stops.size() > 0) {
        const auto& s = style_.hoverColor.stops[0];
        base = Color(s.r, s.g, s.b, s.a);
    } else if (style_.backgroundColor.stops.size() > 0) {
        const auto& s = style_.backgroundColor.stops[0];
        base = Color(s.r, s.g, s.b, s.a);
    } else {
        base = Color(0, 0, 0, 0);
    }
    
    return base;
}

Color Widget::getCurrentForegroundColor() const {
    if (!enabled_ && style_.disabledColor.stops.size() > 0) {
        const auto& s = style_.disabledColor.stops[0];
        return Color(s.r, s.g, s.b, s.a);
    }
    if (style_.foregroundColor.stops.size() > 0) {
        const auto& s = style_.foregroundColor.stops[0];
        return Color(s.r, s.g, s.b, s.a);
    }
    return Color::White();
}

// Container implementation
Container::Container() = default;

Container::~Container() = default;

void Container::render(RenderEngine& renderer) {
    Widget::render(renderer);
}

Vec2 Container::measure(float availableWidth, float availableHeight) {
    float width = constraints_.minWidth;
    float height = constraints_.minHeight;

    // Measure all children
    for (const auto& child : children_) {
        if (child) {
            Vec2 childSize = child->measure(availableWidth - style_.padding.horizontal(),
                                            availableHeight - style_.padding.vertical());
            
            if (layoutDirection_ == LayoutDirection::LeftToRight ||
                layoutDirection_ == LayoutDirection::RightToLeft) {
                width += childSize.x;
                height = std::max(height, childSize.y);
            } else {
                width = std::max(width, childSize.x);
                height += childSize.y;
            }
        }
    }

    // Add padding
    width += style_.padding.horizontal();
    height += style_.padding.vertical();

    // Add spacing
    if (children_.size() > 1) {
        if (layoutDirection_ == LayoutDirection::LeftToRight ||
            layoutDirection_ == LayoutDirection::RightToLeft) {
            width += spacing_ * (children_.size() - 1);
        } else {
            height += spacing_ * (children_.size() - 1);
        }
    }

    // Apply constraints
    width = std::clamp(width, constraints_.minWidth, constraints_.maxWidth);
    height = std::clamp(height, constraints_.minHeight, constraints_.maxHeight);

    return Vec2(width, height);
}

void Container::arrange(const Rect& finalRect) {
    Widget::arrange(finalRect);
    layoutChildren(finalRect);
}

void Container::layoutChildren(const Rect& contentRect) {
    float x = contentRect.x + style_.padding.left;
    float y = contentRect.y + style_.padding.top;
    float availableWidth = contentRect.width - style_.padding.horizontal();
    float availableHeight = contentRect.height - style_.padding.vertical();

    for (auto& child : children_) {
        if (!child) continue;

        Vec2 childSize = child->measure(availableWidth, availableHeight);
        Rect childBounds(x, y, childSize.x, childSize.y);

        // Apply alignment
        switch (hAlignment_) {
            case HAlignment::Center:
                childBounds.x = contentRect.x + (contentRect.width - childSize.x) / 2;
                break;
            case HAlignment::Right:
                childBounds.x = contentRect.x + contentRect.width - childSize.x - style_.padding.right;
                break;
            case HAlignment::Stretch:
                childBounds.width = availableWidth;
                break;
            default:
                break;
        }

        switch (vAlignment_) {
            case VAlignment::Center:
                childBounds.y = contentRect.y + (contentRect.height - childSize.y) / 2;
                break;
            case VAlignment::Bottom:
                childBounds.y = contentRect.y + contentRect.height - childSize.y - style_.padding.bottom;
                break;
            case VAlignment::Stretch:
                childBounds.height = availableHeight;
                break;
            default:
                break;
        }

        child->arrange(childBounds);

        // Position next child
        if (layoutDirection_ == LayoutDirection::LeftToRight) {
            x += childSize.x + spacing_;
        } else if (layoutDirection_ == LayoutDirection::RightToLeft) {
            x -= childSize.x + spacing_;
        } else if (layoutDirection_ == LayoutDirection::TopToBottom) {
            y += childSize.y + spacing_;
        } else if (layoutDirection_ == LayoutDirection::BottomToTop) {
            y -= childSize.y + spacing_;
        }
    }
}

} // namespace NexusForge::UI
