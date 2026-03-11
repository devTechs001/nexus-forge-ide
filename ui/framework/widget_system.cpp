// ui/framework/widget_system.cpp
#include "widget_system.hpp"

namespace NexusForge::UI {

Widget::Widget() = default;

Widget::~Widget() {
    for (auto& child : children_) {
        child->setParent(nullptr);
    }
}

void Widget::setId(const std::string& id) {
    id_ = id;
}

const std::string& Widget::getId() const {
    return id_;
}

void Widget::addClass(const std::string& className) {
    if (std::find(classes_.begin(), classes_.end(), className) == classes_.end()) {
        classes_.push_back(className);
    }
}

void Widget::removeClass(const std::string& className) {
    classes_.erase(
        std::remove(classes_.begin(), classes_.end(), className),
        classes_.end()
    );
}

bool Widget::hasClass(const std::string& className) const {
    return std::find(classes_.begin(), classes_.end(), className) != classes_.end();
}

void Widget::setPosition(float x, float y) {
    bounds_.x = x;
    bounds_.y = y;
    markDirty();
}

void Widget::setSize(float width, float height) {
    bounds_.width = width;
    bounds_.height = height;
    markDirty();
}

void Widget::setBounds(const Rect& bounds) {
    bounds_ = bounds;
    markDirty();
}

void Widget::setConstraints(const SizeConstraints& constraints) {
    constraints_ = constraints;
    markDirty();
}

Rect Widget::getGlobalBounds() const {
    Rect global = bounds_;
    Widget* parent = parent_;
    while (parent) {
        global.x += parent->bounds_.x;
        global.y += parent->bounds_.y;
        parent = parent->parent_;
    }
    return global;
}

void Widget::setMargin(const EdgeInsets& margin) {
    margin_ = margin;
    markDirty();
}

void Widget::setAlignment(HAlignment h, VAlignment v) {
    hAlignment_ = h;
    vAlignment_ = v;
    markDirty();
}

void Widget::setVisible(bool visible) {
    visible_ = visible;
    markDirty();
}

void Widget::setEnabled(bool enabled) {
    enabled_ = enabled;
    if (!enabled) {
        clearFocus();
    }
    markDirty();
}

bool Widget::isVisibleInHierarchy() const {
    if (!visible_) return false;
    Widget* parent = parent_;
    while (parent) {
        if (!parent->visible_) return false;
        parent = parent->parent_;
    }
    return true;
}

void Widget::requestFocus() {
    if (focusPolicy_ != FocusPolicy::NoFocus && enabled_) {
        // Focus handling would be done by focus manager
    }
}

void Widget::clearFocus() {
    // Clear focus
}

bool Widget::hasFocus() const {
    // Check if widget has focus
    return false;
}

void Widget::setParent(Widget* parent) {
    parent_ = parent;
}

void Widget::addChild(Ptr child) {
    if (child) {
        child->setParent(this);
        children_.push_back(child);
        onChildAdded(child);
        markDirty();
    }
}

void Widget::removeChild(Ptr child) {
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        (*it)->setParent(nullptr);
        onChildRemoved(*it);
        children_.erase(it);
        markDirty();
    }
}

void Widget::removeChild(const std::string& id) {
    auto it = std::find_if(children_.begin(), children_.end(),
        [&id](const Ptr& child) { return child->getId() == id; });
    if (it != children_.end()) {
        (*it)->setParent(nullptr);
        onChildRemoved(*it);
        children_.erase(it);
        markDirty();
    }
}

void Widget::removeFromParent() {
    if (parent_) {
        parent_->removeChild(shared_from_this());
    }
}

Widget* Widget::findChild(const std::string& id) const {
    for (const auto& child : children_) {
        if (child->getId() == id) {
            return child.get();
        }
        Widget* found = child->findChild(id);
        if (found) return found;
    }
    return nullptr;
}

void Widget::render(RenderEngine& renderer) {
    if (!isVisibleInHierarchy()) return;

    renderer.pushTransform();
    renderer.translate(bounds_.x, bounds_.y);

    // Render background
    if (style_.backgroundColor.a > 0) {
        renderer.drawRect(Rect(0, 0, bounds_.width, bounds_.height), style_.backgroundColor);
    }

    // Render border
    if (style_.borderWidth > 0) {
        // Border rendering
    }

    renderer.popTransform();

    renderChildren(renderer);
}

void Widget::renderChildren(RenderEngine& renderer) {
    for (auto& child : children_) {
        if (child->isVisibleInHierarchy()) {
            child->render(renderer);
        }
    }
}

Vec2 Widget::measure(float availableWidth, float availableHeight) {
    float width = bounds_.width;
    float height = bounds_.height;

    // Apply constraints
    if (constraints_.preferredWidth >= 0) {
        width = constraints_.preferredWidth;
    }
    if (constraints_.preferredHeight >= 0) {
        height = constraints_.preferredHeight;
    }

    width = std::clamp(width, constraints_.minWidth, constraints_.maxWidth);
    height = std::clamp(height, constraints_.minHeight, constraints_.maxHeight);

    return Vec2(width, height);
}

