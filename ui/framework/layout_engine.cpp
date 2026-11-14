// ui/framework/layout_engine.cpp
#include "layout_engine.hpp"
#include <algorithm>
#include <cmath>

namespace NexusForge::UI {

LayoutEngine::LayoutEngine() = default;

LayoutEngine::~LayoutEngine() = default;

void LayoutEngine::layout(Widget* root, float availableWidth, float availableHeight) {
    if (!root) return;
    
    // Measure phase
    Vec2 size = measureWidget(root, availableWidth, availableHeight);
    
    // Arrange phase
    Rect bounds(0, 0, size.x, size.y);
    root->arrange(bounds);
}

Vec2 LayoutEngine::measureWidget(Widget* widget, float availableWidth, float availableHeight) {
    if (!widget) return Vec2(0, 0);
    
    auto* container = dynamic_cast<Container*>(widget);
    if (container) {
        return container->measure(availableWidth, availableHeight);
    }
    
    return widget->measure(availableWidth, availableHeight);
}

Vec2 LayoutEngine::applyConstraints(const Vec2& size, const SizeConstraints& constraints,
                                     float availableWidth, float availableHeight) {
    float width = size.x;
    float height = size.y;
    
    // Apply preferred size
    if (constraints.preferredWidth >= 0) {
        width = constraints.preferredWidth;
    }
    if (constraints.preferredHeight >= 0) {
        height = constraints.preferredHeight;
    }
    
    // Apply min/max constraints
    width = std::clamp(width, constraints.minWidth, 
                       std::min(constraints.maxWidth, availableWidth));
    height = std::clamp(height, constraints.minHeight, 
                        std::min(constraints.maxHeight, availableHeight));
    
    return Vec2(width, height);
}

void LayoutEngine::layoutFlex(Container* container, const Rect& bounds) {
    if (!container) return;
    
    float x = bounds.x + container->getStyle().padding.left;
    float y = bounds.y + container->getStyle().padding.top;
    float availableWidth = bounds.width - container->getStyle().padding.horizontal();
    float availableHeight = bounds.height - container->getStyle().padding.vertical();
    
    auto direction = container->getLayoutDirection();
    float spacing = container->getSpacing();
    
    if (direction == LayoutDirection::LeftToRight || 
        direction == LayoutDirection::RightToLeft) {
        layoutChildrenHorizontal(container, bounds, 
                                 direction == LayoutDirection::LeftToRight);
    } else {
        layoutChildrenVertical(container, bounds,
                               direction == LayoutDirection::TopToBottom);
    }
}

void LayoutEngine::layoutChildrenHorizontal(Container* container, const Rect& bounds, 
                                             bool leftToRight) {
    float x = bounds.x + container->getStyle().padding.left;
    float availableWidth = bounds.width - container->getStyle().padding.horizontal();
    float availableHeight = bounds.height - container->getStyle().padding.vertical();
    float spacing = container->getSpacing();
    
    // Calculate total flex grow
    float totalFlexGrow = 0;
    float totalFixedSize = 0;
    
    for (const auto& child : container->getChildren()) {
        if (!child) continue;
        
        auto props = getFlexProperties(child.get());
        if (props.flexGrow > 0) {
            totalFlexGrow += props.flexGrow;
        } else {
            Vec2 childSize = child->measure(availableWidth, availableHeight);
            totalFixedSize += childSize.x;
        }
    }
    
    // Add spacing
    if (container->getChildren().size() > 1) {
        totalFixedSize += spacing * (container->getChildren().size() - 1);
    }
    
    float remainingSpace = std::max(0.0f, availableWidth - totalFixedSize);
    float flexUnit = totalFlexGrow > 0 ? remainingSpace / totalFlexGrow : 0;
    
    // Layout children
    if (leftToRight) {
        x = bounds.x + container->getStyle().padding.left;
    } else {
        x = bounds.x + bounds.width - container->getStyle().padding.right;
    }
    
    for (const auto& child : container->getChildren()) {
        if (!child) continue;
        
        auto props = getFlexProperties(child.get());
        Vec2 childSize = child->measure(availableWidth, availableHeight);
        
        // Apply flex grow
        if (props.flexGrow > 0 && totalFlexGrow > 0) {
            childSize.x = props.flexGrow * flexUnit;
        }
        
        float childHeight = childSize.y;
        
        // Apply alignment
        float y = bounds.y + container->getStyle().padding.top;
        if (props.alignSelfV == VAlignment::Center) {
            y = bounds.y + (bounds.height - childHeight) / 2;
        } else if (props.alignSelfV == VAlignment::Bottom) {
            y = bounds.y + bounds.height - childHeight - container->getStyle().padding.bottom;
        }
        
        Rect childBounds;
        if (leftToRight) {
            childBounds = Rect(x, y, childSize.x, childHeight);
            x += childSize.x + spacing;
        } else {
            x -= childSize.x + spacing;
            childBounds = Rect(x, y, childSize.x, childHeight);
        }
        
        child->arrange(childBounds);
    }
}

void LayoutEngine::layoutChildrenVertical(Container* container, const Rect& bounds,
                                           bool topToBottom) {
    float y = bounds.y + container->getStyle().padding.top;
    float availableHeight = bounds.height - container->getStyle().padding.vertical();
    float availableWidth = bounds.width - container->getStyle().padding.horizontal();
    float spacing = container->getSpacing();
    
    // Calculate total flex grow
    float totalFlexGrow = 0;
    float totalFixedSize = 0;
    
    for (const auto& child : container->getChildren()) {
        if (!child) continue;
        
        auto props = getFlexProperties(child.get());
        if (props.flexGrow > 0) {
            totalFlexGrow += props.flexGrow;
        } else {
            Vec2 childSize = child->measure(availableWidth, availableHeight);
            totalFixedSize += childSize.y;
        }
    }
    
    // Add spacing
    if (container->getChildren().size() > 1) {
        totalFixedSize += spacing * (container->getChildren().size() - 1);
    }
    
    float remainingSpace = std::max(0.0f, availableHeight - totalFixedSize);
    float flexUnit = totalFlexGrow > 0 ? remainingSpace / totalFlexGrow : 0;
    
    // Layout children
    if (topToBottom) {
        y = bounds.y + container->getStyle().padding.top;
    } else {
        y = bounds.y + bounds.height - container->getStyle().padding.bottom;
    }
    
    for (const auto& child : container->getChildren()) {
        if (!child) continue;
        
        auto props = getFlexProperties(child.get());
        Vec2 childSize = child->measure(availableWidth, availableHeight);
        
        // Apply flex grow
        if (props.flexGrow > 0 && totalFlexGrow > 0) {
            childSize.y = props.flexGrow * flexUnit;
        }
        
        float childWidth = childSize.x;
        
        // Apply alignment
        float x = bounds.x + container->getStyle().padding.left;
        if (props.alignSelf == HAlignment::Center) {
            x = bounds.x + (bounds.width - childWidth) / 2;
        } else if (props.alignSelf == HAlignment::Right) {
            x = bounds.x + bounds.width - childWidth - container->getStyle().padding.right;
        }
        
        Rect childBounds;
        if (topToBottom) {
            childBounds = Rect(x, y, childWidth, childSize.y);
            y += childSize.y + spacing;
        } else {
            y -= childSize.y + spacing;
            childBounds = Rect(x, y, childWidth, childSize.y);
        }
        
        child->arrange(childBounds);
    }
}

void LayoutEngine::layoutGrid(Container* container, const Rect& bounds) {
    // Grid layout implementation
    // Would calculate grid columns and rows, then position children
}

void LayoutEngine::layoutAbsolute(Container* container, const Rect& bounds) {
    // Absolute layout - children position themselves
    for (const auto& child : container->getChildren()) {
        if (child) {
            Vec2 size = child->measure(bounds.width, bounds.height);
            Rect childBounds(bounds.x, bounds.y, size.x, size.y);
            child->arrange(childBounds);
        }
    }
}

void LayoutEngine::setFlexProperties(Widget* widget, const FlexProperties& props) {
    if (widget) {
        flexProps_[widget] = props;
    }
}

FlexProperties LayoutEngine::getFlexProperties(Widget* widget) const {
    auto it = flexProps_.find(widget);
    if (it != flexProps_.end()) {
        return it->second;
    }
    return FlexProperties();
}

void LayoutEngine::setGridProperties(Widget* widget, const GridProperties& props) {
    if (widget) {
        gridProps_[widget] = props;
    }
}

GridProperties LayoutEngine::getGridProperties(Widget* widget) const {
    auto it = gridProps_.find(widget);
    if (it != gridProps_.end()) {
        return it->second;
    }
    return GridProperties();
}

} // namespace NexusForge::UI
