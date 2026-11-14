// ui/screens/onboarding_screen.cpp
// Onboarding Screen Implementation

#include "onboarding_screen.hpp"
#include "../widgets/button.hpp"
#include <algorithm>

namespace NexusForge::UI {

OnboardingScreen::OnboardingScreen() = default;

OnboardingScreen::~OnboardingScreen() = default;

bool OnboardingScreen::initialize(Core::NexusEngine* engine, Config::SettingsManager* settings) {
    engine_ = engine;
    settings_ = settings;
    
    createSteps();
    
    return true;
}

void OnboardingScreen::createSteps() {
    steps_.clear();

    // Step 1: Welcome
    OnboardingStep welcome;
    welcome.title = "Welcome to NexusForge IDE";
    welcome.description = "A powerful, AI-powered integrated development environment built for modern developers.";
    welcome.icon = "🚀";
    welcome.features = {
        "Cross-platform support (Windows, Linux, macOS)",
        "AI-powered code completion and chat",
        "Built-in terminal and debugger",
        "Extension marketplace",
        "Mobile-first responsive design"
    };
    steps_.push_back(welcome);
    
    // Step 2: Theme Selection
    OnboardingStep theme;
    theme.title = "Choose Your Theme";
    theme.description = "Personalize your development environment with your preferred color scheme.";
    theme.icon = "🎨";
    theme.features = {
        "NexusForge Dark (default)",
        "NexusForge Light",
        "Monokai",
        "GitHub Dark",
        "And many more..."
    };
    theme.action = [this]() {
        // Open theme selector
    };
    steps_.push_back(theme);
    
    // Step 3: AI Setup
    OnboardingStep ai;
    ai.title = "AI Features Setup";
    ai.description = "Configure AI-powered features to boost your productivity.";
    ai.icon = "🤖";
    ai.features = {
        "Code completion with AI",
        "AI chat assistant",
        "Code explanation and refactoring",
        "Bug detection and suggestions",
        "Support for OpenAI, Anthropic, and local models"
    };
    ai.action = [this]() {
        // Open AI settings
    };
    steps_.push_back(ai);
    
    // Step 4: Keybindings
    OnboardingStep keybindings;
    keybindings.title = "Keyboard Shortcuts";
    keybindings.description = "NexusForge comes with sensible defaults. You can customize them later.";
    keybindings.icon = "⌨️";
    keybindings.features = {
        "Ctrl+P: Quick open files",
        "Ctrl+Shift+P: Command palette",
        "Ctrl+`: Toggle terminal",
        "F12: Go to definition",
        "Ctrl+/: Toggle line comment"
    };
    steps_.push_back(keybindings);
    
    // Step 5: Extensions
    OnboardingStep extensions;
    extensions.title = "Extend Your IDE";
    extensions.description = "Install extensions to add new languages, themes, and features.";
    extensions.icon = "🔌";
    extensions.features = {
        "Language support extensions",
        "Theme extensions",
        "Debugger extensions",
        "Productivity tools",
        "Browse the marketplace anytime"
    };
    extensions.action = [this]() {
        // Open extensions marketplace
    };
    steps_.push_back(extensions);
    
    // Step 6: Get Started
    OnboardingStep started;
    started.title = "You're All Set!";
    started.description = "Start coding and enjoy the NexusForge experience.";
    started.icon = "✨";
    started.features = {
        "Create a new project",
        "Open an existing folder",
        "Clone from version control",
        "Check out the documentation"
    };
    steps_.push_back(started);
}

void OnboardingScreen::show() {
    visible_ = true;
    currentStep_ = 0;
}

void OnboardingScreen::hide() {
    visible_ = false;
}

void OnboardingScreen::close() {
    hide();
    markOnboardingComplete();
    
    if (onCompleted) {
        onCompleted();
    }
}

void OnboardingScreen::nextStep() {
    if (currentStep_ < static_cast<int>(steps_.size()) - 1) {
        currentStep_++;
    } else {
        close();
    }
}

void OnboardingScreen::previousStep() {
    if (currentStep_ > 0) {
        currentStep_--;
    }
}

void OnboardingScreen::skipOnboarding() {
    markOnboardingComplete();
    hide();
    
    if (onSkipped) {
        onSkipped();
    }
}

void OnboardingScreen::completeOnboarding() {
    close();
}

void OnboardingScreen::render(RenderEngine& renderer) {
    if (!visible_ || steps_.empty() ||
        currentStep_ < 0 || currentStep_ >= static_cast<int>(steps_.size())) {
        return;
    }

    const Rect& bounds = getBounds();

    // Draw background
    renderer.drawGradientRect(bounds,
                             Color(0.1f, 0.1f, 0.14f),
                             Color(0.08f, 0.08f, 0.1f));

    renderStep(renderer, steps_[currentStep_]);

    // Draw progress indicator
    renderProgress(renderer);

    // Draw navigation buttons
    renderButtons(renderer);
}

void OnboardingScreen::renderStep(RenderEngine& renderer, const OnboardingStep& step) {
    const Rect& bounds = getBounds();

    // Draw icon
    float iconSize = 80;
    float iconX = bounds.x + (bounds.width - iconSize) / 2;
    float iconY = bounds.y + 40;
    renderer.drawText(step.icon, iconX, iconY + iconSize - 10, Color(1, 1, 1), "default", 60);

    // Draw title
    float titleY = iconY + iconSize + 20;
    Vec2 titleSize = renderer.measureText(step.title, "default", 28);
    renderer.drawText(step.title, bounds.x + (bounds.width - titleSize.x) / 2, titleY,
                     Color(1, 1, 1), "default", 28);

    // Draw description
    float descY = titleY + 40;
    Vec2 descSize = renderer.measureText(step.description, "default", 16);
    renderer.drawText(step.description, bounds.x + (bounds.width - descSize.x) / 2, descY,
                     Color(0.7f, 0.7f, 0.75f), "default", 16);

    // Draw features list
    float featuresY = descY + 40;
    float featureX = bounds.x + 100;

    for (const auto& feature : step.features) {
        renderer.drawText("•", featureX, featuresY, Color(0.4f, 0.6f, 1.0f), "default", 16);
        renderer.drawText(feature, featureX + 20, featuresY, Color(0.8f, 0.8f, 0.85f), "default", 14);
        featuresY += 28;
    }
}

void OnboardingScreen::renderProgress(RenderEngine& renderer) {
    if (steps_.empty()) return;
    const Rect& bounds = getBounds();
    float progressY = bounds.y + bounds.height - 120;

    // Draw step indicators
    float totalWidth = static_cast<float>(steps_.size()) * 12 + static_cast<float>(steps_.size() - 1) * 6;
    float startX = bounds.x + (bounds.width - totalWidth) / 2;
    
    for (size_t i = 0; i < steps_.size(); ++i) {
        float x = startX + i * 18;
        Color dotColor = (static_cast<int>(i) <= currentStep_) ? 
                        Color(0.4f, 0.6f, 1.0f) : Color(0.3f, 0.3f, 0.35f);
        
        renderer.drawCircle(x + 6, progressY + 6, 6, dotColor, true);
    }
    
    // Draw step counter
    std::string stepText = "Step " + std::to_string(currentStep_ + 1) + " of " + 
                          std::to_string(steps_.size());
    Vec2 stepSize = renderer.measureText(stepText, "default", 12);
    renderer.drawText(stepText, bounds.x + (bounds.width - stepSize.x) / 2, 
                     progressY + 20, Color(0.5f, 0.5f, 0.55f), "default", 12);
}

void OnboardingScreen::renderButtons(RenderEngine& renderer) {
    const Rect& bounds = getBounds();
    float buttonY = bounds.y + bounds.height - 80;
    
    // Skip button
    float skipWidth = 100;
    float skipX = bounds.x + 40;
    
    if (skipButtonHovered_) {
        renderer.drawRoundedRect(Rect(skipX, buttonY, skipWidth, 36), Color(0.2f, 0.2f, 0.25f), 6);
    }
    renderer.drawText("Skip", skipX + (skipWidth - 40) / 2, buttonY + 22,
                     Color(0.7f, 0.7f, 0.75f), "default", 14);
    
    // Previous button
    if (currentStep_ > 0) {
        float prevWidth = 100;
        float prevX = bounds.x + 160;
        
        if (prevButtonHovered_) {
            renderer.drawRoundedRect(Rect(prevX, buttonY, prevWidth, 36), Color(0.2f, 0.2f, 0.25f), 6);
        }
        renderer.drawText("Back", prevX + (prevWidth - 40) / 2, buttonY + 22,
                         Color(0.9f, 0.9f, 0.9f), "default", 14);
    }
    
    // Next/Complete button
    float nextWidth = 120;
    float nextX = bounds.x + bounds.width - nextWidth - 40;
    std::string nextText = (currentStep_ == static_cast<int>(steps_.size()) - 1) ? 
                          "Get Started" : "Next";
    
    renderer.drawRoundedRect(Rect(nextX, buttonY, nextWidth, 36), Color(0.25f, 0.4f, 0.7f), 6);
    renderer.drawText(nextText, nextX + (nextWidth - renderer.measureText(nextText, "default", 14).x) / 2, 
                     buttonY + 22, Color(1, 1, 1), "default", 14);
}

bool OnboardingScreen::handleEvent(Core::Event& event) {
    return false;
}

bool OnboardingScreen::onMouseDown(const Core::MouseButtonPressEvent& event) {
    const Rect& bounds = getBounds();
    float buttonY = bounds.y + bounds.height - 80;
    
    // Check skip button
    if (event.x >= bounds.x + 40 && event.x < bounds.x + 140 &&
        event.y >= buttonY && event.y < buttonY + 36) {
        skipOnboarding();
        return true;
    }
    
    // Check previous button
    if (currentStep_ > 0) {
        if (event.x >= bounds.x + 160 && event.x < bounds.x + 260 &&
            event.y >= buttonY && event.y < buttonY + 36) {
            previousStep();
            return true;
        }
    }
    
    // Check next button
    float nextWidth = 120;
    float nextX = bounds.x + bounds.width - nextWidth - 40;
    
    if (event.x >= nextX && event.x < nextX + nextWidth &&
        event.y >= buttonY && event.y < buttonY + 36) {
        nextStep();
        return true;
    }
    
    return false;
}

bool OnboardingScreen::isFirstRun() const {
    if (!settings_) return true;
    
    // Check if onboarding was completed
    auto value = settings_->getValue("onboarding.completed");
    if (value.type == Config::SettingsValue::Type::Bool) {
        return !value.boolValue;
    }
    return true;
}

void OnboardingScreen::markOnboardingComplete() {
    if (settings_) {
        settings_->setValue("onboarding.completed", Config::SettingsValue::fromBool(true));
    }
}

} // namespace NexusForge::UI
