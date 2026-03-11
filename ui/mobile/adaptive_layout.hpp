// ui/mobile/adaptive_layout.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include <functional>
#include <optional>

namespace NexusForge::UI::Mobile {

// Device form factors
enum class FormFactor {
    Phone,
    Tablet,
    Desktop,
    LargeDesktop,
    TV
};

// Screen orientation
enum class Orientation {
    Portrait,
    Landscape
};

// Breakpoints (in logical pixels)
struct Breakpoints {
    static constexpr int PhoneMax = 600;
    static constexpr int TabletMax = 1024;
    static constexpr int DesktopMax = 1440;
    static constexpr int LargeDesktopMax = 1920;
};

// Device info
struct DeviceInfo {
    int screenWidth;
    int screenHeight;
    float devicePixelRatio;
    FormFactor formFactor;
    Orientation orientation;
    bool hasTouch;
    bool hasMouse;
    bool hasKeyboard;
    bool hasPen;
    float safeAreaTop;
    float safeAreaBottom;
    float safeAreaLeft;
    float safeAreaRight;
    std::string platform;  // ios, android, windows, macos, linux, web
};

// Responsive value
template<typename T>
class Responsive {
public:
    Responsive(T defaultValue) : defaultValue_(defaultValue) {}

    Responsive& phone(T value) { phoneValue_ = value; return *this; }
    Responsive& tablet(T value) { tabletValue_ = value; return *this; }
    Responsive& desktop(T value) { desktopValue_ = value; return *this; }
    Responsive& largeDesktop(T value) { largeDesktopValue_ = value; return *this; }

    T get(FormFactor formFactor) const {
        switch (formFactor) {
            case FormFactor::Phone:
                return phoneValue_.value_or(defaultValue_);
            case FormFactor::Tablet:
                return tabletValue_.value_or(phoneValue_.value_or(defaultValue_));
            case FormFactor::Desktop:
                return desktopValue_.value_or(tabletValue_.value_or(defaultValue_));
            case FormFactor::LargeDesktop:
            case FormFactor::TV:
                return largeDesktopValue_.value_or(desktopValue_.value_or(defaultValue_));
        }
        return defaultValue_;
    }

private:
    T defaultValue_;
    std::optional<T> phoneValue_;
    std::optional<T> tabletValue_;
    std::optional<T> desktopValue_;
    std::optional<T> largeDesktopValue_;
};

// Layout manager
class AdaptiveLayoutManager {
public:
    static AdaptiveLayoutManager& getInstance();

    // Device info
    void updateDeviceInfo(const DeviceInfo& info);
    const DeviceInfo& getDeviceInfo() const { return deviceInfo_; }
    FormFactor getFormFactor() const { return deviceInfo_.formFactor; }
    Orientation getOrientation() const { return deviceInfo_.orientation; }

    // Responsive helpers
    float getResponsiveValue(const Responsive<float>& value) const {
        return value.get(deviceInfo_.formFactor);
    }

    // Layout configuration
    bool shouldShowSidebar() const;
    bool shouldShowBottomNav() const;
    bool shouldCollapseMenus() const;
    bool shouldUseTabs() const;
    bool shouldUseDrawer() const;

    float getSidebarWidth() const;
    float getToolbarHeight() const;
    float getBottomNavHeight() const;

    // Touch optimizations
    float getMinTouchTarget() const;
    float getTouchSlop() const;
    float getLongPressTimeout() const;

    // Callbacks
    std::function<void(FormFactor)> onFormFactorChanged;
    std::function<void(Orientation)> onOrientationChanged;

private:
    AdaptiveLayoutManager() = default;
    DeviceInfo deviceInfo_;
    FormFactor lastFormFactor_ = FormFactor::Desktop;
    Orientation lastOrientation_ = Orientation::Landscape;
};

// Adaptive container that responds to form factor
class AdaptiveContainer : public Container {
public:
    using BuilderFunc = std::function<Widget::Ptr(FormFactor)>;

    AdaptiveContainer();

    void setBuilder(BuilderFunc builder) { builder_ = builder; rebuild(); }
    void setPhoneLayout(Widget::Ptr layout) { phoneLayout_ = layout; }
    void setTabletLayout(Widget::Ptr layout) { tabletLayout_ = layout; }
    void setDesktopLayout(Widget::Ptr layout) { desktopLayout_ = layout; }

    void rebuild();

protected:
    void onBoundsChanged(const Rect& oldBounds, const Rect& newBounds) override;

private:
    BuilderFunc builder_;
    Widget::Ptr phoneLayout_;
    Widget::Ptr tabletLayout_;
    Widget::Ptr desktopLayout_;
    Widget::Ptr currentLayout_;
    FormFactor currentFormFactor_ = FormFactor::Desktop;
};

// Mobile-optimized toolbar
class MobileToolbar : public Container {
public:
    MobileToolbar();

    void setTitle(const std::string& title) { title_ = title; markDirty(); }
    void setSubtitle(const std::string& subtitle) { subtitle_ = subtitle; markDirty(); }
    void setLeadingIcon(const std::string& icon, std::function<void()> onClick);
    void setTrailingActions(const std::vector<std::pair<std::string, std::function<void()>>>& actions);

