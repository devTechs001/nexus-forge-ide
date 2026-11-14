// ui/mobile/adaptive_layout.cpp
// Adaptive Layout Implementation

#include "adaptive_layout.hpp"
#include <algorithm>

namespace NexusForge::UI::Mobile {

// AdaptiveLayoutManager implementation
AdaptiveLayoutManager& AdaptiveLayoutManager::getInstance() {
    static AdaptiveLayoutManager instance;
    return instance;
}

void AdaptiveLayoutManager::updateDeviceInfo(const DeviceInfo& info) {
    deviceInfo_ = info;
    
    // Determine form factor
    int screenWidth = info.screenWidth;
    
    if (screenWidth <= Breakpoints::PhoneMax) {
        deviceInfo_.formFactor = FormFactor::Phone;
    } else if (screenWidth <= Breakpoints::TabletMax) {
        deviceInfo_.formFactor = FormFactor::Tablet;
    } else if (screenWidth <= Breakpoints::DesktopMax) {
        deviceInfo_.formFactor = FormFactor::Desktop;
    } else {
        deviceInfo_.formFactor = FormFactor::LargeDesktop;
    }
    
    // Determine orientation
    if (info.screenWidth > info.screenHeight) {
        deviceInfo_.orientation = Orientation::Landscape;
    } else {
        deviceInfo_.orientation = Orientation::Portrait;
    }
    
    // Notify callbacks
    if (deviceInfo_.formFactor != lastFormFactor_ && onFormFactorChanged) {
        onFormFactorChanged(deviceInfo_.formFactor);
        lastFormFactor_ = deviceInfo_.formFactor;
    }
    
    if (deviceInfo_.orientation != lastOrientation_ && onOrientationChanged) {
        onOrientationChanged(deviceInfo_.orientation);
        lastOrientation_ = deviceInfo_.orientation;
    }
}

bool AdaptiveLayoutManager::shouldShowSidebar() const {
    return deviceInfo_.formFactor == FormFactor::Desktop ||
           deviceInfo_.formFactor == FormFactor::LargeDesktop;
}

bool AdaptiveLayoutManager::shouldShowBottomNav() const {
    return deviceInfo_.formFactor == FormFactor::Phone ||
           deviceInfo_.formFactor == FormFactor::Tablet;
}

bool AdaptiveLayoutManager::shouldCollapseMenus() const {
    return deviceInfo_.formFactor == FormFactor::Phone;
}

bool AdaptiveLayoutManager::shouldUseTabs() const {
    return deviceInfo_.formFactor != FormFactor::Phone;
}

bool AdaptiveLayoutManager::shouldUseDrawer() const {
    return deviceInfo_.formFactor == FormFactor::Phone ||
           deviceInfo_.formFactor == FormFactor::Tablet;
}

float AdaptiveLayoutManager::getSidebarWidth() const {
    switch (deviceInfo_.formFactor) {
        case FormFactor::Phone:
            return 0;  // Hidden on phone
        case FormFactor::Tablet:
            return 200;
        case FormFactor::Desktop:
            return 250;
        case FormFactor::LargeDesktop:
        case FormFactor::TV:
            return 300;
        default:
            return 250;
    }
}

float AdaptiveLayoutManager::getToolbarHeight() const {
    switch (deviceInfo_.formFactor) {
        case FormFactor::Phone:
            return 56;
        case FormFactor::Tablet:
            return 48;
        default:
            return 40;
    }
}

float AdaptiveLayoutManager::getBottomNavHeight() const {
    if (shouldShowBottomNav()) {
        return 56;
    }
    return 0;
}

float AdaptiveLayoutManager::getMinTouchTarget() const {
    if (deviceInfo_.hasTouch) {
        return 44;  // Minimum touch target size
    }
    return 32;
}

float AdaptiveLayoutManager::getTouchSlop() const {
    return 8;  // Pixels before drag starts
}

float AdaptiveLayoutManager::getLongPressTimeout() const {
    return 0.5f;  // 500ms
}

// AdaptiveContainer implementation
AdaptiveContainer::AdaptiveContainer() = default;

void AdaptiveContainer::rebuild() {
    if (!builder_) return;
    
    FormFactor newFormFactor = AdaptiveLayoutManager::getInstance().getFormFactor();
    
    if (newFormFactor != currentFormFactor_) {
        currentFormFactor_ = newFormFactor;
        
        // Remove current layout
        if (currentLayout_) {
            removeChild(currentLayout_);
        }
        
        // Get new layout for current form factor
        Widget::Ptr newLayout;
        
        if (builder_) {
            newLayout = builder_(currentFormFactor_);
        } else {
            switch (currentFormFactor_) {
                case FormFactor::Phone:
                    newLayout = phoneLayout_;
                    break;
                case FormFactor::Tablet:
                    newLayout = tabletLayout_;
                    break;
                case FormFactor::Desktop:
                case FormFactor::LargeDesktop:
                case FormFactor::TV:
                    newLayout = desktopLayout_;
                    break;
            }
        }
        
        if (newLayout) {
            currentLayout_ = newLayout;
            addChild(currentLayout_);
        }
        
        markDirty();
    }
}

void AdaptiveContainer::onBoundsChanged(const Rect& oldBounds, const Rect& newBounds) {
    Widget::onBoundsChanged(oldBounds, newBounds);
    rebuild();
}

// BottomNavigationBar implementation
BottomNavigationBar::BottomNavigationBar() = default;

void BottomNavigationBar::setItems(const std::vector<NavItem>& items) {
    items_ = items;
    markDirty();
}

void BottomNavigationBar::setSelectedIndex(int index) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        selectedIndex_ = index;
        markDirty();
        
