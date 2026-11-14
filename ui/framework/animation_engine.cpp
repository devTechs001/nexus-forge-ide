// ui/framework/animation_engine.cpp
#include "animation_engine.hpp"
#include <cmath>
#include <algorithm>

namespace NexusForge::UI {

// Bounce easing
float Easing::easeOutBounce(float t) {
    const float n1 = 7.5625f;
    const float d1 = 2.75f;
    
    if (t < 1 / d1) {
        return n1 * t * t;
    } else if (t < 2 / d1) {
        return n1 * (t -= 1.5f / d1) * t + 0.75f;
    } else if (t < 2.5 / d1) {
        return n1 * (t -= 2.25f / d1) * t + 0.9375f;
    } else {
        return n1 * (t -= 2.625f / d1) * t + 0.984375f;
    }
}

// Animation implementation
Animation::Animation() = default;

Animation& Animation::setEasingPreset(const std::string& preset) {
    if (preset == "linear") easing_ = Easing::linear;
    else if (preset == "easeInQuad") easing_ = Easing::easeInQuad;
    else if (preset == "easeOutQuad") easing_ = Easing::easeOutQuad;
    else if (preset == "easeInOutQuad") easing_ = Easing::easeInOutQuad;
    else if (preset == "easeInCubic") easing_ = Easing::easeInCubic;
    else if (preset == "easeOutCubic") easing_ = Easing::easeOutCubic;
    else if (preset == "easeInOutCubic") easing_ = Easing::easeInOutCubic;
    else if (preset == "easeInQuart") easing_ = Easing::easeInQuart;
    else if (preset == "easeOutQuart") easing_ = Easing::easeOutQuart;
    else if (preset == "easeInOutQuart") easing_ = Easing::easeInOutQuart;
    else if (preset == "easeInQuint") easing_ = Easing::easeInQuint;
    else if (preset == "easeOutQuint") easing_ = Easing::easeOutQuint;
    else if (preset == "easeInOutQuint") easing_ = Easing::easeInOutQuint;
    else if (preset == "easeInSine") easing_ = Easing::easeInSine;
    else if (preset == "easeOutSine") easing_ = Easing::easeOutSine;
    else if (preset == "easeInOutSine") easing_ = Easing::easeInOutSine;
    else if (preset == "easeInExpo") easing_ = Easing::easeInExpo;
    else if (preset == "easeOutExpo") easing_ = Easing::easeOutExpo;
    else if (preset == "easeInOutExpo") easing_ = Easing::easeInOutExpo;
    else if (preset == "easeInCirc") easing_ = Easing::easeInCirc;
    else if (preset == "easeOutCirc") easing_ = Easing::easeOutCirc;
    else if (preset == "easeInOutCirc") easing_ = Easing::easeInOutCirc;
    else if (preset == "easeInBack") easing_ = Easing::easeInBack;
    else if (preset == "easeOutBack") easing_ = Easing::easeOutBack;
    else if (preset == "easeInOutBack") easing_ = Easing::easeInOutBack;
    else if (preset == "easeInElastic") easing_ = Easing::easeInElastic;
    else if (preset == "easeOutElastic") easing_ = Easing::easeOutElastic;
    else if (preset == "easeInOutElastic") easing_ = Easing::easeInOutElastic;
    else if (preset == "easeInBounce") easing_ = Easing::easeInBounce;
    else if (preset == "easeOutBounce") easing_ = Easing::easeOutBounce;
    else if (preset == "easeInOutBounce") easing_ = Easing::easeInOutBounce;
    return *this;
}

void Animation::start() {
    running_ = true;
    paused_ = false;
    complete_ = false;
    elapsed_ = 0;
    progress_ = 0;
    currentValue_ = startValue_;
}

void Animation::stop() {
    running_ = false;
    paused_ = false;
}

void Animation::pause() {
    paused_ = true;
}

void Animation::resume() {
    paused_ = false;
}

void Animation::seek(float time) {
    elapsed_ = std::clamp(time, 0.0f, duration_);
    progress_ = duration_ > 0 ? elapsed_ / duration_ : 1.0f;
    currentValue_ = interpolate(progress_);
    if (onUpdate_) {
        onUpdate_(currentValue_);
    }
}

void Animation::update(float deltaTime) {
    if (!running_ || paused_) return;
    
    // Handle delay
    if (delay_ > 0) {
        delay_ -= deltaTime;
        if (delay_ < 0) {
            deltaTime = -delay_;
            delay_ = 0;
        } else {
            return;
        }
    }
    
    elapsed_ += deltaTime;
    
    if (elapsed_ >= duration_) {
        if (loop_) {
            elapsed_ -= duration_;
            if (pingPong_) {
                pingPongForward_ = !pingPongForward_;
            }
        } else {
            elapsed_ = duration_;
            complete_ = true;
            running_ = false;
        }
    }
    
    progress_ = duration_ > 0 ? elapsed_ / duration_ : 1.0f;
    currentValue_ = interpolate(progress_);
    
    if (onUpdate_) {
        onUpdate_(currentValue_);
    }
    
    if (complete_ && onComplete_) {
        onComplete_();
    }
}

float Animation::interpolate(float t) {
    if (pingPong_ && !pingPongForward_) {
        t = 1.0f - t;
    }
    
    if (easing_) {
        t = easing_(t);
    }
    
    return startValue_ + (endValue_ - startValue_) * t;
}

// Animation Manager implementation
AnimationManager& AnimationManager::getInstance() {
    static AnimationManager instance;
    return instance;
}

std::shared_ptr<Animation> AnimationManager::createAnimation() {
    auto anim = std::make_shared<Animation>();
    animations_.push_back(anim);
    return anim;
}

std::shared_ptr<Animation> AnimationManager::fadeIn(float duration) {
    auto anim = std::make_shared<Animation>();
    anim->setDuration(duration);
    anim->setEasingPreset("easeOutQuad");
    anim->from(0.0f);
    anim->to(1.0f);
    return anim;
}

std::shared_ptr<Animation> AnimationManager::fadeOut(float duration) {
    auto anim = std::make_shared<Animation>();
    anim->setDuration(duration);
    anim->setEasingPreset("easeInQuad");
    anim->from(1.0f);
    anim->to(0.0f);
    return anim;
}

std::shared_ptr<Animation> AnimationManager::slideIn(float duration) {
    auto anim = std::make_shared<Animation>();
    anim->setDuration(duration);
    anim->setEasingPreset("easeOutCubic");
    anim->from(-100.0f);
    anim->to(0.0f);
    return anim;
}

std::shared_ptr<Animation> AnimationManager::slideOut(float duration) {
    auto anim = std::make_shared<Animation>();
    anim->setDuration(duration);
    anim->setEasingPreset("easeInCubic");
    anim->from(0.0f);
    anim->to(100.0f);
    return anim;
}

std::shared_ptr<Animation> AnimationManager::scaleIn(float duration) {
    auto anim = std::make_shared<Animation>();
    anim->setDuration(duration);
    anim->setEasingPreset("easeOutBack");
    anim->from(0.0f);
    anim->to(1.0f);
    return anim;
}

std::shared_ptr<Animation> AnimationManager::scaleOut(float duration) {
    auto anim = std::make_shared<Animation>();
    anim->setDuration(duration);
    anim->setEasingPreset("easeInBack");
    anim->from(1.0f);
    anim->to(0.0f);
    return anim;
}

void AnimationManager::update(float deltaTime) {
    for (auto it = animations_.begin(); it != animations_.end();) {
        if (*it && (*it)->isRunning()) {
            (*it)->update(deltaTime);
            ++it;
        } else {
            it = animations_.erase(it);
        }
    }
}

void AnimationManager::stopAll() {
    for (auto& anim : animations_) {
        if (anim) {
            anim->stop();
        }
    }
    animations_.clear();
}

} // namespace NexusForge::UI
