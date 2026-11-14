// ui/mobile/gesture_recognizer.cpp
// Gesture Recognizer Implementation

#include "gesture_recognizer.hpp"
#include <algorithm>
#include <cmath>

namespace NexusForge::UI::Mobile {

// GestureRecognizer base implementation
GestureRecognizer::GestureRecognizer(GestureType type) : type_(type) {}

void GestureRecognizer::reset() {
    state_ = GestureState::Possible;
}

void GestureRecognizer::setState(GestureState state) {
    if (state_ != state) {
        state_ = state;
        
        GestureData data;
        data.type = type_;
        data.state = state;
        
        if (callback_) {
            callback_(data);
        }
    }
}

void GestureRecognizer::notifyCallback(const GestureData& data) {
    if (callback_) {
        callback_(data);
    }
}

void GestureRecognizer::requireGestureRecognizerToFail(GestureRecognizer* other) {
    if (other) {
        failureRequirements_.push_back(other);
    }
}

bool GestureRecognizer::canBePreventedBy(GestureRecognizer* other) const {
    for (auto* req : failureRequirements_) {
        if (req == other) return true;
    }
    return false;
}

// TapGestureRecognizer implementation
TapGestureRecognizer::TapGestureRecognizer(int requiredTaps, int requiredTouches)
    : GestureRecognizer(GestureType::Tap),
      requiredTaps_(requiredTaps),
      requiredTouches_(requiredTouches) {}

void TapGestureRecognizer::touchBegan(const std::vector<TouchState>& touches) {
    if (!enabled_ || static_cast<int>(touches.size()) != requiredTouches_) {
        return;
    }
    
    initialX_ = touches[0].x;
    initialY_ = touches[0].y;
    currentTapCount_ = 1;
    lastTapTime_ = std::chrono::steady_clock::now();
}

void TapGestureRecognizer::touchMoved(const std::vector<TouchState>& touches) {
    if (state_ != GestureState::Possible) return;
    
    // Check if finger moved too far
    if (touches.empty()) return;
    
    float dx = touches[0].x - initialX_;
    float dy = touches[0].y - initialY_;
    float distance = std::sqrt(dx * dx + dy * dy);
    
    if (distance > maxTapDistance_) {
        state_ = GestureState::Failed;
    }
}

void TapGestureRecognizer::touchEnded(const std::vector<TouchState>& touches) {
    if (!enabled_ || state_ == GestureState::Failed) {
        state_ = GestureState::Failed;
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - lastTapTime_).count();
    
    if (elapsed > maxTimeBetweenTaps_) {
        currentTapCount_ = 1;
    } else {
        currentTapCount_++;
    }
    
    lastTapTime_ = now;
    
    if (currentTapCount_ == requiredTaps_) {
        state_ = GestureState::Ended;
        
        GestureData data;
        data.type = type_;
        data.state = GestureState::Ended;
        data.x = initialX_;
        data.y = initialY_;
        data.tapCount = currentTapCount_;
        data.touchCount = requiredTouches_;
        
        notifyCallback(data);
        
        reset();
    } else {
        state_ = GestureState::Possible;
    }
}

void TapGestureRecognizer::touchCancelled() {
    state_ = GestureState::Cancelled;
    reset();
}

void TapGestureRecognizer::reset() {
    GestureRecognizer::reset();
    currentTapCount_ = 0;
}

// LongPressGestureRecognizer implementation
LongPressGestureRecognizer::LongPressGestureRecognizer()
    : GestureRecognizer(GestureType::LongPress) {}

void LongPressGestureRecognizer::touchBegan(const std::vector<TouchState>& touches) {
    if (!enabled_ || touches.empty()) return;
    
    tracking_ = true;
    recognized_ = false;
    startX_ = touches[0].x;
    startY_ = touches[0].y;
    pressStartTime_ = std::chrono::steady_clock::now();
    state_ = GestureState::Possible;
}

void LongPressGestureRecognizer::touchMoved(const std::vector<TouchState>& touches) {
    if (!tracking_ || touches.empty()) return;
    
    float dx = touches[0].x - startX_;
    float dy = touches[0].y - startY_;
    float distance = std::sqrt(dx * dx + dy * dy);
    
    if (distance > allowableMovement_) {
        tracking_ = false;
        state_ = GestureState::Failed;
    }
}

void LongPressGestureRecognizer::touchEnded(const std::vector<TouchState>& touches) {
    tracking_ = false;
    if (!recognized_) {
        state_ = GestureState::Failed;
    }
    reset();
}

void LongPressGestureRecognizer::touchCancelled() {
    tracking_ = false;
    state_ = GestureState::Cancelled;
    reset();
}

void LongPressGestureRecognizer::update(double deltaTime) {
    if (!tracking_ || recognized_) return;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - pressStartTime_).count();
    
