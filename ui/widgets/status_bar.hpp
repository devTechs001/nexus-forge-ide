// ui/widgets/status_bar.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace NexusForge::UI {

// Status bar item
struct StatusItem {
    std::string id;
    std::string text;
    std::string icon;
    Color color = Color::White();
    Color backgroundColor = Color::Transparent();
    std::string tooltip;
    std::function<void()> onClick;
};

// Status Bar widget
class StatusBar : public Widget {
public:
    using Ptr = std::shared_ptr<StatusBar>;

    StatusBar();
    ~StatusBar() override;

    // Items
    void addItem(const StatusItem& item);
    void removeItem(const std::string& id);
    void updateItem(const std::string& id, const std::string& text);
    void clearItems();
    
    // Status message
    void setStatusMessage(const std::string& message);
    void clearStatusMessage();
    void showProgress(float progress);
    void hideProgress();
    
    // Access
    StatusItem* getItem(const std::string& id);
    
    // Events
    std::function<void(const std::string&)> onItemClicked;
    
    // Widget overrides
    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;
    bool onMouseDown(const Core::MouseButtonPressEvent& event) override;
    bool onMouseMove(const Core::MouseMoveEvent& event) override;

private:
    std::vector<StatusItem> items_;
    std::string statusMessage_;
    float progress_ = -1; // -1 means hidden

    int hoverItemIndex_ = -1;
    float itemHeight_ = 24;
    float lastMouseX_ = 0;
    float lastMouseY_ = 0;

    int getItemAtPosition(float x, float y) const;
};

} // namespace NexusForge::UI
