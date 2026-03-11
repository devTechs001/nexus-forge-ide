// ui/screens/splash_screen.cpp
#include "splash_screen.hpp"
#include "../../core/platform/platform_abstraction.hpp"

#include <cmath>
#include <random>

namespace NexusForge::UI {

SplashScreen::SplashScreen() {
    particles_.reserve(100);
}

SplashScreen::~SplashScreen() {
    close();
}

bool SplashScreen::initialize(Core::NexusEngine* engine) {
    engine_ = engine;

    if (!createNativeWindow()) {
        return false;
    }

    renderer_ = std::make_unique<RenderEngine>();
    if (!renderer_->initialize(RenderAPI::Auto)) {
        return false;
    }

    // Load resources
    logoTexture_ = renderer_->loadTexture("resources/splash/logo.png");
    backgroundTexture_ = renderer_->loadTexture("resources/splash/background.png");

    return true;
}

void SplashScreen::show() {
    visible_ = true;
    startTime_ = std::chrono::steady_clock::now();

    // Initial animation state
    logoScale_ = 0.5f;
    logoAlpha_ = 0.0f;
    textAlpha_ = 0.0f;
    progressBarAlpha_ = 0.0f;

    // Spawn initial particles
    for (int i = 0; i < 50; ++i) {
        spawnParticle();
    }
}

void SplashScreen::hide() {
    visible_ = false;
}

void SplashScreen::close() {
    hide();

    if (logoTexture_) renderer_->destroyTexture(logoTexture_);
    if (backgroundTexture_) renderer_->destroyTexture(backgroundTexture_);

    renderer_.reset();
    destroyNativeWindow();

    if (onClosed) onClosed();
}

void SplashScreen::setProgress(float progress, const std::string& message) {
    progress_ = std::clamp(progress, 0.0f, 1.0f);
    if (!message.empty()) {
        message_ = message;
    }
}

void SplashScreen::setPhase(const std::string& phase) {
    phase_ = phase;
}

void SplashScreen::update(double deltaTime) {
    if (!visible_) return;

    float dt = static_cast<float>(deltaTime);

    // Animate progress bar smoothly
    float progressDiff = progress_ - displayedProgress_;
    displayedProgress_ += progressDiff * std::min(1.0f, dt * 5.0f);

    // Fade in animations
    float elapsed = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - startTime_).count();

    // Logo animation
    float logoProgress = std::min(1.0f, elapsed / fadeInDuration_);
    logoScale_ = 0.5f + 0.5f * easeOutBack(logoProgress);
    logoAlpha_ = easeOutQuad(logoProgress);

    // Text fade in (delayed)
    float textProgress = std::max(0.0f, std::min(1.0f, (elapsed - 0.3f) / 0.3f));
    textAlpha_ = easeOutQuad(textProgress);

    // Progress bar fade in (delayed)
    float barProgress = std::max(0.0f, std::min(1.0f, (elapsed - 0.5f) / 0.3f));
    progressBarAlpha_ = easeOutQuad(barProgress);

    // Update particles
    updateParticles(deltaTime);

    // Spawn new particles occasionally
    particleTime_ += dt;
    if (particleTime_ > 0.1f) {
        spawnParticle();
        particleTime_ = 0;
    }
}

void SplashScreen::render() {
    if (!visible_ || !renderer_) return;

    renderer_->beginFrame();

    // Set viewport
    int width = 600, height = 400;  // Splash screen size
    renderer_->setViewport(0, 0, width, height);
    renderer_->setProjection(Mat4::ortho(0, width, height, 0, -1, 1));

    // Clear
    renderer_->clear(Color(0.08f, 0.08f, 0.12f, 1.0f));

    renderBackground();
    renderParticles();
    renderLogo();
    renderProgressBar();
    renderMessage();

    renderer_->endFrame();
    renderer_->present();
}

void SplashScreen::renderBackground() {
    if (backgroundTexture_) {
        renderer_->drawImage(backgroundTexture_, Rect(0, 0, 600, 400));
    } else {
        // Gradient background
        renderer_->drawGradientRect(
            Rect(0, 0, 600, 400),
            Color(0.1f, 0.1f, 0.18f, 1.0f),
            Color(0.05f, 0.05f, 0.1f, 1.0f)
        );
    }

    // Subtle pattern overlay
    for (int i = 0; i < 600; i += 20) {
        for (int j = 0; j < 400; j += 20) {
            float alpha = 0.02f + 0.01f * std::sin(i * 0.1f + j * 0.1f);
            renderer_->drawRect(Rect(i, j, 1, 1), Color(1, 1, 1, alpha));
        }
    }
}

