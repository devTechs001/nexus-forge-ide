// ui/widgets/tab_bar.cpp
#include "tab_bar.hpp"
#include <algorithm>
#include <iterator>

namespace NexusForge::UI {

TabBar::TabBar() {
    style_.backgroundColor = GradientColor::fromHex(0x1E1E2E);
    style_.foregroundColor = GradientColor::fromHex(0xA6ADC8);
}

TabBar::~TabBar() = default;

void TabBar::addTab(const TabItem& tab) {
    tabs_.push_back(tab);
    if (tabs_.size() == 1) {
        selectedIndex_ = 0;
    }
    layoutTabs();
}

void TabBar::removeTab(const std::string& id) {
    for (auto it = tabs_.begin(); it != tabs_.end(); ++it) {
        if (it->id == id) {
            size_t index = std::distance(tabs_.begin(), it);
            removeTab(index);
            return;
        }
    }
}

void TabBar::removeTab(size_t index) {
    if (index >= tabs_.size()) return;
    
    tabs_.erase(tabs_.begin() + index);
    
    if (selectedIndex_ >= tabs_.size()) {
        selectedIndex_ = tabs_.empty() ? 0 : tabs_.size() - 1;
    }
    
    if (onTabClosed) {
        onTabClosed(index);
    }
    
    layoutTabs();
}

void TabBar::clearTabs() {
    tabs_.clear();
    selectedIndex_ = 0;
}

void TabBar::setSelectedTab(const std::string& id) {
    for (size_t i = 0; i < tabs_.size(); ++i) {
        if (tabs_[i].id == id) {
            setSelectedTab(i);
            return;
        }
    }
}

void TabBar::setSelectedTab(size_t index) {
    if (index >= tabs_.size()) return;
    selectedIndex_ = index;
    if (onTabSelected) {
        onTabSelected(index);
    }
    markDirty();
}

std::string TabBar::getSelectedTabId() const {
    if (selectedIndex_ < tabs_.size()) {
        return tabs_[selectedIndex_].id;
    }
    return "";
}

TabItem* TabBar::getSelectedTab() {
    if (selectedIndex_ < tabs_.size()) {
        return &tabs_[selectedIndex_];
    }
    return nullptr;
}

const TabItem* TabBar::getSelectedTab() const {
    if (selectedIndex_ < tabs_.size()) {
        return &tabs_[selectedIndex_];
    }
    return nullptr;
}

TabItem* TabBar::getTab(size_t index) {
    if (index < tabs_.size()) {
        return &tabs_[index];
    }
    return nullptr;
}

TabItem* TabBar::getTab(const std::string& id) {
    for (auto& tab : tabs_) {
        if (tab.id == id) {
            return &tab;
        }
    }
    return nullptr;
}

const TabItem* TabBar::getTab(const std::string& id) const {
    for (const auto& tab : tabs_) {
        if (tab.id == id) {
            return &tab;
        }
    }
    return nullptr;
}

void TabBar::setTabsClosable(bool closable) {
    defaultClosable_ = closable;
    for (auto& tab : tabs_) {
        tab.closable = closable;
    }
}

void TabBar::render(RenderEngine& renderer) {
    if (!isVisibleInHierarchy()) return;

    float y = bounds_.y;
    
    // Draw tab bar background
    renderer.drawRect(bounds_, Color(0.12f, 0.12f, 0.18f, 1.0f));
    
    // Draw tabs
    for (size_t i = 0; i < tabs_.size(); ++i) {
        const TabItem& tab = tabs_[i];
        float tabX = getTabX(i);
        Rect tabRect(tabX, y, tabWidth_, tabHeight_);
        
        bool isSelected = (i == selectedIndex_);
        bool isHovered = (getTabAtPosition(lastMouseX_, lastMouseY_) == i);
        bool closeHovered = isCloseButtonAtPosition(lastMouseX_, lastMouseY_, i);
        
        // Draw tab background
        Color bgColor;
        if (isSelected) {
            bgColor = Color(0.18f, 0.18f, 0.28f, 1.0f);
        } else if (isHovered) {
            bgColor = Color(0.15f, 0.15f, 0.22f, 1.0f);
        } else {
            bgColor = Color(0.12f, 0.12f, 0.18f, 0.5f);
        }
        renderer.drawRect(tabRect, bgColor);
        
        // Draw tab separator
        if (!isSelected) {
            renderer.drawRect(Rect(tabX + tabWidth_ - 1, y + 4, 1, tabHeight_ - 8), 
                              Color(0.2f, 0.2f, 0.3f, 1.0f));
        }
        
        // Draw tab label
        float textX = tabX + 12;
        float textY = y + tabHeight_ / 2 + style_.fontSize / 3;
        Color textColor = isSelected ? Color(0.9f, 0.9f, 0.95f, 1.0f) : Color(0.6f, 0.6f, 0.7f, 1.0f);
        renderer.drawText(tab.label, textX, textY, textColor, style_.fontFamily, style_.fontSize);
        
        // Draw close button
        if (tab.closable) {
            float closeX = tabX + tabWidth_ - 24;
            float closeY = y + tabHeight_ / 2 - 5;
            Color closeColor = closeHovered ? Color(1, 0.4f, 0.4f, 1.0f) : Color(0.5f, 0.5f, 0.6f, 1.0f);
            renderer.drawText("×", closeX, closeY + 10, closeColor, style_.fontFamily, 14);
        }
        
        // Draw selected indicator
        if (isSelected) {
            renderer.drawRect(Rect(tabX, y + tabHeight_ - 2, tabWidth_, 2), 
                              Color(0.3f, 0.5f, 0.9f, 1.0f));
        }
    }
}

Vec2 TabBar::measure(float availableWidth, float availableHeight) {
    return Vec2(availableWidth, tabHeight_);
}

bool TabBar::onMouseDown(const Core::MouseButtonPressEvent& event) {
    size_t tabIndex = getTabAtPosition(event.x, event.y);
    if (tabIndex < tabs_.size()) {
        // Check if clicked on close button
        if (isCloseButtonAtPosition(event.x, event.y, tabIndex)) {
            removeTab(tabIndex);
            return true;
        }
        
        setSelectedTab(tabIndex);
        dragging_ = true;
        dragStartIndex_ = tabIndex;
        dragOffset_ = event.x - getTabX(tabIndex);
        return true;
    }
    return false;
}

bool TabBar::onMouseMove(const Core::MouseMoveEvent& event) {
    lastMouseX_ = event.x;
    lastMouseY_ = event.y;
    
    // Update close button hover state
    closeHoverIndex_ = -1;
    for (size_t i = 0; i < tabs_.size(); ++i) {
        if (isCloseButtonAtPosition(event.x, event.y, i)) {
            closeHoverIndex_ = static_cast<int>(i);
            break;
        }
    }
    
    if (dragging_) {
        // Would handle tab reordering
    }
    
    markDirty();
    return true;
}

bool TabBar::onMouseUp(const Core::MouseButtonReleaseEvent& event) {
    dragging_ = false;
    return true;
}

void TabBar::layoutTabs() {
    // Calculate tab widths based on available space
    if (tabs_.empty()) return;
    
    float availableWidth = bounds_.width;
    tabWidth_ = std::min(maxTabWidth_, std::max(minTabWidth_, availableWidth / tabs_.size()));
}

float TabBar::getTabX(size_t index) const {
    return bounds_.x + index * tabWidth_;
}

size_t TabBar::getTabAtPosition(float x, float y) const {
    for (size_t i = 0; i < tabs_.size(); ++i) {
        float tabX = getTabX(i);
        if (x >= tabX && x < tabX + tabWidth_ &&
            y >= bounds_.y && y < bounds_.y + tabHeight_) {
            return i;
        }
    }
    return tabs_.size();
}

bool TabBar::isCloseButtonAtPosition(float x, float y, size_t index) const {
    if (index >= tabs_.size() || !tabs_[index].closable) return false;
    
    float closeX = getTabX(index) + tabWidth_ - 24;
    float closeY = bounds_.y;
    
    return x >= closeX && x < closeX + 20 &&
           y >= closeY && y < closeY + tabHeight_;
}

} // namespace NexusForge::UI
