// ui/screens/splash_screen.cpp
#include "splash_screen.hpp"
#include "../renderer/software_backend.hpp"
#include "../../core/platform/platform_abstraction.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
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
    auto backend = std::make_unique<SoftwareRenderBackend>(nativeWindow_);
    renderer_->setBackend(std::move(backend));
    if (!renderer_->initialize(RenderAPI::Auto)) {
        return false;
    }

    // Only load image textures when files actually exist on disk; otherwise draw vector artwork
    if (Platform::PlatformAbstraction::fileExists("resources/splash/logo.png")) {
        logoTexture_ = renderer_->loadTexture("resources/splash/logo.png");
    }
    if (Platform::PlatformAbstraction::fileExists("resources/splash/background.png")) {
        backgroundTexture_ = renderer_->loadTexture("resources/splash/background.png");
    }

    return true;
}

void SplashScreen::show() {
    visible_ = true;
    startTime_ = std::chrono::steady_clock::now();

    // Initial animation state
    logoScale_ = 0.55f;
    logoAlpha_ = 0.0f;
    textAlpha_ = 0.0f;
    progressBarAlpha_ = 0.0f;
    pulseTime_ = 0.0f;

    particles_.clear();
    for (int i = 0; i < 45; ++i) {
        spawnParticle(true);
    }

    if (nativeWindow_) {
        Platform::PlatformAbstraction::showWindow(nativeWindow_);
    }
}

void SplashScreen::hide() {
    visible_ = false;
    if (nativeWindow_) {
        Platform::PlatformAbstraction::hideWindow(nativeWindow_);
    }
}

void SplashScreen::close() {
    bool wasOpen = visible_ || renderer_ != nullptr || nativeWindow_ != nullptr;
    hide();

    if (renderer_) {
        if (logoTexture_) {
            renderer_->destroyTexture(logoTexture_);
            logoTexture_ = 0;
        }
        if (backgroundTexture_) {
            renderer_->destroyTexture(backgroundTexture_);
            backgroundTexture_ = 0;
        }
        renderer_.reset();
    } else {
        logoTexture_ = 0;
        backgroundTexture_ = 0;
    }

    destroyNativeWindow();

    if (wasOpen && onClosed) {
        onClosed();
    }
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

bool SplashScreen::isMinimumTimeElapsed() const {
    float elapsed = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - startTime_).count();
    return elapsed >= minimumDisplayTime_;
}

bool SplashScreen::isCloseRequested() const {
    return nativeWindow_ && Platform::PlatformAbstraction::isWindowCloseRequested(nativeWindow_);
}

bool SplashScreen::saveFramePPM(const std::string& path) const {
    if (!renderer_) return false;
    auto* sw = dynamic_cast<SoftwareRenderBackend*>(renderer_->getBackend());
    return sw ? sw->savePPM(path) : false;
}

void SplashScreen::update(double deltaTime) {
    if (!visible_) return;

    float dt = std::clamp(static_cast<float>(deltaTime), 0.0f, 0.1f);
    pulseTime_ += dt;

    // Smoothly interpolate displayed progress toward target progress
    float progressDiff = progress_ - displayedProgress_;
    displayedProgress_ += progressDiff * std::min(1.0f, dt * 6.5f);
    if (std::abs(progress_ - displayedProgress_) < 0.002f) {
        displayedProgress_ = progress_;
    }

    float elapsed = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - startTime_).count();

    // Logo zoom & fade in
    float logoProgress = std::clamp(elapsed / fadeInDuration_, 0.0f, 1.0f);
    logoScale_ = 0.55f + 0.45f * easeOutBack(logoProgress);
    logoAlpha_ = easeOutQuad(logoProgress);

    // Title & badge fade in
    float textProgress = std::clamp((elapsed - 0.18f) / 0.35f, 0.0f, 1.0f);
    textAlpha_ = easeOutQuad(textProgress);

    // Progress bar & status fade in
    float barProgress = std::clamp((elapsed - 0.32f) / 0.35f, 0.0f, 1.0f);
    progressBarAlpha_ = easeOutQuad(barProgress);

    updateParticles(dt);

    particleTime_ += dt;
    if (particleTime_ > 0.08f) {
        spawnParticle(false);
        particleTime_ = 0.0f;
    }
}

