// ui/mobile/mobile_toolbar.cpp
// Mobile Toolbar Implementation

#include "mobile_toolbar.hpp"
#include "adaptive_layout.hpp"
#include <algorithm>

namespace NexusForge::UI::Mobile {

MobileToolbar::MobileToolbar() {
    height_ = 56;
    style_.backgroundColor = Color(0.15f, 0.15f, 0.18f);
    style_.foregroundColor = Color(0.9f, 0.9f, 0.9f);
    
    showBackButton_ = false;
    showMenuButton_ = true;
    showSearchButton_ = true;
    collapsed_ = false;
}

MobileToolbar::~MobileToolbar() = default;

void MobileToolbar::setTitle(const std::string& title) {
    title_ = title;
    markDirty();
}

void MobileToolbar::setSubtitle(const std::string& subtitle) {
    subtitle_ = subtitle;
    markDirty();
}

void MobileToolbar::setActions(const std::vector<ToolbarAction>& actions) {
    actions_ = actions;
    markDirty();
}

void MobileToolbar::setBackVisible(bool visible) {
    showBackButton_ = visible;
    markDirty();
}

void MobileToolbar::setMenuVisible(bool visible) {
    showMenuButton_ = visible;
    markDirty();
}

void MobileToolbar::setSearchVisible(bool visible) {
    showSearchButton_ = visible;
    markDirty();
}

void MobileToolbar::setCollapsed(bool collapsed) {
    collapsed_ = collapsed;
    collapseProgress_ = collapsed ? 1.0f : 0.0f;
    markDirty();
}

void MobileToolbar::toggleCollapsed() {
    setCollapsed(!collapsed_);
}

void MobileToolbar::render(RenderEngine& renderer) {
    if (!isVisible()) return;
    
    const Rect& bounds = getBounds();
    
    // Draw background
    renderer.drawRect(bounds, style_.backgroundColor);
    
    // Draw status bar overlay (for immersive mode)
    float statusBarHeight = 24;  // Could be device-specific
    if (bounds.y == 0) {
        renderer.drawRect(Rect(bounds.x, 0, bounds.width, statusBarHeight),
                         Color(0.1f, 0.1f, 0.12f));
    }
    
    float contentY = bounds.y + bounds.height / 2 + style_.fontSize * 0.3f;
    float x = bounds.x + 4;
    
    // Back button
    if (showBackButton_) {
        Rect backRect(x, bounds.y + 8, 40, 40);
        
        if (backButtonHovered_) {
            renderer.drawRect(backRect, Color(0.2f, 0.2f, 0.25f));
        }
        
        renderer.drawText("←", x + 12, contentY, style_.foregroundColor, "default", 20);
        x += 40;
    }
    
    // Menu button (hamburger)
    if (showMenuButton_ && !collapsed_) {
        Rect menuRect(x, bounds.y + 8, 40, 40);
        
        if (menuButtonHovered_) {
            renderer.drawRect(menuRect, Color(0.2f, 0.2f, 0.25f));
        }
        
        // Hamburger icon
        float menuX = x + 12;
        float menuY = bounds.y + 20;
        for (int i = 0; i < 3; ++i) {
            renderer.drawLine(menuX, menuY + i * 6, menuX + 16, menuY + i * 6,
                            style_.foregroundColor, 2);
        }
        x += 40;
    }
    
    // Title
    if (!title_.empty()) {
        float titleX = x + 12;
        float titleY = contentY;
        
        // Truncate title if needed
        float maxWidth = bounds.width - x - 100;
        Vec2 titleSize = renderer.measureText(title_, "default", collapsed_ ? 16 : 18);
        
        std::string displayTitle = title_;
        if (titleSize.x > maxWidth) {
            while (titleSize.x > maxWidth && displayTitle.length() > 3) {
                displayTitle = displayTitle.substr(0, displayTitle.length() - 4) + "...";
                titleSize = renderer.measureText(displayTitle, "default", collapsed_ ? 16 : 18);
            }
        }
        
        renderer.drawText(displayTitle, titleX, titleY, style_.foregroundColor,
                         "default", collapsed_ ? 16 : 18);
        
        if (collapsed_ && !subtitle_.empty()) {
            Color subtitleColor(0.6f, 0.6f, 0.65f);
            renderer.drawText(subtitle_, titleX, titleY + 14, subtitleColor, "default", 12);
        }
    }
    
    // Search button
    float rightX = bounds.x + bounds.width - 4;
    
    if (showSearchButton_) {
        Rect searchRect(rightX - 40, bounds.y + 8, 40, 40);
        
        if (searchButtonHovered_) {
            renderer.drawRect(searchRect, Color(0.2f, 0.2f, 0.25f));
        }
        
        renderer.drawText("🔍", rightX - 28, contentY, style_.foregroundColor, "default", 18);
        rightX -= 40;
    }
    
    // Action buttons
    for (auto it = actions_.rbegin(); it != actions_.rend(); ++it) {
        const ToolbarAction& action = *it;
        
        Rect actionRect(rightX - 40, bounds.y + 8, 40, 40);
        
        if (action.hovered) {
            renderer.drawRect(actionRect, Color(0.2f, 0.2f, 0.25f));
        }
        
        renderer.drawText(action.icon, rightX - 28, contentY, style_.foregroundColor, "default", 18);
        rightX -= 40;
    }
    
    // Bottom border (when collapsed)
    if (collapsed_) {
        renderer.drawLine(bounds.x, bounds.y + bounds.height,
                         bounds.x + bounds.width, bounds.y + bounds.height,
                         Color(0.2f, 0.2f, 0.25f));
    }
}

Vec2 MobileToolbar::measure(float availableWidth, float availableHeight) {
    float h = collapsed_ ? height_ * 0.8f : height_;
    return Vec2(availableWidth, h);
}

bool MobileToolbar::onMouseMove(const Core::MouseMoveEvent& event) {
    const Rect& bounds = getBounds();
    
    // Check back button
    if (showBackButton_) {
        backButtonHovered_ = (event.x >= bounds.x + 4 && event.x < bounds.x + 44 &&
                              event.y >= bounds.y + 8 && event.y < bounds.y + 48);
    }
    
    // Check menu button
    if (showMenuButton_) {
        float menuX = showBackButton_ ? bounds.x + 44 : bounds.x + 4;
        menuButtonHovered_ = (event.x >= menuX && event.x < menuX + 40 &&
                              event.y >= bounds.y + 8 && event.y < bounds.y + 48);
    }
    
    // Check search button
    if (showSearchButton_) {
        searchButtonHovered_ = (event.x >= bounds.x + bounds.width - 44 &&
                                event.x < bounds.x + bounds.width - 4 &&
                                event.y >= bounds.y + 8 && event.y < bounds.y + 48);
    }
    
    // Check action buttons (matching render order: right-to-left after search button)
    float rightX = bounds.x + bounds.width - 4;
    if (showSearchButton_) {
        rightX -= 40;
    }
    for (auto it = actions_.rbegin(); it != actions_.rend(); ++it) {
        it->hovered = (event.x >= rightX - 40 && event.x < rightX &&
                       event.y >= bounds.y + 8 && event.y < bounds.y + 48);
        rightX -= 40;
    }

    markDirty();
    return true;
}

bool MobileToolbar::onMouseLeave(const Core::MouseMoveEvent& event) {
    backButtonHovered_ = false;
    menuButtonHovered_ = false;
    searchButtonHovered_ = false;
    for (auto& action : actions_) {
        action.hovered = false;
    }
    markDirty();
    return true;
}

bool MobileToolbar::onMouseDown(const Core::MouseButtonPressEvent& event) {
    if (event.button != 0) return false;

    const Rect& bounds = getBounds();
    bool inButtonY = (event.y >= bounds.y + 8 && event.y < bounds.y + 48);

    // Handle back button
    if (showBackButton_ && inButtonY &&
        event.x >= bounds.x + 4 && event.x < bounds.x + 44) {
        if (onBack) onBack();
        return true;
    }

    // Handle menu button
    if (showMenuButton_ && !collapsed_ && inButtonY) {
        float menuX = showBackButton_ ? bounds.x + 44 : bounds.x + 4;
        if (event.x >= menuX && event.x < menuX + 40) {
            if (onMenu) onMenu();
            return true;
        }
    }

    // Handle search button
    float rightX = bounds.x + bounds.width - 4;
    if (showSearchButton_) {
        if (inButtonY && event.x >= rightX - 40 && event.x < rightX) {
            if (onSearch) onSearch();
            return true;
        }
        rightX -= 40;
    }

    // Handle action buttons (right-to-left)
    for (auto it = actions_.rbegin(); it != actions_.rend(); ++it) {
        if (inButtonY && event.x >= rightX - 40 && event.x < rightX) {
            if (it->enabled && it->onClick) it->onClick();
            return true;
        }
        rightX -= 40;
    }

    // Handle title click (expand/collapse)
    if (event.y >= bounds.y && event.y < bounds.y + bounds.height) {
        if (onTitleClick) {
            onTitleClick();
        }
        return true;
    }

    return false;
}

void MobileToolbar::setHeight(float height) {
    height_ = height;
    markDirty();
}

float MobileToolbar::getHeight() const {
    return collapsed_ ? height_ * 0.8f : height_;
}

} // namespace NexusForge::UI::Mobile
