// ui/framework/animation_engine.hpp
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>
#include <cmath>

namespace NexusForge::UI {

// Easing functions
class Easing {
public:
    static float linear(float t) { return t; }
    static float easeInQuad(float t) { return t * t; }
    static float easeOutQuad(float t) { return t * (2 - t); }
    static float easeInOutQuad(float t) { return t < 0.5f ? 2 * t * t : -1 + (4 - 2 * t) * t; }
    static float easeInCubic(float t) { return t * t * t; }
    static float easeOutCubic(float t) { return 1 - std::pow(1 - t, 3); }
    static float easeInOutCubic(float t) { return t < 0.5f ? 4 * t * t * t : 1 - std::pow(-2 * t + 2, 3) / 2; }
    static float easeInQuart(float t) { return t * t * t * t; }
    static float easeOutQuart(float t) { return 1 - std::pow(1 - t, 4); }
    static float easeInOutQuart(float t) { return t < 0.5f ? 8 * t * t * t * t : 1 - std::pow(-2 * t + 2, 4) / 2; }
    static float easeInQuint(float t) { return t * t * t * t * t; }
    static float easeOutQuint(float t) { return 1 - std::pow(1 - t, 5); }
    static float easeInOutQuint(float t) { return t < 0.5f ? 16 * t * t * t * t * t : 1 - std::pow(-2 * t + 2, 5) / 2; }
    static float easeInSine(float t) { return 1 - std::cos(t * 3.14159f / 2); }
    static float easeOutSine(float t) { return std::sin(t * 3.14159f / 2); }
    static float easeInOutSine(float t) { return -(std::cos(3.14159f * t) - 1) / 2; }
    static float easeInExpo(float t) { return t == 0 ? 0 : std::pow(2, 10 * t - 10); }
    static float easeOutExpo(float t) { return t == 1 ? 1 : 1 - std::pow(2, -10 * t); }
    static float easeInOutExpo(float t) {
        return t == 0 ? 0 : t == 1 ? 1 : t < 0.5f ? std::pow(2, 20 * t - 10) / 2 : (2 - std::pow(2, -20 * t + 10)) / 2;
    }
    static float easeInCirc(float t) { return 1 - std::sqrt(1 - std::pow(t, 2)); }
    static float easeOutCirc(float t) { return std::sqrt(1 - std::pow(t - 1, 2)); }
    static float easeInOutCirc(float t) {
        return t < 0.5f ? (1 - std::sqrt(1 - std::pow(2 * t, 2))) / 2 : (std::sqrt(1 - std::pow(-2 * t + 2, 2)) + 1) / 2;
    }
    static float easeInBack(float t) {
        const float c1 = 1.70158f;
        const float c3 = c1 + 1;
        return c3 * t * t * t - c1 * t * t;
    }
    static float easeOutBack(float t) {
        const float c1 = 1.70158f;
        const float c3 = c1 + 1;
        return 1 + c3 * std::pow(t - 1, 3) + c1 * std::pow(t - 1, 2);
    }
    static float easeInOutBack(float t) {
        const float c1 = 1.70158f;
        const float c2 = c1 * 1.525f;
        return t < 0.5f ? (std::pow(2 * t, 2) * ((c2 + 1) * 2 * t - c2)) / 2 
                        : (std::pow(2 * t - 2, 2) * ((c2 + 1) * (t * 2 - 2) + c2) + 2) / 2;
    }
    static float easeInElastic(float t) {
        const float c4 = (2 * 3.14159f) / 3;
        return t == 0 ? 0 : t == 1 ? 1 : -std::pow(2, 10 * t - 10) * std::sin((t * 10 - 10.75f) * c4);
    }
    static float easeOutElastic(float t) {
        const float c4 = (2 * 3.14159f) / 3;
        return t == 0 ? 0 : t == 1 ? 1 : std::pow(2, -10 * t) * std::sin((t * 10 - 0.75f) * c4) + 1;
    }
    static float easeInOutElastic(float t) {
        const float c5 = (2 * 3.14159f) / 4.5f;
        return t == 0 ? 0 : t == 1 ? 1 
               : t < 0.5f ? -(std::pow(2, 20 * t - 10) * std::sin((20 * t - 11.125f) * c5)) / 2 
               : (std::pow(2, -20 * t + 10) * std::sin((20 * t - 11.125f) * c5)) / 2 + 1;
    }
    static float easeOutBounce(float t);
    static float easeInBounce(float t) { return 1 - easeOutBounce(1 - t); }
    static float easeInOutBounce(float t) {
        return t < 0.5f ? (1 - easeOutBounce(1 - 2 * t)) / 2 : (1 + easeOutBounce(2 * t - 1)) / 2;
    }
};

// Animation types
enum class AnimationType {
    Fade,
    Slide,
    Scale,
    Rotate,
    Color,
    Size,
    Position,
    Custom
};

// Animation class
class Animation {
public:
    using EasingFunction = std::function<float(float)>;
    using UpdateCallback = std::function<void(float)>;
    using CompleteCallback = std::function<void()>;

    Animation();
    
    // Configuration
    Animation& setDuration(float seconds) { duration_ = seconds; return *this; }
    Animation& setDelay(float seconds) { delay_ = seconds; return *this; }
    Animation& setEasing(EasingFunction easing) { easing_ = std::move(easing); return *this; }
    Animation& setEasingPreset(const std::string& preset);
    Animation& setLoop(bool loop, bool pingPong = false) { loop_ = loop; pingPong_ = pingPong; return *this; }
    
    // Value animation
    Animation& from(float value) { startValue_ = value; return *this; }
    Animation& to(float value) { endValue_ = value; return *this; }
    
    // Callbacks
    Animation& onUpdate(UpdateCallback callback) { onUpdate_ = std::move(callback); return *this; }
    Animation& onComplete(CompleteCallback callback) { onComplete_ = std::move(callback); return *this; }
    
    // Control
    void start();
    void stop();
    void pause();
    void resume();
    void seek(float time);
    
    // State
    bool isRunning() const { return running_; }
    bool isPaused() const { return paused_; }
    bool isComplete() const { return complete_; }
    float getProgress() const { return progress_; }
    float getCurrentValue() const { return currentValue_; }
    
    // Update
    void update(float deltaTime);

private:
    float duration_ = 0.5f;
    float delay_ = 0;
    float elapsed_ = 0;
    float progress_ = 0;
    float startValue_ = 0;
    float endValue_ = 1;
    float currentValue_ = 0;
    bool running_ = false;
    bool paused_ = false;
    bool complete_ = false;
    bool loop_ = false;
    bool pingPong_ = false;
    bool pingPongForward_ = true;
    
    EasingFunction easing_;
    UpdateCallback onUpdate_;
    CompleteCallback onComplete_;
    
    float interpolate(float t);
};

// Animation Manager
class AnimationManager {
public:
    static AnimationManager& getInstance();
    
    // Create animations
    std::shared_ptr<Animation> createAnimation();
    std::shared_ptr<Animation> fadeIn(float duration = 0.3f);
    std::shared_ptr<Animation> fadeOut(float duration = 0.3f);
    std::shared_ptr<Animation> slideIn(float duration = 0.3f);
    std::shared_ptr<Animation> slideOut(float duration = 0.3f);
    std::shared_ptr<Animation> scaleIn(float duration = 0.3f);
    std::shared_ptr<Animation> scaleOut(float duration = 0.3f);

    // Update all animations
    void update(float deltaTime);

    // Stop all animations
    void stopAll();

private:
    AnimationManager() = default;
    std::vector<std::shared_ptr<Animation>> animations_;
};

} // namespace NexusForge::UI