    void setElevation(float elevation) { elevation_ = elevation; }
    void setCollapsible(bool collapsible) { collapsible_ = collapsible; }
    void setCollapsed(bool collapsed);

    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;

private:
    std::string title_;
    std::string subtitle_;
    std::string leadingIcon_;
    std::function<void()> leadingAction_;
    std::vector<std::pair<std::string, std::function<void()>>> trailingActions_;
    float elevation_ = 4.0f;
    bool collapsible_ = false;
    bool collapsed_ = false;
    float collapseProgress_ = 0.0f;
};

// Bottom navigation bar
class BottomNavigationBar : public Container {
public:
    struct NavItem {
        std::string id;
        std::string icon;
        std::string activeIcon;
        std::string label;
        int badgeCount = 0;
    };

    BottomNavigationBar();

    void setItems(const std::vector<NavItem>& items);
    void setSelectedIndex(int index);
    int getSelectedIndex() const { return selectedIndex_; }

    std::function<void(int index, const std::string& id)> onItemSelected;

    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;
    bool onMouseDown(const Core::MouseButtonPressEvent& event) override;
    bool onTouchStart(const Core::TouchEvent& event) override;

private:
    std::vector<NavItem> items_;
    int selectedIndex_ = 0;
    float itemWidth_ = 0;
};

// Floating Action Button
class FloatingActionButton : public Widget {
public:
    enum class Size { Mini, Regular, Extended };

    FloatingActionButton();

    void setIcon(const std::string& icon) { icon_ = icon; markDirty(); }
    void setLabel(const std::string& label) { label_ = label; size_ = Size::Extended; markDirty(); }
    void setSize(Size size) { size_ = size; markDirty(); }
    void setExtended(bool extended);

    std::function<void()> onPressed;

    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;
    bool onMouseDown(const Core::MouseButtonPressEvent& event) override;
    bool onTouchStart(const Core::TouchEvent& event) override;

private:
    std::string icon_;
    std::string label_;
    Size size_ = Size::Regular;
    bool extended_ = false;
    float extendProgress_ = 0.0f;
};

// Swipeable drawer
class SwipeableDrawer : public Container {
public:
    enum class Position { Left, Right, Bottom };

    SwipeableDrawer(Position position = Position::Left);

    void setContent(Widget::Ptr content) { content_ = content; addChild(content); }
    void setWidth(float width) { drawerWidth_ = width; }

    void open();
    void close();
    void toggle();
    bool isOpen() const { return open_; }

    void setSwipeEnabled(bool enabled) { swipeEnabled_ = enabled; }
    void setScrimColor(const Color& color) { scrimColor_ = color; }

    std::function<void(bool open)> onStateChanged;

    void render(RenderEngine& renderer) override;
    void update(double deltaTime) override;
    bool onTouchStart(const Core::TouchEvent& event) override;
    bool onTouchMove(const Core::TouchEvent& event) override;
    bool onTouchEnd(const Core::TouchEvent& event) override;
    bool onGesture(const Core::GestureEvent& event) override;

private:
    Position position_;
    Widget::Ptr content_;
    float drawerWidth_ = 280;
    bool open_ = false;
    float openProgress_ = 0.0f;
    float targetProgress_ = 0.0f;
    bool swipeEnabled_ = true;
    Color scrimColor_ = Color(0, 0, 0, 0.5f);

    // Gesture tracking
    bool tracking_ = false;
    float startX_ = 0;
    float currentX_ = 0;
    float velocity_ = 0;
};

// Pull to refresh
class PullToRefresh : public Container {
public:
    PullToRefresh();

    void setContent(Widget::Ptr content) { content_ = content; addChild(content); }
    void setRefreshing(bool refreshing);
    bool isRefreshing() const { return refreshing_; }

    std::function<void()> onRefresh;

    void render(RenderEngine& renderer) override;
    bool onTouchStart(const Core::TouchEvent& event) override;
    bool onTouchMove(const Core::TouchEvent& event) override;
    bool onTouchEnd(const Core::TouchEvent& event) override;

private:
    Widget::Ptr content_;
    bool refreshing_ = false;
    float pullDistance_ = 0;
    float refreshThreshold_ = 80;
    bool canRefresh_ = false;
    float startY_ = 0;
};

// Mobile keyboard overlay handler
class VirtualKeyboardHandler {
public:
    static VirtualKeyboardHandler& getInstance();

    void setKeyboardVisible(bool visible, float height);
    bool isKeyboardVisible() const { return keyboardVisible_; }
    float getKeyboardHeight() const { return keyboardHeight_; }

    void showKeyboard();
    void hideKeyboard();
    void setInputType(const std::string& type);  // text, number, email, etc.

    std::function<void(bool visible, float height)> onKeyboardChanged;

private:
    VirtualKeyboardHandler() = default;
    bool keyboardVisible_ = false;
    float keyboardHeight_ = 0;
};

} // namespace NexusForge::UI::Mobile