void SplashScreen::render() {
    if (!visible_ || !renderer_) return;

    renderer_->beginFrame();
    renderer_->setViewport(0, 0, SPLASH_WIDTH, SPLASH_HEIGHT);
    renderer_->setProjection(Mat4::ortho(0.0f, static_cast<float>(SPLASH_WIDTH),
                                         static_cast<float>(SPLASH_HEIGHT), 0.0f, -1.0f, 1.0f));
    renderer_->clear(Color(0.055f, 0.065f, 0.11f, 1.0f));

    renderBackground();
    renderParticles();
    renderLogo();
    renderProgressBar();
    renderMessage();
    renderFrameBorder();

    renderer_->endFrame();
    renderer_->present();
}

void SplashScreen::renderBackground() {
    Rect full(0.0f, 0.0f, static_cast<float>(SPLASH_WIDTH), static_cast<float>(SPLASH_HEIGHT));

    if (backgroundTexture_) {
        renderer_->drawImage(backgroundTexture_, full);
    } else {
        // Deep cosmic vertical gradient
        renderer_->drawGradientRect(
            full,
            Color(0.075f, 0.090f, 0.165f, 1.0f),
            Color(0.035f, 0.042f, 0.080f, 1.0f)
        );
    }

    // Soft radial indigo/blue ambient aura centered behind the logo
    float auraPulse = 0.16f + 0.04f * std::sin(pulseTime_ * 2.2f);
    renderer_->drawRadialGradient(
        Rect(120.0f, -20.0f, 400.0f, 320.0f),
        Color(0.28f, 0.48f, 1.0f, auraPulse * logoAlpha_),
        Color(0.28f, 0.48f, 1.0f, 0.0f)
    );

    // Subtle secondary violet glow on bottom-right
    renderer_->drawRadialGradient(
        Rect(340.0f, 200.0f, 340.0f, 260.0f),
        Color(0.55f, 0.30f, 0.95f, 0.10f),
        Color(0.55f, 0.30f, 0.95f, 0.0f)
    );

    // Subtle architectural dot grid
    for (int x = 24; x < SPLASH_WIDTH - 16; x += 28) {
        for (int y = 24; y < SPLASH_HEIGHT - 16; y += 28) {
            float alpha = 0.055f + 0.025f * std::sin(x * 0.05f + y * 0.05f + pulseTime_);
            renderer_->drawRect(Rect(static_cast<float>(x), static_cast<float>(y), 2.0f, 2.0f),
                                Color(0.65f, 0.78f, 1.0f, alpha));
        }
    }

    // Sleek top accent hairline gradient
    renderer_->drawHorizontalGradient(
        Rect(0.0f, 0.0f, static_cast<float>(SPLASH_WIDTH), 3.0f),
        Color(0.24f, 0.55f, 1.0f, 0.95f),
        Color(0.62f, 0.36f, 1.0f, 0.95f)
    );
}

