// ui/widgets/status_bar.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include <string>
#include <vector>
#include <functional>

namespace NexusForge::UI {

// Status bar item
struct StatusBarItem {
    std::string id;
    std::string text;
    std::string tooltip;
    std::string icon;
    std::string alignment = "left";  // left, right
    int priority = 0;
    bool visible = true;
    std::function<void()> onClick;
};

// Status bar widget
class StatusBar : public Widget {
public:
    using Ptr = std::shared_ptr<StatusBar>;

    StatusBar();
    ~StatusBar() override;

    // Item management
    void addItem(const StatusBarItem& item);
    void removeItem(const std::string& id);
    void removeItem(int index);
    void clearItems();

    StatusBarItem* getItem(const std::string& id);
    StatusBarItem* getItem(int index);
    int getItemIndex(const std::string& id) const;

    // Item properties
    void setItemText(const std::string& id, const std::string& text);
    void setItemTooltip(const std::string& id, const std::string& tooltip);
    void setItemIcon(const std::string& id, const std::string& icon);
    void setItemVisible(const std::string& id, bool visible);

    // Quick access
    void setText(const std::string& text);
    std::string getText() const;

    // Appearance
    void setHeight(float height) { height_ = height; }
    float getHeight() const { return height_; }

    // Callbacks
    std::function<void(const std::string&)> onItemClicked;

    // Rendering
    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;

    // Events
    bool onMouseDown(const Core::MouseButtonPressEvent& event) override;
    bool onMouseMove(const Core::MouseMoveEvent& event) override;

protected:
    void updateLayout();
    std::vector<StatusBarItem> leftItems_;
    std::vector<StatusBarItem> rightItems_;
    float height_ = 24;

    std::string hoveredItemId_;
};

// Progress indicator for status bar
class StatusProgress : public Widget {
public:
    StatusProgress();

    void setProgress(float progress) { progress_ = progress; markDirty(); }
    float getProgress() const { return progress_; }

    void setMessage(const std::string& message) { message_ = message; markDirty(); }
    const std::string& getMessage() const { return message_; }

    void setIndeterminate(bool indeterminate) { indeterminate_ = indeterminate; }
    bool isIndeterminate() const { return indeterminate_; }

    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;

private:
    float progress_ = 0;
    std::string message_;
    bool indeterminate_ = false;
    float animationTime_ = 0;
};

} // namespace NexusForge::UI