void Widget::arrange(const Rect& finalRect) {
    bounds_ = finalRect;
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
        child->update(deltaTime);
    }
}

void Widget::updateAnimations(double deltaTime) {
    for (auto it = animations_.begin(); it != animations_.end();) {
        it->elapsed += static_cast<float>(deltaTime);

        float progress = it->elapsed / it->duration;
        if (progress >= 1.0f) {
            progress = 1.0f;
            if (it->onComplete) {
                it->onComplete();
            }
            it = animations_.erase(it);
        } else {
            float easedProgress = it->easingFunction ? it->easingFunction(progress) : progress;
            // Apply animation value
            ++it;
        }
    }
}

bool Widget::handleEvent(Core::Event& event) {
    // Check children first (bottom-up event propagation)
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if ((*it)->isVisibleInHierarchy() && (*it)->handleEvent(event)) {
            return true;
        }
    }

    // Then handle at this level
    if (!isEnabled()) return false;

    if (auto* mouseEvent = dynamic_cast<Core::MouseMoveEvent*>(&event)) {
        return onMouseMove(*mouseEvent);
    }
    if (auto* mouseEvent = dynamic_cast<Core::MouseButtonPressEvent*>(&event)) {
        return onMouseDown(*mouseEvent);
    }
    if (auto* mouseEvent = dynamic_cast<Core::MouseButtonReleaseEvent*>(&event)) {
        return onMouseUp(*mouseEvent);
    }
    if (auto* scrollEvent = dynamic_cast<Core::ScrollEvent*>(&event)) {
        return onScroll(*scrollEvent);
    }
    if (auto* keyEvent = dynamic_cast<Core::KeyPressEvent*>(&event)) {
        return onKeyDown(*keyEvent);
    }
    if (auto* keyEvent = dynamic_cast<Core::KeyReleaseEvent*>(&event)) {
        return onKeyUp(*keyEvent);
    }
    if (auto* charEvent = dynamic_cast<Core::CharInputEvent*>(&event)) {
        return onCharInput(*charEvent);
    }

    return false;
}

bool Widget::onMouseEnter(const Core::MouseMoveEvent& event) {
    setState(WidgetState::Hovered);
    if (onHover_) onHover_();
    return false;
}

bool Widget::onMouseLeave(const Core::MouseMoveEvent& event) {
    if (state_ == WidgetState::Hovered) {
        setState(WidgetState::Normal);
    }
    return false;
}

bool Widget::onMouseMove(const Core::MouseMoveEvent& event) {
    if (dragging_ && onDrag_) {
        onDrag_(Vec2(event.x, event.y));
    }
    return false;
}

bool Widget::onMouseDown(const Core::MouseButtonPressEvent& event) {
    if (bounds_.contains(static_cast<float>(event.x), static_cast<float>(event.y))) {
        requestFocus();
        setState(WidgetState::Pressed);
        dragging_ = true;
        dragStartPos_ = Vec2(static_cast<float>(event.x), static_cast<float>(event.y));
    }
    return false;
}

bool Widget::onMouseUp(const Core::MouseButtonReleaseEvent& event) {
    setState(WidgetState::Normal);
    dragging_ = false;
    if (onDragEnd_) {
        onDragEnd_(Vec2(static_cast<float>(event.x), static_cast<float>(event.y)));
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
    return false;
}

bool Widget::onTouchMove(const Core::TouchEvent& event) {
    return false;
}

bool Widget::onTouchEnd(const Core::TouchEvent& event) {
    return false;
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
    }
}

Color Widget::getCurrentBackgroundColor() const {
    if (!enabled_) return style_.disabledColor;
    switch (state_) {
        case WidgetState::Hovered: return style_.hoverColor;
        case WidgetState::Pressed: return style_.pressedColor;
        default: return style_.backgroundColor;
    }
}

Color Widget::getCurrentForegroundColor() const {
    if (!enabled_) return style_.disabledColor;
    return style_.foregroundColor;
}

// Container implementation
Container::Container() = default;

Container::~Container() = default;

void Container::render(RenderEngine& renderer) {
    Widget::render(renderer);
}

Vec2 Container::measure(float availableWidth, float availableHeight) {
    return Widget::measure(availableWidth, availableHeight);
}

void Container::arrange(const Rect& finalRect) {
    Widget::arrange(finalRect);
    layoutChildren(finalRect);
}

void Container::layoutChildren(const Rect& contentRect) {
    // Default layout - children positioned absolutely
    for (auto& child : children_) {
        child->arrange(child->getBounds());
    }
}

} // namespace NexusForge::UI
