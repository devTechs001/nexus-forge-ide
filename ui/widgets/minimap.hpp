// ui/widgets/minimap.hpp
#pragma once

#include "../framework/widget_system.hpp"

namespace NexusForge::UI {

// Minimap widget for code editor
class Minimap : public Widget {
public:
    using Ptr = std::shared_ptr<Minimap>;

    Minimap();
    ~Minimap() override;

    // Content
    void setContent(const std::string& content);
    void setLines(const std::vector<std::string>& lines);
    void setViewport(size_t firstVisibleLine, size_t visibleLineCount);
    
    // Scaling
    void setScale(float scale) { scale_ = scale; }
    float getScale() const { return scale_; }
    
    // Events
    std::function<void(size_t)> onLineSelected;
    
    // Widget overrides
    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;
    bool onMouseDown(const Core::MouseButtonPressEvent& event) override;
    bool onMouseMove(const Core::MouseMoveEvent& event) override;
    bool onMouseUp(const Core::MouseButtonReleaseEvent& event) override;

private:
    std::vector<std::string> lines_;
    size_t firstVisibleLine_ = 0;
    size_t visibleLineCount_ = 30;
    size_t totalLines_ = 0;
    
    float scale_ = 0.1f;
    float charWidth_ = 3;
    float lineHeight_ = 3;
    
    bool dragging_ = false;
    
    size_t getLineAtY(float y) const;
    void updateViewport();
};

} // namespace NexusForge::UI
