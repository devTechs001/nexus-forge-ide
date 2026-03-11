// ui/mobile/gesture_recognizer.hpp
#pragma once

#include "../../core/engine/event_system.hpp"
#include <vector>
#include <chrono>
#include <functional>
#include <unordered_map>

namespace NexusForge::UI::Mobile {

// Touch state
struct TouchState {
    int id;
    float x, y;
    float startX, startY;
    float previousX, previousY;
    float pressure;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point lastUpdateTime;
    bool active;
};

// Gesture types
enum class GestureType {
    Tap,
    DoubleTap,
    LongPress,
    Pan,
    Swipe,
    Pinch,
    Rotate,
    TwoFingerTap,
    ThreeFingerTap,
    EdgeSwipe
};

// Gesture state
enum class GestureState {
    Possible,
    Began,
    Changed,
    Ended,
    Cancelled,
    Failed
};

// Swipe direction
enum class SwipeDirection {
    None,
    Left,
    Right,
    Up,
    Down
};

// Gesture event data
struct GestureData {
    GestureType type;
    GestureState state;

    // Position
    float x, y;
    float startX, startY;

    // Movement
    float translationX, translationY;
    float velocityX, velocityY;

    // Pinch/Rotate
    float scale;
    float rotation;
    float velocity;  // Scale or rotation velocity

    // Tap
    int tapCount;

    // Swipe
    SwipeDirection direction;

    // Touch info
    int touchCount;
    std::vector<TouchState> touches;
};

// Gesture callback
using GestureCallback = std::function<void(const GestureData&)>;

// Individual gesture recognizer
class GestureRecognizer {
public:
    GestureRecognizer(GestureType type);
    virtual ~GestureRecognizer() = default;

    GestureType getType() const { return type_; }
    GestureState getState() const { return state_; }
    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }

    virtual void touchBegan(const std::vector<TouchState>& touches) = 0;
    virtual void touchMoved(const std::vector<TouchState>& touches) = 0;
    virtual void touchEnded(const std::vector<TouchState>& touches) = 0;
    virtual void touchCancelled() = 0;
    virtual void reset();

    void setCallback(GestureCallback callback) { callback_ = callback; }

    // Failure requirements
    void requireGestureRecognizerToFail(GestureRecognizer* other);
    bool canBePreventedBy(GestureRecognizer* other) const;

protected:
    void setState(GestureState state);
    void notifyCallback(const GestureData& data);

    GestureType type_;
    GestureState state_ = GestureState::Possible;
    bool enabled_ = true;
    GestureCallback callback_;
    std::vector<GestureRecognizer*> failureRequirements_;
};

// Tap recognizer
class TapGestureRecognizer : public GestureRecognizer {
public:
    TapGestureRecognizer(int requiredTaps = 1, int requiredTouches = 1);

    void setNumberOfTapsRequired(int taps) { requiredTaps_ = taps; }
    void setNumberOfTouchesRequired(int touches) { requiredTouches_ = touches; }
    void setMaxTapDistance(float distance) { maxTapDistance_ = distance; }
    void setMaxTapDuration(float duration) { maxTapDuration_ = duration; }

    void touchBegan(const std::vector<TouchState>& touches) override;
    void touchMoved(const std::vector<TouchState>& touches) override;
    void touchEnded(const std::vector<TouchState>& touches) override;
    void touchCancelled() override;
    void reset() override;

private:
    int requiredTaps_;
    int requiredTouches_;
    float maxTapDistance_ = 20.0f;
    float maxTapDuration_ = 0.3f;  // seconds
    float maxTimeBetweenTaps_ = 0.3f;

    int currentTapCount_ = 0;
    std::chrono::steady_clock::time_point lastTapTime_;
    float initialX_, initialY_;
};

// Long press recognizer
class LongPressGestureRecognizer : public GestureRecognizer {
public:
    LongPressGestureRecognizer();

    void setMinimumPressDuration(float duration) { minDuration_ = duration; }
    void setAllowableMovement(float movement) { allowableMovement_ = movement; }

    void touchBegan(const std::vector<TouchState>& touches) override;
    void touchMoved(const std::vector<TouchState>& touches) override;
    void touchEnded(const std::vector<TouchState>& touches) override;
    void touchCancelled() override;
    void reset() override;

    void update(double deltaTime);

private:
    float minDuration_ = 0.5f;  // seconds
    float allowableMovement_ = 10.0f;

    bool tracking_ = false;
    float startX_, startY_;
    std::chrono::steady_clock::time_point pressStartTime_;
    bool recognized_ = false;
};

