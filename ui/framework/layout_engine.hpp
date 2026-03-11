// ui/framework/layout_engine.hpp
#pragma once

#include "widget_system.hpp"
#include <memory>
#include <vector>

namespace NexusForge::UI {

// Layout types
enum class LayoutType {
    Horizontal,
    Vertical,
    Grid,
    Flex,
    Stack,
    Absolute
};

// Flex direction
enum class FlexDirection {
    Row,
    RowReverse,
    Column,
    ColumnReverse
};

// Flex wrap
enum class FlexWrap {
    NoWrap,
    Wrap,
    WrapReverse
};

// Justify content
enum class JustifyContent {
    FlexStart,
    FlexEnd,
    Center,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly
};

// Align items
enum class AlignItems {
    Stretch,
    FlexStart,
    FlexEnd,
    Center,
    Baseline
};

// Align content
enum class AlignContent {
    FlexStart,
    FlexEnd,
    Center,
    SpaceBetween,
    SpaceAround,
    Stretch
};

// Layout properties
struct LayoutProperties {
    LayoutType type = LayoutType::Vertical;
    
    // Flex properties
    FlexDirection flexDirection = FlexDirection::Row;
    FlexWrap flexWrap = FlexWrap::NoWrap;
    JustifyContent justifyContent = JustifyContent::FlexStart;
    AlignItems alignItems = AlignItems::Stretch;
    AlignContent alignContent = AlignContent::Stretch;
    
    // Grid properties
    int gridColumns = 0;
    int gridRows = 0;
    float gridColumnGap = 0;
    float gridRowGap = 0;
    
    // Spacing
    EdgeInsets margin;
    EdgeInsets padding;
    
    // Size constraints
    float minWidth = 0;
    float minHeight = 0;
    float maxWidth = std::numeric_limits<float>::max();
    float maxHeight = std::numeric_limits<float>::max();
    
    // Flex grow/shrink
    float flexGrow = 0;
    float flexShrink = 1;
    float flexBasis = 0;
    
    // Position (for absolute)
    float left = 0;
    float top = 0;
    float right = 0;
    float bottom = 0;
    
    // Z-index
    int zIndex = 0;
    
    // Visibility
    bool visible = true;
    bool collapsed = false;
};

// Layout node
class LayoutNode {
public:
    using Ptr = std::shared_ptr<LayoutNode>;
    
    LayoutNode();
    explicit LayoutNode(Widget* widget);
    ~LayoutNode();
    
    Widget* getWidget() const { return widget_; }
    LayoutProperties& getProperties() { return properties_; }
    const LayoutProperties& getProperties() const { return properties_; }
    
    void addChild(Ptr child);
    void removeChild(Ptr child);
    const std::vector<Ptr>& getChildren() const { return children_; }
    
    // Calculated layout
    float getX() const { return x_; }
    float getY() const { return y_; }
    float getWidth() const { return width_; }
    float getHeight() const { return height_; }
    
    void setCalculatedBounds(float x, float y, float w, float h);
    
private:
    Widget* widget_ = nullptr;
    LayoutProperties properties_;
    std::vector<Ptr> children_;
    
    float x_ = 0, y_ = 0;
    float width_ = 0, height_ = 0;
};

// Layout engine
class LayoutEngine {
public:
    LayoutEngine();
    ~LayoutEngine();
    
    // Layout calculation
    void layout(Widget* root, float availableWidth, float availableHeight);
    void layoutNode(LayoutNode::Ptr node, float parentWidth, float parentHeight);
    
    // Measure
    Vec2 measure(Widget* widget, float availableWidth, float availableHeight);
    
    // Flexbox layout
    void layoutFlex(LayoutNode::Ptr node, float parentWidth, float parentHeight);
    void calculateFlexBasis(LayoutNode::Ptr node);
    void resolveFlexibleLengths(const std::vector<LayoutNode::Ptr>& items,
                                 float containerSize, float totalFlexGrow,
                                 float totalFlexShrink, float totalFlexBasis);
    
    // Grid layout
    void layoutGrid(LayoutNode::Ptr node, float parentWidth, float parentHeight);
    void calculateGridTracks(const std::vector<LayoutNode::Ptr>& items,
                             float containerWidth, float containerHeight);
    
    // Box layout (horizontal/vertical)
    void layoutBox(LayoutNode::Ptr node, float parentWidth, float parentHeight);
    
    // Stack layout
    void layoutStack(LayoutNode::Ptr node, float parentWidth, float parentHeight);
    
    // Absolute layout
    void layoutAbsolute(LayoutNode::Ptr node, float parentWidth, float parentHeight);
    
    // Helpers
    float getMarginLeft(const LayoutProperties& props) const;
    float getMarginRight(const LayoutProperties& props) const;
    float getMarginTop(const LayoutProperties& props) const;
    float getMarginBottom(const LayoutProperties& props) const;
    
    void applyLayout(Widget* widget, float x, float y, float width, float height);
    
private:
    std::vector<LayoutNode::Ptr> nodes_;
    
    LayoutNode::Ptr createNode(Widget* widget);
    void buildLayoutTree(Widget* widget, LayoutNode::Ptr parentNode);
};

} // namespace NexusForge::UI
