// ui/screens/splash_screen.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include "../renderer/render_engine.hpp"
#include "../../core/engine/nexus_core.hpp"

#include <string>
#include <functional>
#include <chrono>

namespace NexusForge::UI {

class SplashScreen {
public:
    SplashScreen();
    ~SplashScreen();

    bool initialize(Core::NexusEngine* engine);
    void show();
    void hide();
    void close();

    void setProgress(float progress, const std::string& message = "");
    void setPhase(const std::string& phase);

    void update(double deltaTime);
    void render();

    bool isVisible() const { return visible_; }

    std::function<void()> onClosed;

private:
    Core::NexusEngine* engine_ = nullptr;
    std::unique_ptr<RenderEngine> renderer_;

    bool visible_ = false;
    float progress_ = 0.0f;
    float displayedProgress_ = 0.0f;
    std::string message_;
    std::string phase_;

    // Animation
    float logoScale_ = 0.0f;
    float logoAlpha_ = 0.0f;
    float textAlpha_ = 0.0f;
    float progressBarAlpha_ = 0.0f;
    float particleTime_ = 0.0f;

    // Timing
    std::chrono::steady_clock::time_point startTime_;
    float fadeInDuration_ = 0.5f;
    float minimumDisplayTime_ = 2.0f;

    // Resources
    TextureHandle logoTexture_ = 0;
    TextureHandle backgroundTexture_ = 0;

    // Particles
    struct Particle {
        float x, y;
        float vx, vy;
        float size;
        float alpha;
        float life;
    };
    std::vector<Particle> particles_;

    void renderBackground();
    void renderLogo();
    void renderProgressBar();
    void renderMessage();
    void renderParticles();
    void updateParticles(double deltaTime);
    void spawnParticle();

    // Platform-specific window handle
    void* nativeWindow_ = nullptr;
    bool createNativeWindow();
    void destroyNativeWindow();

    // Easing functions
    float easeOutQuad(float t);
    float easeOutBack(float t);
};

} // namespace NexusForge::UI