// Pan gesture recognizer
class PanGestureRecognizer : public GestureRecognizer {
public:
    PanGestureRecognizer(int minTouches = 1, int maxTouches = 1);

    void setMinimumNumberOfTouches(int touches) { minTouches_ = touches; }
    void setMaximumNumberOfTouches(int touches) { maxTouches_ = touches; }
    void setMinimumDistance(float distance) { minDistance_ = distance; }

    void touchBegan(const std::vector<TouchState>& touches) override;
    void touchMoved(const std::vector<TouchState>& touches) override;
    void touchEnded(const std::vector<TouchState>& touches) override;
    void touchCancelled() override;
    void reset() override;

    float getTranslationX() const { return translationX_; }
    float getTranslationY() const { return translationY_; }
    float getVelocityX() const { return velocityX_; }
    float getVelocityY() const { return velocityY_; }

private:
    int minTouches_, maxTouches_;
    float minDistance_ = 10.0f;

    float startX_, startY_;
    float translationX_ = 0, translationY_ = 0;
    float velocityX_ = 0, velocityY_ = 0;
    float lastX_, lastY_;
    std::chrono::steady_clock::time_point lastTime_;
};

// Swipe gesture recognizer
class SwipeGestureRecognizer : public GestureRecognizer {
public:
    SwipeGestureRecognizer(SwipeDirection direction = SwipeDirection::Right);

    void setDirection(SwipeDirection direction) { direction_ = direction; }
    void setMinimumVelocity(float velocity) { minVelocity_ = velocity; }
    void setMinimumDistance(float distance) { minDistance_ = distance; }

    void touchBegan(const std::vector<TouchState>& touches) override;
    void touchMoved(const std::vector<TouchState>& touches) override;
    void touchEnded(const std::vector<TouchState>& touches) override;
    void touchCancelled() override;

    SwipeDirection getDetectedDirection() const { return detectedDirection_; }

private:
    SwipeDirection direction_;
    SwipeDirection detectedDirection_ = SwipeDirection::None;
    float minVelocity_ = 300.0f;  // pixels per second
    float minDistance_ = 50.0f;

    float startX_, startY_;
    std::chrono::steady_clock::time_point startTime_;
};

// Pinch gesture recognizer
class PinchGestureRecognizer : public GestureRecognizer {
public:
    PinchGestureRecognizer();

    void touchBegan(const std::vector<TouchState>& touches) override;
    void touchMoved(const std::vector<TouchState>& touches) override;
    void touchEnded(const std::vector<TouchState>& touches) override;
    void touchCancelled() override;
    void reset() override;

    float getScale() const { return scale_; }
    float getVelocity() const { return velocity_; }

private:
    float scale_ = 1.0f;
    float initialDistance_ = 0;
    float velocity_ = 0;
    float lastScale_ = 1.0f;
    std::chrono::steady_clock::time_point lastTime_;
};

// Rotation gesture recognizer
class RotationGestureRecognizer : public GestureRecognizer {
public:
    RotationGestureRecognizer();

    void touchBegan(const std::vector<TouchState>& touches) override;
    void touchMoved(const std::vector<TouchState>& touches) override;
    void touchEnded(const std::vector<TouchState>& touches) override;
    void touchCancelled() override;
    void reset() override;

    float getRotation() const { return rotation_; }
    float getVelocity() const { return velocity_; }

private:
    float rotation_ = 0;
    float initialAngle_ = 0;
    float velocity_ = 0;
    float lastRotation_ = 0;
    std::chrono::steady_clock::time_point lastTime_;

    float calculateAngle(const TouchState& t1, const TouchState& t2);
};

// Main gesture handler
class GestureHandler {
public:
    GestureHandler();
    ~GestureHandler();

    void addGestureRecognizer(std::shared_ptr<GestureRecognizer> recognizer);
    void removeGestureRecognizer(GestureRecognizer* recognizer);
    void removeAllGestureRecognizers();

    void handleTouchEvent(const Core::TouchEvent& event);
    void update(double deltaTime);

    // Quick setup methods
    void onTap(GestureCallback callback, int taps = 1);
    void onDoubleTap(GestureCallback callback);
    void onLongPress(GestureCallback callback);
    void onPan(GestureCallback callback);
    void onSwipe(SwipeDirection direction, GestureCallback callback);
    void onPinch(GestureCallback callback);
    void onRotate(GestureCallback callback);

private:
    std::vector<std::shared_ptr<GestureRecognizer>> recognizers_;
    std::unordered_map<int, TouchState> activeTouches_;

    void updateTouchState(const Core::TouchEvent& event);
    std::vector<TouchState> getActiveTouches() const;
};

} // namespace NexusForge::UI::Mobile