    if (elapsed >= minDuration_) {
        recognized_ = true;
        state_ = GestureState::Began;
        
        GestureData data;
        data.type = type_;
        data.state = GestureState::Began;
        data.x = startX_;
        data.y = startY_;
        
        notifyCallback(data);
    }
}

void LongPressGestureRecognizer::reset() {
    GestureRecognizer::reset();
    tracking_ = false;
    recognized_ = false;
}

// PanGestureRecognizer implementation
PanGestureRecognizer::PanGestureRecognizer(int minTouches, int maxTouches)
    : GestureRecognizer(GestureType::Pan),
      minTouches_(minTouches),
      maxTouches_(maxTouches) {}

void PanGestureRecognizer::touchBegan(const std::vector<TouchState>& touches) {
    if (!enabled_ || static_cast<int>(touches.size()) < minTouches_) return;
    
    startX_ = touches[0].x;
    startY_ = touches[0].y;
    translationX_ = 0;
    translationY_ = 0;
    state_ = GestureState::Possible;
}

void PanGestureRecognizer::touchMoved(const std::vector<TouchState>& touches) {
    if (!enabled_ || touches.empty()) return;
    
    float dx = touches[0].x - startX_;
    float dy = touches[0].y - startY_;
    
    if (state_ == GestureState::Possible) {
        float distance = std::sqrt(dx * dx + dy * dy);
        if (distance >= minDistance_) {
            state_ = GestureState::Began;
        }
    }
    
    if (state_ != GestureState::Possible) {
        translationX_ = dx;
        translationY_ = dy;
        
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime_).count();
        
        if (dt > 0) {
            velocityX_ = (touches[0].x - lastX_) / dt;
            velocityY_ = (touches[0].y - lastY_) / dt;
        }
        
        lastX_ = touches[0].x;
        lastY_ = touches[0].y;
        lastTime_ = now;
        
        GestureData data;
        data.type = type_;
        data.state = state_;
        data.x = touches[0].x;
        data.y = touches[0].y;
        data.translationX = translationX_;
        data.translationY = translationY_;
        data.velocityX = velocityX_;
        data.velocityY = velocityY_;
        
        notifyCallback(data);
    }
}

void PanGestureRecognizer::touchEnded(const std::vector<TouchState>& touches) {
    if (state_ != GestureState::Possible) {
        state_ = GestureState::Ended;
    }
    reset();
}

void PanGestureRecognizer::touchCancelled() {
    state_ = GestureState::Cancelled;
    reset();
}

void PanGestureRecognizer::reset() {
    GestureRecognizer::reset();
    translationX_ = 0;
    translationY_ = 0;
    velocityX_ = 0;
    velocityY_ = 0;
}

// SwipeGestureRecognizer implementation
SwipeGestureRecognizer::SwipeGestureRecognizer(SwipeDirection direction)
    : GestureRecognizer(GestureType::Swipe),
      direction_(direction) {}

void SwipeGestureRecognizer::touchBegan(const std::vector<TouchState>& touches) {
    if (!enabled_ || touches.empty()) return;
    
    startX_ = touches[0].x;
    startY_ = touches[0].y;
    startTime_ = std::chrono::steady_clock::now();
    state_ = GestureState::Possible;
    detectedDirection_ = SwipeDirection::None;
}

