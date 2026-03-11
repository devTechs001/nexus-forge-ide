// ui/framework/animation_engine.hpp
#pragma once

#include "widget_system.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>

namespace NexusForge::UI {

// Easing functions
enum class EasingType {
    Linear,
    EaseInQuad,
    EaseOutQuad,
    EaseInOutQuad,
    EaseInCubic,
    EaseOutCubic,
    EaseInOutCubic,
    EaseInQuart,
    EaseOutQuart,
    EaseInOutQuart,
    EaseInQuint,
    EaseOutQuint,
    EaseInOutQuint,
    EaseInSine,
    EaseOutSine,
    EaseInOutSine,
    EaseInExpo,
    EaseOutExpo,
    EaseInOutExpo,
    EaseInCirc,
    EaseOutCirc,
    EaseInOutCirc,
    EaseInBack,
    EaseOutBack,
    EaseInOutBack,
    EaseInElastic,
    EaseOutElastic,
    EaseInOutElastic,
    EaseInBounce,
    EaseOutBounce,
    EaseInOutBounce
};

// Easing function
using EasingFunction = std::function<float(float)>;

// Get easing function by type
EasingFunction getEasingFunction(EasingType type);

// Animation types
enum class AnimationType {
    Opacity,
    Translation,
    Scale,
    Rotation,
    Width,
    Height,
    Size,
    Bounds,
    Color,
    Custom
};

// Animation target
struct AnimationTarget {
    AnimationType type;
    Widget* widget;
    std::string property;
    
    // Start and end values
    float startX, startY;
    float endX, endY;
    float startWidth, startHeight;
    float endWidth, endHeight;
    float startOpacity, endOpacity;
    float startAngle, endAngle;
    Color startColor, endColor;
};

// Animation class
class Animation {
public:
    using Ptr = std::shared_ptr<Animation>;
    
    Animation();
    ~Animation();
    
    // Configuration
    void setDuration(float seconds) { duration_ = seconds; }
    void setDelay(float seconds) { delay_ = seconds; }
    void setEasing(EasingType type) { easingType_ = type; }
    void setEasingFunction(EasingFunction func) { easingFunction_ = func; }
    void setLoop(bool loop) { loop_ = loop; }
    void setPingPong(bool pingPong) { pingPong_ = pingPong; }
    void setAutoReverse(bool reverse) { autoReverse_ = reverse; }
    
    // Target
    void setTarget(Widget* widget);
    void setProperty(const std::string& property);
    void setFrom(float value);
    void setTo(float value);
    void setFromTo(float from, float to);
    
    // For multi-property animations
    void setFromBounds(const Rect& bounds);
    void setToBounds(const Rect& bounds);
    void setFromColor(const Color& color);
    void setToColor(const Color& color);
    
    // State
    bool isRunning() const { return running_; }
    bool isPaused() const { return paused_; }
    bool isComplete() const { return complete_; }
    float getProgress() const { return progress_; }
    float getElapsed() const { return elapsed_; }
    
    // Control
    void start();
    void stop();
    void pause();
    void resume();
    void reset();
    void reverse();
    
    // Update
    void update(float deltaTime);
    
    // Callbacks
    std::function<void()> onStart;
    std::function<void()> onUpdate;
    std::function<void()> onComplete;
    std::function<void()> onReverse;
    
private:
    AnimationTarget target_;
    float duration_ = 0.5f;
    float delay_ = 0;
    float elapsed_ = 0;
    float progress_ = 0;
    
    EasingType easingType_ = EasingType::EaseOutQuad;
    EasingFunction easingFunction_;
    
    bool running_ = false;
    bool paused_ = false;
    bool complete_ = false;
    bool loop_ = false;
    bool pingPong_ = false;
    bool autoReverse_ = false;
    bool reversing_ = false;
    
    std::chrono::steady_clock::time_point startTime_;
    
    void applyValue(float value);
    float ease(float t);
};

// Animation builder
class AnimationBuilder {
public:
    AnimationBuilder();
    
    AnimationBuilder& with(Widget* widget);
    AnimationBuilder& property(const std::string& name);
    AnimationBuilder& from(float value);
    AnimationBuilder& to(float value);
    AnimationBuilder& duration(float seconds);
    AnimationBuilder& delay(float seconds);
    AnimationBuilder& easing(EasingType type);
    AnimationBuilder& loop(bool loop = true);
    AnimationBuilder& pingPong(bool pingPong = true);
    
    Animation::Ptr build();
    
private:
    std::shared_ptr<Animation> animation_;
};

// Animation engine
class AnimationEngine {
public:
    static AnimationEngine& getInstance();
    
    // Animation management
    void play(Animation::Ptr animation);
    void stop(Animation::Ptr animation);
    void stopAll();
    void stop(Widget* widget);
    void stop(Widget* widget, const std::string& property);
    
    // Pause/Resume
    void pause(Animation::Ptr animation);
    void resume(Animation::Ptr animation);
    void pauseAll();
    void resumeAll();
    
    // Update
    void update(float deltaTime);
    
    // Statistics
    size_t getActiveAnimationCount() const { return activeAnimations_.size(); }
    size_t getTotalAnimationCount() const { return allAnimations_.size(); }
    
    // Convenience methods
    Animation::Ptr fade(Widget* widget, float from, float to, float duration);
    Animation::Ptr move(Widget* widget, float fromX, float fromY, float toX, float toY, float duration);
    Animation::Ptr scale(Widget* widget, float fromScale, float toScale, float duration);
    Animation::Ptr rotate(Widget* widget, float fromAngle, float toAngle, float duration);
    Animation::Ptr resize(Widget* widget, float fromWidth, float fromHeight,
                          float toWidth, float toHeight, float duration);
    
private:
    AnimationEngine() = default;
    
    std::vector<Animation::Ptr> activeAnimations_;
    std::unordered_map<Widget*, std::vector<Animation::Ptr>> widgetAnimations_;
    std::vector<Animation::Ptr> allAnimations_;
    
    void removeCompletedAnimations();
};

// Pre-built animations
namespace Animations {
    Animation::Ptr fadeIn(Widget* widget, float duration = 0.3f);
    Animation::Ptr fadeOut(Widget* widget, float duration = 0.3f);
    Animation::Ptr fadeInOut(Widget* widget, float duration = 0.6f);
    
    Animation::Ptr slideInFromLeft(Widget* widget, float distance = 100, float duration = 0.3f);
    Animation::Ptr slideInFromRight(Widget* widget, float distance = 100, float duration = 0.3f);
    Animation::Ptr slideInFromTop(Widget* widget, float distance = 100, float duration = 0.3f);
    Animation::Ptr slideInFromBottom(Widget* widget, float distance = 100, float duration = 0.3f);
    
    Animation::Ptr scaleIn(Widget* widget, float fromScale = 0.5f, float duration = 0.3f);
    Animation::Ptr scaleOut(Widget* widget, float toScale = 0.5f, float duration = 0.3f);
    
    Animation::Ptr bounce(Widget* widget, float amplitude = 10, float duration = 0.5f);
    Animation::Ptr shake(Widget* widget, float amplitude = 5, float duration = 0.3f);
    
    Animation::Ptr pulse(Widget* widget, float scale = 1.1f, float duration = 0.2f);
    Animation::Ptr rotateIn(Widget* widget, float fromAngle = -360, float duration = 0.5f);
}

} // namespace NexusForge::UI
