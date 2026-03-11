// ui/widgets/tab_bar.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include <string>
#include <vector>
#include <functional>

namespace NexusForge::UI {

// Tab info
struct TabInfo {
    std::string id;
    std::string title;
    std::string icon;
    bool closable = true;
    bool pinned = false;
    int badgeCount = 0;
    Widget::Ptr content;
};

// Tab bar widget
class TabBar : public Widget {
public:
    using Ptr = std::shared_ptr<TabBar>;

    TabBar();
    ~TabBar() override;

    // Tab management
    int addTab(const std::string& title, Widget::Ptr content = nullptr);
    int addTab(const TabInfo& tab);
    void removeTab(int index);
    void removeTab(const std::string& id);
    void clearTabs();

    int getTabCount() const { return static_cast<int>(tabs_.size()); }
    TabInfo* getTab(int index);
    const TabInfo* getTab(int index) const;
    TabInfo* getTab(const std::string& id);
    int getTabIndex(const std::string& id) const;

    // Selection
    void setCurrentTab(int index);
    void setCurrentTab(const std::string& id);
    int getCurrentTabIndex() const { return currentTabIndex_; }
    TabInfo* getCurrentTab();
    const TabInfo* getCurrentTab() const;

    // Tab properties
    void setTabTitle(int index, const std::string& title);
    void setTabIcon(int index, const std::string& icon);
    void setTabClosable(int index, bool closable);
    void setTabBadge(int index, int count);
    void setTabPinned(int index, bool pinned);

    // Appearance
    void setShowCloseButtons(bool show) { showCloseButtons_ = show; }
    void setShowAddButton(bool show) { showAddButton_ = show; }
    void setTabsClosable(bool closable) { tabsClosable_ = closable; }
    void setTabWidth(float width) { tabWidth_ = width; }
    void setMinTabWidth(float width) { minTabWidth_ = width; }
    void setMaxTabWidth(float width) { maxTabWidth_ = width; }

    // Callbacks
    std::function<void(int)> onTabSelected;
    std::function<void(int)> onTabClosed;
    std::function<void()> onAddButtonClicked;
    std::function<void(int, int)> onTabMoved;
    std::function<void(int)> onTabDoubleClicked;

    // Rendering
    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;

    // Events
    bool onMouseDown(const Core::MouseButtonPressEvent& event) override;
    bool onMouseMove(const Core::MouseMoveEvent& event) override;
    bool onScroll(const Core::ScrollEvent& event) override;

protected:
    void updateTabPositions();
    void ensureTabVisible(int index);
    Rect getTabBounds(int index) const;
    int getTabAtPosition(float x, float y) const;
    int getCloseButtonAtPosition(float x, float y) const;

    std::vector<TabInfo> tabs_;
    int currentTabIndex_ = -1;
    int hoveredTabIndex_ = -1;
    int hoveredCloseButton_ = -1;

    bool showCloseButtons_ = true;
    bool showAddButton_ = false;
    bool tabsClosable_ = true;

    float tabWidth_ = 0;  // 0 = auto
    float minTabWidth_ = 100;
    float maxTabWidth_ = 200;
    float tabHeight_ = 32;

    float scrollOffset_ = 0;
    bool dragging_ = false;
    int dragStartIndex_ = -1;
};

} // namespace NexusForge::UI
