// ui/framework/ui_core.hpp
#pragma once

#include "widget_system.hpp"
#include "layout_engine.hpp"
#include "theme_manager.hpp"
#include "../renderer/render_engine.hpp"
#include "../../core/engine/event_system.hpp"

#include <memory>
#include <vector>
#include <string>

namespace NexusForge::UI {

// UI Application context
struct UIContext {
    std::unique_ptr<RenderEngine> renderEngine;
    std::unique_ptr<LayoutEngine> layoutEngine;
    std::unique_ptr<ThemeManager> themeManager;
    
    Widget* rootWidget = nullptr;
    Widget* hoveredWidget = nullptr;
    Widget* focusedWidget = nullptr;
    Widget* captureWidget = nullptr;
    
    float dpiScale = 1.0f;
    float width = 1920;
    float height = 1080;
};

// UI Core
class UICore {
public:
    static UICore& getInstance();
    
    bool initialize(void* windowHandle, int width, int height);
    void shutdown();
    
    void render();
    void update(double deltaTime);
    void handleEvent(Core::Event& event);
    
    // Widget management
    void setRootWidget(Widget::Ptr root);
    Widget::Ptr getRootWidget() const { return rootWidget_; }
    
    // Context
    UIContext& getContext() { return context_; }
    const UIContext& getContext() const { return context_; }
    
    // Theme
    void setTheme(const std::string& themeId);
    const Theme* getCurrentTheme() const;
    
    // Layout
    void invalidateLayout();
    void performLayout();
    
    // Focus
    void setFocusedWidget(Widget::Ptr widget);
    Widget::Ptr getFocusedWidget() const { return context_.focusedWidget ? context_.focusedWidget->shared_from_this() : nullptr; }
    
    // Hit testing
    Widget::Ptr hitTest(float x, float y);
    
    // DPI
    float getDPIScale() const { return context_.dpiScale; }
    void setDPIScale(float scale);
    
    // Size
    void setSize(float width, float height);
    float getWidth() const { return context_.width; }
    float getHeight() const { return context_.height; }

private:
    UICore() = default;
    
    UIContext context_;
    Widget::Ptr rootWidget_;
    bool layoutDirty_ = true;
};

} // namespace NexusForge::UI