void SplashScreen::renderLogo() {
    float centerX = static_cast<float>(SPLASH_WIDTH) * 0.5f;
    float centerY = 122.0f;

    renderer_->pushTransform();
    renderer_->translate(centerX, centerY);
    renderer_->scale(logoScale_, logoScale_);

    if (logoTexture_) {
        renderer_->drawImage(
            logoTexture_,
            Rect(-58.0f, -58.0f, 116.0f, 116.0f),
            Color(1.0f, 1.0f, 1.0f, logoAlpha_)
        );
    } else {
        // Outer subtle pulsing orbit ring
        float ringRadius = 60.0f + 2.5f * std::sin(pulseTime_ * 3.0f);
        renderer_->drawCircle(0.0f, 0.0f, ringRadius,
                              Color(0.40f, 0.65f, 1.0f, 0.22f * logoAlpha_), false);

        // Outer hexagon shadow / rim
        auto drawHex = [&](float radius, const Color& col, bool filled, float thick = 2.0f) {
            renderer_->beginPath();
            for (int i = 0; i < 6; ++i) {
                float angle = i * 3.14159265f / 3.0f - 3.14159265f / 6.0f;
                float hx = std::cos(angle) * radius;
                float hy = std::sin(angle) * radius;
                if (i == 0) renderer_->moveTo(hx, hy);
                else renderer_->lineTo(hx, hy);
            }
            renderer_->closePath();
            if (filled) renderer_->fillPath(col);
            else renderer_->strokePath(col, thick);
        };

        drawHex(52.0f, Color(0.26f, 0.52f, 0.98f, logoAlpha_), true);
        drawHex(45.0f, Color(0.11f, 0.15f, 0.28f, logoAlpha_), true);
        drawHex(45.0f, Color(0.52f, 0.72f, 1.0f, 0.85f * logoAlpha_), false, 2.0f);

        // Crisp centered "NF" monogram inside the hexagon
        renderer_->drawTextCentered(
            "NF",
            Rect(-36.0f, -22.0f, 72.0f, 44.0f),
            Color(1.0f, 1.0f, 1.0f, logoAlpha_),
            "default", 34.0f
        );
    }

    renderer_->popTransform();

    // Application title
    renderer_->drawTextCentered(
        "NEXUSFORGE IDE",
        Rect(0.0f, 194.0f, static_cast<float>(SPLASH_WIDTH), 36.0f),
        Color(0.96f, 0.97f, 1.0f, textAlpha_),
        "default", 26.0f
    );

    // Tagline
    renderer_->drawTextCentered(
        "Code Without Boundaries",
        Rect(0.0f, 232.0f, static_cast<float>(SPLASH_WIDTH), 22.0f),
        Color(0.62f, 0.69f, 0.84f, textAlpha_ * 0.92f),
        "default", 14.0f
    );

    // Phoenix Edition pill badge
    Rect pill(centerX - 88.0f, 260.0f, 176.0f, 22.0f);
    renderer_->drawRoundedRect(pill, Color(0.22f, 0.38f, 0.78f, 0.28f * textAlpha_), 11.0f);
    renderer_->drawTextCentered(
        "PHOENIX EDITION  v1.0.0",
        pill,
        Color(0.58f, 0.78f, 1.0f, textAlpha_),
        "default", 10.5f
    );
}

void SplashScreen::renderProgressBar() {
    float barWidth = 460.0f;
    float barX = (static_cast<float>(SPLASH_WIDTH) - barWidth) * 0.5f;
    float barY = 308.0f;
    float barHeight = 7.0f;

    // Track background
    renderer_->drawRoundedRect(
        Rect(barX, barY, barWidth, barHeight),
        Color(0.14f, 0.17f, 0.27f, progressBarAlpha_),
        3.5f
    );

    // Progress fill
    float fillWidth = barWidth * std::clamp(displayedProgress_, 0.0f, 1.0f);
    if (fillWidth > 1.0f) {
        renderer_->drawHorizontalGradient(
            Rect(barX, barY, fillWidth, barHeight),
            Color(0.25f, 0.58f, 1.0f, progressBarAlpha_),
            Color(0.62f, 0.38f, 1.0f, progressBarAlpha_)
        );

        // Soft glowing highlight at progress head
        float glowX = std::clamp(barX + fillWidth - 8.0f, barX, barX + barWidth - 16.0f);
        renderer_->drawRoundedRect(
            Rect(glowX, barY - 3.0f, 16.0f, barHeight + 6.0f),
            Color(0.65f, 0.75f, 1.0f, progressBarAlpha_ * 0.45f),
            6.0f
        );
    }

    // Percentage readout flush-right above bar
    char pctBuf[16];
    std::snprintf(pctBuf, sizeof(pctBuf), "%d%%",
                  static_cast<int>(std::lround(displayedProgress_ * 100.0f)));
    float pctW = renderer_->measureText(pctBuf, "default", 11.0f).x;
    renderer_->drawText(
        pctBuf,
        barX + barWidth - pctW, barY - 17.0f,
        Color(0.60f, 0.76f, 1.0f, progressBarAlpha_),
        "default", 11.0f
    );
}

