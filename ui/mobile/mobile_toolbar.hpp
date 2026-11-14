// ui/mobile/mobile_toolbar.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include "../renderer/render_engine.hpp"

#include <vector>
#include <string>
#include <functional>

namespace NexusForge::UI::Mobile {

// Toolbar action button
struct ToolbarAction {
    std::string id;
    std::string icon;
    std::string tooltip;
    bool enabled = true;
    bool hovered = false;
    std::function<void()> onClick;
};

// Mobile toolbar class
class MobileToolbar : public Widget {
public:
    MobileToolbar();
    ~MobileToolbar();

    // Title
    void setTitle(const std::string& title);
    void setSubtitle(const std::string& subtitle);
    const std::string& getTitle() const { return title_; }
    const std::string& getSubtitle() const { return subtitle_; }

    // Actions
    void setActions(const std::vector<ToolbarAction>& actions);
    const std::vector<ToolbarAction>& getActions() const { return actions_; }

    // Navigation buttons
    void setBackVisible(bool visible);
    void setMenuVisible(bool visible);
    void setSearchVisible(bool visible);
    bool isBackVisible() const { return showBackButton_; }
    bool isMenuVisible() const { return showMenuButton_; }
    bool isSearchVisible() const { return showSearchButton_; }

    // Collapsed state
    void setCollapsed(bool collapsed);
    void toggleCollapsed();
    bool isCollapsed() const { return collapsed_; }

    // Size
    void setHeight(float height);
    float getHeight() const;

    // Rendering
    void render(RenderEngine& renderer) override;

    // Measurement
    Vec2 measure(float availableWidth, float availableHeight);

    // Event handling
    bool onMouseMove(const Core::MouseMoveEvent& event) override;
    bool onMouseLeave(const Core::MouseMoveEvent& event) override;
    bool onMouseDown(const Core::MouseButtonPressEvent& event) override;

    // Callbacks
    std::function<void()> onBack;
    std::function<void()> onMenu;
    std::function<void()> onSearch;
    std::function<void()> onTitleClick;

private:
    std::string title_;
    std::string subtitle_;
    std::vector<ToolbarAction> actions_;

    bool showBackButton_ = false;
    bool showMenuButton_ = true;
    bool showSearchButton_ = true;
    bool collapsed_ = false;
    float collapseProgress_ = 0.0f;

    float height_ = 56;

    // Hover states
    bool backButtonHovered_ = false;
    bool menuButtonHovered_ = false;
    bool searchButtonHovered_ = false;
};

} // namespace NexusForge::UI::Mobile
