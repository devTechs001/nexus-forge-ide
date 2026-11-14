// ui/mobile/touch_handler.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include "gesture_recognizer.hpp"
#include "../../core/engine/event_system.hpp"

#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <chrono>

namespace NexusForge::UI::Mobile {

// Gesture type enum (forward reference)
enum class GestureType;

// Touch handler class
class TouchHandler {
public:
    TouchHandler();
    ~TouchHandler();

    void initialize(Widget* rootWidget);

    // Touch event handling
    bool handleTouchEvent(const Core::TouchEvent& event);

    // Touch state queries
    std::vector<TouchState> getActiveTouches() const;
    TouchState* getTouchState(int id);
    bool isMultiTouch() const;
    int getTouchCount() const;

    // Gesture callbacks
    using GestureCallback = std::function<void(const GestureData&)>;
    void setGestureCallback(GestureType type, GestureCallback callback);

    // Zoom and pan support
    void enableZoom(bool enabled);
    void enablePan(bool enabled);
    void setZoomLimits(float minZoom, float maxZoom);
    float getCurrentZoom() const;
    void setCurrentZoom(float zoom);
    Vec2 getPanOffset() const;
    void setPanOffset(const Vec2& offset);
    void resetTransform();

private:
    Widget* rootWidget_ = nullptr;
    std::unique_ptr<GestureHandler> gestureHandler_;

    std::unordered_map<int, TouchState> activeTouches_;

    bool zoomEnabled_ = true;
    bool panEnabled_ = true;
    float minZoom_ = 0.5f;
    float maxZoom_ = 3.0f;
    float currentZoom_ = 1.0f;
    Vec2 panOffset_;

    void updateTouchState(const Core::TouchEvent& event);
};

} // namespace NexusForge::UI::Mobile
