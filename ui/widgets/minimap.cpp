// ui/widgets/minimap.cpp
#include "minimap.hpp"

namespace NexusForge::UI {

Minimap::Minimap() {
    style_.backgroundColor = GradientColor::fromHex(0x1A1A2E);
    setFocusPolicy(FocusPolicy::NoFocus);
}

Minimap::~Minimap() = default;

void Minimap::setContent(const std::string& content) {
    lines_.clear();
    size_t start = 0;
    size_t end;
    
    while ((end = content.find('\n', start)) != std::string::npos) {
        lines_.push_back(content.substr(start, end - start));
        start = end + 1;
    }
    lines_.push_back(content.substr(start));
    totalLines_ = lines_.size();
}

void Minimap::setLines(const std::vector<std::string>& lines) {
    lines_ = lines;
    totalLines_ = lines_.size();
}

void Minimap::setViewport(size_t firstVisibleLine, size_t visibleLineCount) {
    firstVisibleLine_ = firstVisibleLine;
    visibleLineCount_ = visibleLineCount;
    markDirty();
}

void Minimap::render(RenderEngine& renderer) {
    if (!isVisibleInHierarchy() || lines_.empty()) return;

    // Draw background
    renderer.drawRect(bounds_, Color(0.1f, 0.1f, 0.18f, 1.0f));
    
    float lineY = bounds_.y + 2;
    float usableHeight = std::max(1.0f, bounds_.height - 4.0f);
    float maxLineHeight = usableHeight / static_cast<float>(totalLines_);
    float actualLineHeight = std::max(0.5f, std::min(lineHeight_, maxLineHeight));
    
    // Draw lines
    size_t startLine = firstVisibleLine_ > 5 ? firstVisibleLine_ - 5 : 0;
    size_t endLine = std::min(totalLines_, firstVisibleLine_ + visibleLineCount_ + 5);
    
    for (size_t i = startLine; i < endLine && i < lines_.size(); ++i) {
        const std::string& line = lines_[i];
        
        // Skip empty lines
        if (line.empty()) {
            lineY += actualLineHeight;
            continue;
        }
        
        // Calculate line color based on content
        Color lineColor;
        if (line.find('{') != std::string::npos || line.find('}') != std::string::npos) {
            lineColor = Color(0.6f, 0.4f, 0.8f, 0.5f);  // Purple for braces
        } else if (line.find("function") != std::string::npos || 
                   line.find("def ") != std::string::npos) {
            lineColor = Color(0.4f, 0.7f, 0.9f, 0.5f);  // Blue for functions
        } else if (line.find("//") != std::string::npos ||
                   line.find("#") != std::string::npos) {
            lineColor = Color(0.4f, 0.5f, 0.4f, 0.3f);  // Green for comments
        } else {
            lineColor = Color(0.5f, 0.5f, 0.6f, 0.4f);  // Default
        }
        
        // Draw line representation
        float lineWidth = std::min(bounds_.width - 4, 
                                   static_cast<float>(line.length()) * charWidth_);
        renderer.drawRect(Rect(bounds_.x + 2, lineY, lineWidth, actualLineHeight - 1), lineColor);
        
        lineY += actualLineHeight;
    }
    
    // Draw viewport indicator
    float viewportHeight = visibleLineCount_ * actualLineHeight;
    float viewportY = bounds_.y + 2 + firstVisibleLine_ * actualLineHeight;
    
    renderer.drawRect(Rect(bounds_.x + 1, viewportY, bounds_.width - 2, viewportHeight),
                      Color(0.3f, 0.4f, 0.6f, 0.2f));
    
    renderer.drawRect(Rect(bounds_.x + 1, viewportY, 2, viewportHeight),
                      Color(0.4f, 0.5f, 0.8f, 0.5f));
}

Vec2 Minimap::measure(float availableWidth, float availableHeight) {
    return Vec2(100, availableHeight);
}

bool Minimap::onMouseDown(const Core::MouseButtonPressEvent& event) {
    size_t line = getLineAtY(event.y);
    if (line < totalLines_) {
        if (onLineSelected) {
            onLineSelected(line);
        }
        dragging_ = true;
    }
    return true;
}

bool Minimap::onMouseMove(const Core::MouseMoveEvent& event) {
    if (dragging_) {
        size_t line = getLineAtY(event.y);
        if (line < totalLines_ && onLineSelected) {
            onLineSelected(line);
        }
    }
    return dragging_;
}

bool Minimap::onMouseUp(const Core::MouseButtonReleaseEvent& event) {
    dragging_ = false;
    return true;
}

size_t Minimap::getLineAtY(float y) const {
    if (totalLines_ == 0) return 0;

    float usableHeight = std::max(1.0f, bounds_.height - 4.0f);
    float maxLineHeight = usableHeight / static_cast<float>(totalLines_);
    float actualLineHeight = std::max(0.5f, std::min(lineHeight_, maxLineHeight));

    float relY = std::max(0.0f, y - bounds_.y - 2.0f);
    size_t line = static_cast<size_t>(relY / actualLineHeight);
    return std::min(line, totalLines_ - 1);
}

void Minimap::updateViewport() {
    // Would sync with editor viewport
}

} // namespace NexusForge::UI
