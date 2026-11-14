// ui/mobile/touch_handler.cpp
// Touch Handler Implementation

#include "touch_handler.hpp"
#include "gesture_recognizer.hpp"
#include <algorithm>

namespace NexusForge::UI::Mobile {

TouchHandler::TouchHandler() = default;

TouchHandler::~TouchHandler() = default;

void TouchHandler::initialize(Widget* rootWidget) {
    rootWidget_ = rootWidget;
    gestureHandler_ = std::make_unique<GestureHandler>();
}

bool TouchHandler::handleTouchEvent(const Core::TouchEvent& event) {
    if (!rootWidget_) return false;
    
    // Update active touches
    updateTouchState(event);
    
    // Pass to gesture recognizer
    if (gestureHandler_) {
        gestureHandler_->handleTouchEvent(event);
    }
    
    // Convert to mouse events for widget system
    switch (event.action) {
        case Core::TouchEvent::Action::Down:
            if (!event.touches.empty()) {
                const auto& touch = event.touches[0];
                Core::MouseButtonPressEvent mouseEvent(0, touch.x, touch.y);
                rootWidget_->handleEvent(mouseEvent);
            }
            break;
            
        case Core::TouchEvent::Action::Up:
            if (!event.touches.empty()) {
                const auto& touch = event.touches[0];
                Core::MouseButtonReleaseEvent mouseEvent(0, touch.x, touch.y);
                rootWidget_->handleEvent(mouseEvent);
            }
            break;
            
        case Core::TouchEvent::Action::Move:
            if (!event.touches.empty()) {
                const auto& touch = event.touches[0];
                Core::MouseMoveEvent mouseEvent(touch.x, touch.y, 0, 0);
                rootWidget_->handleEvent(mouseEvent);
            }
            break;
            
        default:
            break;
    }
    
    return true;
}

void TouchHandler::updateTouchState(const Core::TouchEvent& event) {
    switch (event.action) {
        case Core::TouchEvent::Action::Down:
            for (const auto& touch : event.touches) {
                TouchState state;
                state.id = touch.id;
                state.x = touch.x;
                state.y = touch.y;
                state.startX = touch.x;
                state.startY = touch.y;
                state.previousX = touch.x;
                state.previousY = touch.y;
                state.pressure = touch.pressure;
                state.startTime = std::chrono::steady_clock::now();
                state.lastUpdateTime = state.startTime;
                state.active = true;
                
                activeTouches_[touch.id] = state;
            }
            break;
            
        case Core::TouchEvent::Action::Move:
            for (const auto& touch : event.touches) {
                auto it = activeTouches_.find(touch.id);
                if (it != activeTouches_.end()) {
                    it->second.previousX = it->second.x;
                    it->second.previousY = it->second.y;
                    it->second.x = touch.x;
                    it->second.y = touch.y;
                    it->second.pressure = touch.pressure;
                    it->second.lastUpdateTime = std::chrono::steady_clock::now();
                }
            }
            break;
            
        case Core::TouchEvent::Action::Up:
            for (const auto& touch : event.touches) {
                auto it = activeTouches_.find(touch.id);
                if (it != activeTouches_.end()) {
                    it->second.active = false;
                }
            }
            
            // Clean up inactive touches
            for (auto it = activeTouches_.begin(); it != activeTouches_.end();) {
                if (!it->second.active) {
                    it = activeTouches_.erase(it);
                } else {
                    ++it;
                }
            }
            break;
            
        case Core::TouchEvent::Action::Cancel:
            activeTouches_.clear();
            break;
    }
}

std::vector<TouchState> TouchHandler::getActiveTouches() const {
    std::vector<TouchState> touches;
    for (const auto& [id, state] : activeTouches_) {
        if (state.active) {
            touches.push_back(state);
        }
    }
    return touches;
}

TouchState* TouchHandler::getTouchState(int id) {
    auto it = activeTouches_.find(id);
    if (it != activeTouches_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool TouchHandler::isMultiTouch() const {
    return activeTouches_.size() > 1;
}

int TouchHandler::getTouchCount() const {
    return static_cast<int>(activeTouches_.size());
}

void TouchHandler::setGestureCallback(GestureType type, GestureCallback callback) {
    if (!gestureHandler_) return;
    
    std::shared_ptr<GestureRecognizer> recognizer;
    switch (type) {
        case GestureType::Tap:
            recognizer = std::make_shared<TapGestureRecognizer>();
            break;
        case GestureType::LongPress:
            recognizer = std::make_shared<LongPressGestureRecognizer>();
            break;
        case GestureType::Pan:
            recognizer = std::make_shared<PanGestureRecognizer>();
            break;
        case GestureType::Swipe:
            recognizer = std::make_shared<SwipeGestureRecognizer>();
            break;
        case GestureType::Pinch:
            recognizer = std::make_shared<PinchGestureRecognizer>();
            break;
        case GestureType::Rotate:
            recognizer = std::make_shared<RotationGestureRecognizer>();
            break;
        default:
            return;
    }
    recognizer->setCallback(callback);
    gestureHandler_->addGestureRecognizer(recognizer);
}

void TouchHandler::enableZoom(bool enabled) {
    zoomEnabled_ = enabled;
}

void TouchHandler::enablePan(bool enabled) {
    panEnabled_ = enabled;
}

void TouchHandler::setZoomLimits(float minZoom, float maxZoom) {
    if (minZoom > maxZoom) std::swap(minZoom, maxZoom);
    minZoom_ = minZoom;
    maxZoom_ = maxZoom;
    currentZoom_ = std::clamp(currentZoom_, minZoom_, maxZoom_);
}

float TouchHandler::getCurrentZoom() const {
    return currentZoom_;
}

void TouchHandler::setCurrentZoom(float zoom) {
    currentZoom_ = std::clamp(zoom, minZoom_, maxZoom_);
}

Vec2 TouchHandler::getPanOffset() const {
    return panOffset_;
}

void TouchHandler::setPanOffset(const Vec2& offset) {
    panOffset_ = offset;
}

void TouchHandler::resetTransform() {
    currentZoom_ = 1.0f;
    panOffset_ = Vec2(0, 0);
}

} // namespace NexusForge::UI::Mobile