void SplashScreen::renderLogo() {
    float centerX = 300;
    float centerY = 150;
    float logoSize = 120 * logoScale_;

    renderer_->pushTransform();
    renderer_->translate(centerX, centerY);
    renderer_->scale(logoScale_, logoScale_);

    if (logoTexture_) {
        renderer_->drawImage(
            logoTexture_,
            Rect(-60, -60, 120, 120),
            Color(1, 1, 1, logoAlpha_)
        );
    } else {
        // Fallback: Draw stylized "NF" logo
        Color primaryColor(0.4f, 0.6f, 1.0f, logoAlpha_);
        Color secondaryColor(0.6f, 0.4f, 1.0f, logoAlpha_);

        // Hexagon shape
        renderer_->beginPath();
        for (int i = 0; i < 6; ++i) {
            float angle = i * 3.14159f / 3.0f - 3.14159f / 6.0f;
            float x = std::cos(angle) * 50;
            float y = std::sin(angle) * 50;
            if (i == 0) renderer_->moveTo(x, y);
            else renderer_->lineTo(x, y);
        }
        renderer_->closePath();
        renderer_->fillPath(primaryColor);

        // Inner design
        renderer_->drawText("NF", -25, 15, Color(1, 1, 1, logoAlpha_), "default", 36);
    }

    renderer_->popTransform();

    // App name
    renderer_->drawTextCentered(
        "NEXUSFORGE IDE",
        Rect(0, 220, 600, 40),
        Color(1, 1, 1, textAlpha_),
        "default", 28
    );

    // Tagline
    renderer_->drawTextCentered(
        "Code Without Boundaries",
        Rect(0, 255, 600, 25),
        Color(0.6f, 0.6f, 0.7f, textAlpha_ * 0.8f),
        "default", 14
    );
}

void SplashScreen::renderProgressBar() {
    float barX = 100;
    float barY = 320;
    float barWidth = 400;
    float barHeight = 4;

    // Background
    renderer_->drawRoundedRect(
        Rect(barX, barY, barWidth, barHeight),
        Color(0.2f, 0.2f, 0.25f, progressBarAlpha_),
        2
    );

    // Progress fill
    float fillWidth = barWidth * displayedProgress_;
    if (fillWidth > 0) {
        // Gradient fill
        renderer_->drawHorizontalGradient(
            Rect(barX, barY, fillWidth, barHeight),
            Color(0.4f, 0.6f, 1.0f, progressBarAlpha_),
            Color(0.6f, 0.4f, 1.0f, progressBarAlpha_)
        );

        // Glow effect
        renderer_->drawRoundedRect(
            Rect(barX + fillWidth - 10, barY - 2, 20, barHeight + 4),
            Color(0.5f, 0.5f, 1.0f, progressBarAlpha_ * 0.3f),
            4
        );
    }
}

void SplashScreen::renderMessage() {
    // Phase text
    if (!phase_.empty()) {
        renderer_->drawTextCentered(
            phase_,
            Rect(0, 340, 600, 20),
            Color(0.8f, 0.8f, 0.9f, progressBarAlpha_),
            "default", 12
        );
    }

    // Message text
    if (!message_.empty()) {
        renderer_->drawTextCentered(
            message_,
            Rect(0, 358, 600, 20),
            Color(0.5f, 0.5f, 0.6f, progressBarAlpha_ * 0.8f),
            "default", 11
        );
    }

    // Version info
    renderer_->drawText(
        "Version 1.0.0",
        10, 385,
        Color(0.4f, 0.4f, 0.5f, progressBarAlpha_ * 0.6f),
        "default", 10
    );

    // Copyright
    renderer_->drawTextCentered(
        "© 2024 NexusForge Team",
        Rect(0, 380, 600, 15),
        Color(0.4f, 0.4f, 0.5f, progressBarAlpha_ * 0.6f),
        "default", 10
    );
}

void SplashScreen::renderParticles() {
    for (const auto& p : particles_) {
        Color color(0.5f, 0.6f, 1.0f, p.alpha * 0.3f);
        renderer_->drawCircle(p.x, p.y, p.size, color, true);
    }
}

void SplashScreen::updateParticles(double deltaTime) {
    float dt = static_cast<float>(deltaTime);

    for (auto it = particles_.begin(); it != particles_.end();) {
        it->x += it->vx * dt;
        it->y += it->vy * dt;
        it->life -= dt;
        it->alpha = std::max(0.0f, it->life / 5.0f);

        if (it->life <= 0 || it->x < -50 || it->x > 650 ||
            it->y < -50 || it->y > 450) {
            it = particles_.erase(it);
        } else {
            ++it;
        }
    }
}

void SplashScreen::spawnParticle() {
    if (particles_.size() >= 100) return;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    Particle p;
    p.x = dist(gen) * 600;
    p.y = 400 + dist(gen) * 50;
    p.vx = (dist(gen) - 0.5f) * 20;
    p.vy = -30 - dist(gen) * 50;
    p.size = 2 + dist(gen) * 4;
    p.alpha = 0.5f + dist(gen) * 0.5f;
    p.life = 3 + dist(gen) * 4;

    particles_.push_back(p);
}

// Easing functions
float easeOutQuad(float t) {
    return 1 - (1 - t) * (1 - t);
}

float easeOutBack(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1;
    return 1 + c3 * std::pow(t - 1, 3) + c1 * std::pow(t - 1, 2);
}

bool SplashScreen::createNativeWindow() {
    return Platform::PlatformAbstraction::createSplashWindow(
        600, 400, &nativeWindow_);
}

void SplashScreen::destroyNativeWindow() {
    if (nativeWindow_) {
        Platform::PlatformAbstraction::destroyWindow(nativeWindow_);
        nativeWindow_ = nullptr;
    }
}

} // namespace NexusForge::UI
