// ui/widgets/status_bar.cpp
#include "status_bar.hpp"

namespace NexusForge::UI {

StatusBar::StatusBar() {
    style_.backgroundColor = GradientColor::fromHex(0x11111B);
    style_.foregroundColor = GradientColor::fromHex(0xA6ADC8);
    style_.fontSize = 12;
}

StatusBar::~StatusBar() = default;

void StatusBar::addItem(const StatusItem& item) {
    items_.push_back(item);
}

void StatusBar::removeItem(const std::string& id) {
    items_.erase(
        std::remove_if(items_.begin(), items_.end(),
            [&id](const StatusItem& item) { return item.id == id; }),
        items_.end()
    );
}

void StatusBar::updateItem(const std::string& id, const std::string& text) {
    for (auto& item : items_) {
        if (item.id == id) {
            item.text = text;
            break;
        }
    }
}

void StatusBar::clearItems() {
    items_.clear();
}

void StatusBar::setStatusMessage(const std::string& message) {
    statusMessage_ = message;
}

void StatusBar::clearStatusMessage() {
    statusMessage_.clear();
}

void StatusBar::showProgress(float progress) {
    progress_ = std::clamp(progress, 0.0f, 1.0f);
}

void StatusBar::hideProgress() {
    progress_ = -1;
}

StatusItem* StatusBar::getItem(const std::string& id) {
    for (auto& item : items_) {
        if (item.id == id) {
            return &item;
        }
    }
    return nullptr;
}

void StatusBar::render(RenderEngine& renderer) {
    if (!isVisibleInHierarchy()) return;

    // Draw background
    renderer.drawRect(bounds_, Color(0.07f, 0.07f, 0.11f, 1.0f));
    
    float x = bounds_.x + 8;
    float y = bounds_.y + (bounds_.height - itemHeight_) / 2;
    
    // Draw status message
    if (!statusMessage_.empty()) {
        renderer.drawText(statusMessage_, x, y + style_.fontSize, 
                          Color(0.7f, 0.7f, 0.8f, 1.0f),
                          style_.fontFamily, style_.fontSize);
        x += 200;
    }
    
    // Draw items (right-aligned)
    float rightX = bounds_.x + bounds_.width - 8;
    
    // Draw progress bar
    if (progress_ >= 0) {
        float progressWidth = 100;
        float progressX = rightX - progressWidth - 8;
        
        // Background
        renderer.drawRect(Rect(progressX, y + 4, progressWidth, itemHeight_ - 8),
                          Color(0.2f, 0.2f, 0.3f, 1.0f));
        
        // Fill
        renderer.drawRect(Rect(progressX, y + 4, progressWidth * progress_, itemHeight_ - 8),
                          Color(0.3f, 0.5f, 0.9f, 1.0f));
        
        rightX = progressX - 16;
    }
    
    // Draw status items
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        const StatusItem& item = items_[i];
        
        // Calculate item width based on text
        float itemWidth = item.text.length() * style_.fontSize * 0.5f + 24;
        float itemX = rightX - itemWidth;
        
        // Draw background
        Color bgColor = item.backgroundColor;
        if (i == hoverItemIndex_) {
            bgColor = Color(0.2f, 0.2f, 0.3f, 1.0f);
        }
        
        if (bgColor.a > 0) {
            renderer.drawRect(Rect(itemX, y, itemWidth, itemHeight_), bgColor);
        }
        
        // Draw icon
        if (!item.icon.empty()) {
            renderer.drawText(item.icon, itemX + 6, y + style_.fontSize,
                              item.color, style_.fontFamily, style_.fontSize);
        }
        
        // Draw text
        renderer.drawText(item.text, itemX + 6, y + style_.fontSize,
                          item.color, style_.fontFamily, style_.fontSize);
        
        rightX = itemX - 4;
    }
    
    // Draw separator line at top
    renderer.drawRect(Rect(bounds_.x, bounds_.y, bounds_.width, 1),
                      Color(0.2f, 0.2f, 0.3f, 1.0f));
}

Vec2 StatusBar::measure(float availableWidth, float availableHeight) {
    return Vec2(availableWidth, itemHeight_);
}

bool StatusBar::onMouseDown(const Core::MouseButtonPressEvent& event) {
    int index = getItemAtPosition(event.x, event.y);
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        if (items_[index].onClick) {
            items_[index].onClick();
        }
        if (onItemClicked) {
            onItemClicked(items_[index].id);
        }
        return true;
    }
    return false;
}

bool StatusBar::onMouseMove(const Core::MouseMoveEvent& event) {
    lastMouseX_ = event.x;
    lastMouseY_ = event.y;
    
    hoverItemIndex_ = getItemAtPosition(event.x, event.y);
    markDirty();
    
    return false;
}

int StatusBar::getItemAtPosition(float x, float y) const {
    float rightX = bounds_.x + bounds_.width - 8;
    
    // Skip progress bar
    if (progress_ >= 0) {
        rightX -= 124;
    }
    
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        const StatusItem& item = items_[i];
        float itemWidth = item.text.length() * style_.fontSize * 0.5f + 24;
        float itemX = rightX - itemWidth;
        
        if (x >= itemX && x < rightX &&
            y >= bounds_.y && y < bounds_.y + itemHeight_) {
            return i;
        }
        
        rightX = itemX - 4;
    }
    
    return -1;
}

} // namespace NexusForge::UI