void SwipeGestureRecognizer::touchMoved(const std::vector<TouchState>& touches) {
    if (!enabled_ || touches.empty() || state_ != GestureState::Possible) return;
    
    float dx = touches[0].x - startX_;
    float dy = touches[0].y - startY_;
    
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - startTime_).count();
    
    float distance = std::sqrt(dx * dx + dy * dy);
    
    if (distance >= minDistance_) {
        // Determine direction
        float absDx = std::abs(dx);
        float absDy = std::abs(dy);
        
        if (absDx > absDy) {
            detectedDirection_ = (dx > 0) ? SwipeDirection::Right : SwipeDirection::Left;
        } else {
            detectedDirection_ = (dy > 0) ? SwipeDirection::Down : SwipeDirection::Up;
        }
        
        // Check if direction matches
        if (direction_ != SwipeDirection::None && detectedDirection_ != direction_) {
            state_ = GestureState::Failed;
            return;
        }
        
        // Check velocity
        float velocity = distance / elapsed;
        if (velocity >= minVelocity_) {
            state_ = GestureState::Ended;
            
            GestureData data;
            data.type = type_;
            data.state = GestureState::Ended;
            data.x = touches[0].x;
            data.y = touches[0].y;
            data.direction = detectedDirection_;
            data.velocityX = dx / elapsed;
            data.velocityY = dy / elapsed;
            
            notifyCallback(data);
        } else {
            state_ = GestureState::Failed;
        }
    }
}

void SwipeGestureRecognizer::touchEnded(const std::vector<TouchState>& touches) {
    if (state_ == GestureState::Possible) {
        state_ = GestureState::Failed;
    }
    reset();
}

void SwipeGestureRecognizer::touchCancelled() {
    state_ = GestureState::Cancelled;
    reset();
}

// PinchGestureRecognizer implementation
PinchGestureRecognizer::PinchGestureRecognizer()
    : GestureRecognizer(GestureType::Pinch) {}

void PinchGestureRecognizer::touchBegan(const std::vector<TouchState>& touches) {
    if (!enabled_ || touches.size() < 2) return;
    
    float dx = touches[1].x - touches[0].x;
    float dy = touches[1].y - touches[0].y;
    initialDistance_ = std::sqrt(dx * dx + dy * dy);
    scale_ = 1.0f;
    state_ = GestureState::Possible;
}

void PinchGestureRecognizer::touchMoved(const std::vector<TouchState>& touches) {
    if (!enabled_ || touches.size() < 2) return;
    
    float dx = touches[1].x - touches[0].x;
    float dy = touches[1].y - touches[0].y;
    float currentDistance = std::sqrt(dx * dx + dy * dy);
    
    if (state_ == GestureState::Possible) {
        float delta = std::abs(currentDistance - initialDistance_);
        if (delta > 5) {
            state_ = GestureState::Began;
        }
    }
    
    if (state_ != GestureState::Possible) {
        float lastScale = scale_;
        scale_ = currentDistance / initialDistance_;
        
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime_).count();
        
        if (dt > 0) {
            velocity_ = (scale_ - lastScale) / dt;
        }
        
        lastTime_ = now;
        
        GestureData data;
        data.type = type_;
        data.state = state_;
        data.scale = scale_;
        data.velocity = velocity_;
        
        notifyCallback(data);
    }
}

void PinchGestureRecognizer::touchEnded(const std::vector<TouchState>& touches) {
    if (state_ != GestureState::Possible) {
        state_ = GestureState::Ended;
    }
    reset();
}

void PinchGestureRecognizer::touchCancelled() {
    state_ = GestureState::Cancelled;
    reset();
}

void PinchGestureRecognizer::reset() {
    GestureRecognizer::reset();
    scale_ = 1.0f;
    velocity_ = 0;
    initialDistance_ = 0;
}

// RotationGestureRecognizer implementation
RotationGestureRecognizer::RotationGestureRecognizer()
    : GestureRecognizer(GestureType::Rotate) {}

void RotationGestureRecognizer::touchBegan(const std::vector<TouchState>& touches) {
    if (!enabled_ || touches.size() < 2) return;
    
    initialAngle_ = calculateAngle(touches[0], touches[1]);
    rotation_ = 0;
    state_ = GestureState::Possible;
}

void RotationGestureRecognizer::touchMoved(const std::vector<TouchState>& touches) {
    if (!enabled_ || touches.size() < 2) return;
    
    float currentAngle = calculateAngle(touches[0], touches[1]);
    float angleDelta = currentAngle - initialAngle_;
    
    // Normalize to -180 to 180
    while (angleDelta > 180) angleDelta -= 360;
    while (angleDelta < -180) angleDelta += 360;
    
    if (state_ == GestureState::Possible) {
        if (std::abs(angleDelta) > 2) {
            state_ = GestureState::Began;
        }
    }
    
    if (state_ != GestureState::Possible) {
        float lastRotation = rotation_;
        rotation_ = angleDelta;
        
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime_).count();
        
        if (dt > 0) {
            velocity_ = (rotation_ - lastRotation) / dt;
        }
        
        lastTime_ = now;
        
        GestureData data;
        data.type = type_;
        data.state = state_;
        data.rotation = rotation_;
        data.velocity = velocity_;
        
        notifyCallback(data);
    }
}