void SplashScreen::renderMessage() {
    float barWidth = 460.0f;
    float barX = (static_cast<float>(SPLASH_WIDTH) - barWidth) * 0.5f;

    // Phase headline (left-aligned above progress bar)
    if (!phase_.empty()) {
        renderer_->drawText(
            phase_,
            barX, 290.0f,
            Color(0.86f, 0.90f, 0.98f, progressBarAlpha_),
            "default", 12.0f
        );
    }

    // Detailed message (centered below progress bar)
    if (!message_.empty()) {
        renderer_->drawTextCentered(
            message_,
            Rect(40.0f, 326.0f, static_cast<float>(SPLASH_WIDTH) - 80.0f, 20.0f),
            Color(0.56f, 0.62f, 0.76f, progressBarAlpha_ * 0.95f),
            "default", 11.5f
        );
    }

    // Bottom subtle divider + footer
    renderer_->drawLine(24.0f, 384.0f, static_cast<float>(SPLASH_WIDTH) - 24.0f, 384.0f,
                        Color(0.22f, 0.27f, 0.40f, 0.45f * progressBarAlpha_), 1.0f);

    renderer_->drawText(
        "Version 1.0.0  (Build 2024.12.001)",
        24.0f, 394.0f,
        Color(0.45f, 0.50f, 0.64f, progressBarAlpha_ * 0.85f),
        "default", 10.5f
    );

    const std::string copyright = "(c) 2026 NexusForge Team";
    float copyW = renderer_->measureText(copyright, "default", 10.5f).x;
    renderer_->drawText(
        copyright,
        static_cast<float>(SPLASH_WIDTH) - 24.0f - copyW, 394.0f,
        Color(0.45f, 0.50f, 0.64f, progressBarAlpha_ * 0.85f),
        "default", 10.5f
    );
}

void SplashScreen::renderFrameBorder() {
    float w = static_cast<float>(SPLASH_WIDTH);
    float h = static_cast<float>(SPLASH_HEIGHT);
    Color border(0.24f, 0.32f, 0.52f, 0.75f);
    renderer_->drawLine(0.0f, 0.5f, w, 0.5f, border, 1.0f);
    renderer_->drawLine(0.0f, h - 0.5f, w, h - 0.5f, border, 1.0f);
    renderer_->drawLine(0.5f, 0.0f, 0.5f, h, border, 1.0f);
    renderer_->drawLine(w - 0.5f, 0.0f, w - 0.5f, h, border, 1.0f);
}

void SplashScreen::renderParticles() {
    for (const auto& p : particles_) {
        Color color(0.48f, 0.68f, 1.0f, p.alpha * 0.35f);
        renderer_->drawCircle(p.x, p.y, p.size, color, true);
    }
}

void SplashScreen::updateParticles(double deltaTime) {
    float dt = static_cast<float>(deltaTime);

    for (auto it = particles_.begin(); it != particles_.end();) {
        it->x += it->vx * dt;
        it->y += it->vy * dt;
        it->life -= dt;
        float norm = std::clamp(it->life / std::max(0.1f, it->maxLife), 0.0f, 1.0f);
        // Smooth fade in & fade out
        it->alpha = std::sin(norm * 3.14159265f);

        if (it->life <= 0.0f || it->x < -30.0f || it->x > SPLASH_WIDTH + 30.0f ||
            it->y < -30.0f || it->y > SPLASH_HEIGHT + 40.0f) {
            it = particles_.erase(it);
        } else {
            ++it;
        }
    }
}

void SplashScreen::spawnParticle(bool randomY) {
    if (particles_.size() >= 75) return;

    static std::mt19937 gen(0x4E464944U);
    static std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    Particle p;
    p.x = dist(gen) * static_cast<float>(SPLASH_WIDTH);
    p.y = randomY ? (dist(gen) * static_cast<float>(SPLASH_HEIGHT))
                  : (static_cast<float>(SPLASH_HEIGHT) + dist(gen) * 20.0f);
    p.vx = (dist(gen) - 0.5f) * 18.0f;
    p.vy = -20.0f - dist(gen) * 35.0f;
    p.size = 1.5f + dist(gen) * 3.0f;
    p.maxLife = 3.0f + dist(gen) * 4.0f;
    p.life = randomY ? (dist(gen) * p.maxLife) : p.maxLife;
    p.alpha = 0.0f;

    particles_.push_back(p);
}

// Easing functions
float SplashScreen::easeOutQuad(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t);
}

float SplashScreen::easeOutBack(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
}

bool SplashScreen::createNativeWindow() {
    nativeWindow_ = Platform::PlatformAbstraction::createSplashWindow(SPLASH_WIDTH, SPLASH_HEIGHT);
    return nativeWindow_ != nullptr;
}

void SplashScreen::destroyNativeWindow() {
    if (nativeWindow_) {
        Platform::PlatformAbstraction::destroyWindow(nativeWindow_);
        nativeWindow_ = nullptr;
    }
}

} // namespace NexusForge::UI
