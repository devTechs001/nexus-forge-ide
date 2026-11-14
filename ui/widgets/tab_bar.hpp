// ui/widgets/tab_bar.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include <string>
#include <vector>
#include <functional>

namespace NexusForge::UI {

// Tab item
struct TabItem {
    std::string id;
    std::string label;
    std::string icon;
    bool closable = true;
    void* userData = nullptr;
};

// Tab Bar widget
class TabBar : public Widget {
public:
    using Ptr = std::shared_ptr<TabBar>;

    TabBar();
    ~TabBar() override;

    // Tabs
    void addTab(const TabItem& tab);
    void removeTab(const std::string& id);
    void removeTab(size_t index);
    void clearTabs();
    
    // Selection
    void setSelectedTab(const std::string& id);
    void setSelectedTab(size_t index);
    size_t getSelectedIndex() const { return selectedIndex_; }
    std::string getSelectedTabId() const;
    TabItem* getSelectedTab();
    const TabItem* getSelectedTab() const;
    
    // Access
    size_t getTabCount() const { return tabs_.size(); }
    TabItem* getTab(size_t index);
    TabItem* getTab(const std::string& id);
    const TabItem* getTab(const std::string& id) const;

    // Options
    void setClosable(bool closable) { defaultClosable_ = closable; }
    bool isClosable() const { return defaultClosable_; }
    void setTabsClosable(bool closable);

    // Events
    std::function<void(size_t)> onTabSelected;
    std::function<void(size_t)> onTabClosed;
    std::function<void(size_t, size_t)> onTabMoved;

    // Widget overrides
    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;
    bool onMouseDown(const Core::MouseButtonPressEvent& event) override;
    bool onMouseMove(const Core::MouseMoveEvent& event) override;
    bool onMouseUp(const Core::MouseButtonReleaseEvent& event) override;

private:
    std::vector<TabItem> tabs_;
    size_t selectedIndex_ = 0;
    bool defaultClosable_ = true;

    int closeHoverIndex_ = -1;
    bool dragging_ = false;
    size_t dragStartIndex_ = 0;
    float dragOffset_ = 0;
    float lastMouseX_ = 0;
    float lastMouseY_ = 0;

    float tabWidth_ = 150;
    float minTabWidth_ = 100;
    float maxTabWidth_ = 200;
    float tabHeight_ = 32;

    void layoutTabs();
    float getTabX(size_t index) const;
    size_t getTabAtPosition(float x, float y) const;
    bool isCloseButtonAtPosition(float x, float y, size_t index) const;
};

} // namespace NexusForge::UI
