// ui/widgets/minimap.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include <string>
#include <vector>

namespace NexusForge::UI {

// Minimap widget
class Minimap : public Widget {
public:
    using Ptr = std::shared_ptr<Minimap>;

    Minimap();
    ~Minimap() override;

    // Content
    void setContent(const std::string& content);
    const std::string& getContent() const { return content_; }

    void setLines(const std::vector<std::string>& lines);
    const std::vector<std::string>& getLines() const { return lines_; }

    // Viewport
    void setViewportLine(size_t line) { viewportLine_ = line; }
    size_t getViewportLine() const { return viewportLine_; }

    void setViewportHeight(size_t lines) { viewportHeight_ = lines; }
    size_t getViewportHeight() const { return viewportHeight_; }

    // Appearance
    void setCharWidth(float width) { charWidth_ = width; }
    float getCharWidth() const { return charWidth_; }

    void setLineHeight(float height) { lineHeight_ = height; }
    float getLineHeight() const { return lineHeight_; }

    void setShowSlider(bool show) { showSlider_ = show; }
    bool showSlider() const { return showSlider_; }

    void setSliderColor(const Color& color) { sliderColor_ = color; }
    Color getSliderColor() const { return sliderColor_; }

    // Rendering
    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;

    // Events
    bool onMouseDown(const Core::MouseButtonPressEvent& event) override;
    bool onMouseMove(const Core::MouseMoveEvent& event) override;
    bool onMouseUp(const Core::MouseButtonReleaseEvent& event) override;

    // Callbacks
    std::function<void(size_t)> onViewportChanged;
    std::function<void()> onSliderDragStart;
    std::function<void()> onSliderDragEnd;

protected:
    void updateSliderBounds();
    size_t lineFromY(float y) const;
    float yFromLine(size_t line) const;
    void renderContent(RenderEngine& renderer);
    void renderSlider(RenderEngine& renderer);
    void renderViewportIndicator(RenderEngine& renderer);

    std::string content_;
    std::vector<std::string> lines_;
    size_t viewportLine_ = 0;
    size_t viewportHeight_ = 20;

    float charWidth_ = 3;
    float lineHeight_ = 6;
    bool showSlider_ = true;
    Color sliderColor_ = Color(0.3f, 0.3f, 0.4f, 0.5f);

    bool dragging_ = false;
    Rect sliderBounds_;
};

} // namespace NexusForge::UI
