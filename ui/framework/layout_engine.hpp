// ui/framework/layout_engine.hpp
#pragma once

#include "widget_system.hpp"
#include <memory>
#include <vector>
#include <string>

namespace NexusForge::UI {

// Flex layout properties
struct FlexProperties {
    float flexGrow = 0;
    float flexShrink = 1;
    float flexBasis = -1;
    HAlignment alignSelf = HAlignment::Stretch;
    VAlignment alignSelfV = VAlignment::Stretch;
};

// Grid layout properties
struct GridProperties {
    int gridColumnStart = 0;
    int gridColumnEnd = 1;
    int gridRowStart = 0;
    int gridRowEnd = 1;
};

// Layout Engine
class LayoutEngine {
public:
    LayoutEngine();
    ~LayoutEngine();

    // Layout calculation
    void layout(Widget* root, float availableWidth, float availableHeight);
    
    // Flexbox layout
    void layoutFlex(Container* container, const Rect& bounds);
    
    // Grid layout
    void layoutGrid(Container* container, const Rect& bounds);
    
    // Absolute layout
    void layoutAbsolute(Container* container, const Rect& bounds);
    
    // Measure widget
    Vec2 measureWidget(Widget* widget, float availableWidth, float availableHeight);
    
    // Apply constraints
    Vec2 applyConstraints(const Vec2& size, const SizeConstraints& constraints,
                          float availableWidth, float availableHeight);

    // Flex properties
    void setFlexProperties(Widget* widget, const FlexProperties& props);
    FlexProperties getFlexProperties(Widget* widget) const;
    
    // Grid properties
    void setGridProperties(Widget* widget, const GridProperties& props);
    GridProperties getGridProperties(Widget* widget) const;

private:
    std::unordered_map<Widget*, FlexProperties> flexProps_;
    std::unordered_map<Widget*, GridProperties> gridProps_;
    
    void layoutChildrenHorizontal(Container* container, const Rect& bounds, bool leftToRight);
    void layoutChildrenVertical(Container* container, const Rect& bounds, bool topToBottom);
};

} // namespace NexusForge::UI