void RotationGestureRecognizer::touchEnded(const std::vector<TouchState>& touches) {
    if (state_ != GestureState::Possible) {
        state_ = GestureState::Ended;
    }
    reset();
}

void RotationGestureRecognizer::touchCancelled() {
    state_ = GestureState::Cancelled;
    reset();
}

void RotationGestureRecognizer::reset() {
    GestureRecognizer::reset();
    rotation_ = 0;
    velocity_ = 0;
    initialAngle_ = 0;
}

float RotationGestureRecognizer::calculateAngle(const TouchState& t1, const TouchState& t2) {
    float dx = t2.x - t1.x;
    float dy = t2.y - t1.y;
    return std::atan2(dy, dx) * 180.0f / 3.14159f;
}

// GestureHandler implementation
GestureHandler::GestureHandler() = default;

GestureHandler::~GestureHandler() = default;

void GestureHandler::addGestureRecognizer(std::shared_ptr<GestureRecognizer> recognizer) {
    recognizers_.push_back(recognizer);
}

void GestureHandler::removeGestureRecognizer(GestureRecognizer* recognizer) {
    recognizers_.erase(
        std::remove_if(recognizers_.begin(), recognizers_.end(),
            [recognizer](const std::shared_ptr<GestureRecognizer>& r) {
                return r.get() == recognizer;
            }),
        recognizers_.end()
    );
}

void GestureHandler::removeAllGestureRecognizers() {
    recognizers_.clear();
}

void GestureHandler::handleTouchEvent(const Core::TouchEvent& event) {
    std::vector<TouchState> touches;
    for (const auto& touch : event.touches) {
        TouchState state;
        state.id = touch.id;
        state.x = touch.x;
        state.y = touch.y;
        state.pressure = touch.pressure;
        touches.push_back(state);
        
        activeTouches_[touch.id] = state;
    }
    
    for (auto& recognizer : recognizers_) {
        switch (event.action) {
            case Core::TouchEvent::Action::Down:
                recognizer->touchBegan(touches);
                break;
            case Core::TouchEvent::Action::Move:
                recognizer->touchMoved(touches);
                break;
            case Core::TouchEvent::Action::Up:
                recognizer->touchEnded(touches);
                break;
            case Core::TouchEvent::Action::Cancel:
                recognizer->touchCancelled();
                break;
        }
    }
}

void GestureHandler::update(double deltaTime) {
    for (auto& recognizer : recognizers_) {
        if (auto* longPress = dynamic_cast<LongPressGestureRecognizer*>(recognizer.get())) {
            longPress->update(deltaTime);
        }
    }
}

void GestureHandler::onTap(GestureCallback callback, int taps) {
    auto recognizer = std::make_shared<TapGestureRecognizer>(taps, 1);
    recognizer->setCallback(callback);
    addGestureRecognizer(recognizer);
}

void GestureHandler::onDoubleTap(GestureCallback callback) {
    onTap(callback, 2);
}

void GestureHandler::onLongPress(GestureCallback callback) {
    auto recognizer = std::make_shared<LongPressGestureRecognizer>();
    recognizer->setCallback(callback);
    addGestureRecognizer(recognizer);
}

void GestureHandler::onPan(GestureCallback callback) {
    auto recognizer = std::make_shared<PanGestureRecognizer>();
    recognizer->setCallback(callback);
    addGestureRecognizer(recognizer);
}

void GestureHandler::onSwipe(SwipeDirection direction, GestureCallback callback) {
    auto recognizer = std::make_shared<SwipeGestureRecognizer>(direction);
    recognizer->setCallback(callback);
    addGestureRecognizer(recognizer);
}

void GestureHandler::onPinch(GestureCallback callback) {
    auto recognizer = std::make_shared<PinchGestureRecognizer>();
    recognizer->setCallback(callback);
    addGestureRecognizer(recognizer);
}

void GestureHandler::onRotate(GestureCallback callback) {
    auto recognizer = std::make_shared<RotationGestureRecognizer>();
    recognizer->setCallback(callback);
    addGestureRecognizer(recognizer);
}

} // namespace NexusForge::UI::Mobile