        if (onItemSelected) {
            onItemSelected(index, items_[index].id);
        }
    }
}

void BottomNavigationBar::render(RenderEngine& renderer) {
    if (!isVisible() || items_.empty()) return;
    
    const Rect& bounds = getBounds();
    
    // Draw background
    Color bgColor = style_.backgroundColor;
    if (bgColor.a == 0) {
        bgColor = Color(0.15f, 0.15f, 0.18f);
    }
    
    renderer.drawRect(bounds, bgColor);
    
    // Draw items
    float itemWidth = bounds.width / items_.size();
    float centerY = bounds.y + bounds.height / 2;
    
    for (size_t i = 0; i < items_.size(); ++i) {
        const NavItem& item = items_[i];
        float x = bounds.x + i * itemWidth;
        bool selected = (static_cast<int>(i) == selectedIndex_);
        
        // Draw background for selected item
        if (selected) {
            renderer.drawRect(Rect(x, bounds.y, itemWidth, bounds.height),
                            Color(0.25f, 0.4f, 0.7f, 0.3f));
        }
        
        // Draw icon
        float iconX = x + itemWidth / 2;
        float iconY = centerY - 12;
        std::string icon = selected && !item.activeIcon.empty() ? item.activeIcon : item.icon;
        Color iconColor = selected ? Color(0.4f, 0.6f, 1.0f) : Color(0.6f, 0.6f, 0.65f);
        renderer.drawText(icon, iconX - 8, iconY, iconColor, "default", 20);
        
        // Draw label
        float labelY = centerY + 16;
        Color labelColor = selected ? Color(0.9f, 0.9f, 0.9f) : Color(0.5f, 0.5f, 0.55f);
        renderer.drawText(item.label, iconX - renderer.measureText(item.label, "default", 10).x / 2,
                         labelY, labelColor, "default", 10);
        
        // Draw badge
        if (item.badgeCount > 0) {
            std::string badgeText = item.badgeCount > 99 ? "99+" : std::to_string(item.badgeCount);
            Vec2 badgeSize = renderer.measureText(badgeText, "default", 10);
            float badgeX = iconX + 12;
            float badgeY = iconY - 4;
            
            renderer.drawRoundedRect(Rect(badgeX, badgeY, badgeSize.x + 8, badgeSize.y + 4),
                                    Color(1.0f, 0.3f, 0.3f), 6);
            renderer.drawText(badgeText, badgeX + 4, badgeY + badgeSize.y, Color(1, 1, 1), "default", 10);
        }
    }
}

Vec2 BottomNavigationBar::measure(float availableWidth, float availableHeight) {
    return Vec2(availableWidth, AdaptiveLayoutManager::getInstance().getBottomNavHeight());
}

bool BottomNavigationBar::onMouseDown(const Core::MouseButtonPressEvent& event) {
    if (items_.empty()) return false;
    
    float itemWidth = getBounds().width / items_.size();
    int index = static_cast<int>((event.x - getBounds().x) / itemWidth);
    
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        setSelectedIndex(index);
        return true;
    }
    
    return false;
}

// FloatingActionButton implementation
FloatingActionButton::FloatingActionButton() = default;

void FloatingActionButton::setExtended(bool extended) {
    extended_ = extended;
    if (extended_) {
        size_ = Size::Extended;
    }
    markDirty();
}

void FloatingActionButton::render(RenderEngine& renderer) {
    if (!isVisible()) return;
    
    const Rect& bounds = getBounds();
    
    float size = (size_ == Size::Mini) ? 40 : 56;
    if (extended_) {
        size = 56;
    }
    
    float centerX = bounds.x + bounds.width / 2;
    float centerY = bounds.y + bounds.height / 2;
    
    // Draw shadow
    for (int i = 6; i > 0; --i) {
        Color shadow(0, 0, 0, 0.15f * i);
        renderer.drawCircle(centerX, centerY + i, size / 2, shadow, true);
    }
    
    // Draw button
    Color bgColor = style_.backgroundColor;
    if (bgColor.a == 0) {
        bgColor = Color(0.25f, 0.4f, 0.7f);
    }
    
    if (extended_) {
        float width = size + renderer.measureText(label_, "default", 14).x + 24;
        renderer.drawRoundedRect(Rect(centerX - width/2, centerY - size/2, width, size), bgColor, size/2);
    } else {
        renderer.drawCircle(centerX, centerY, size / 2, bgColor, true);
    }
    
    // Draw icon
    Color iconColor(1, 1, 1);
    if (!icon_.empty()) {
        renderer.drawText(icon_, centerX - 10, centerY + 10, iconColor, "default", 20);
    }
    
    // Draw label (extended)
    if (extended_ && !label_.empty()) {
        float textX = centerX - size/2 + 24;
        renderer.drawText(label_, textX, centerY + 5, iconColor, "default", 14);
    }
}

Vec2 FloatingActionButton::measure(float availableWidth, float availableHeight) {
    float size = (size_ == Size::Mini) ? 40 : 56;
    
    if (extended_ && !label_.empty()) {
        float width = size + 24 + 14 * label_.length() * 0.6f;
        return Vec2(width, size);
    }
    
    return Vec2(size, size);
}

bool FloatingActionButton::onMouseDown(const Core::MouseButtonPressEvent& event) {
    if (onPressed) {
        onPressed();
    }
    return true;
}

} // namespace NexusForge::UI::Mobile
