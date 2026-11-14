// ui/screens/onboarding_screen.hpp
// Onboarding Screen Header

#pragma once

#include "../framework/widget_system.hpp"
#include "../../core/engine/nexus_core.hpp"
#include "../../config/settings_manager.hpp"

#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace NexusForge::UI {

struct OnboardingStep {
    std::string title;
    std::string description;
    std::string icon;
    std::vector<std::string> features;
    std::function<void()> action;
};

class OnboardingScreen : public Widget {
public:
    using Ptr = std::shared_ptr<OnboardingScreen>;
    
    OnboardingScreen();
    ~OnboardingScreen() override;
    
    bool initialize(Core::NexusEngine* engine, Config::SettingsManager* settings);
    void show();
    void hide();
    void close();
    
    void nextStep();
    void previousStep();
    void skipOnboarding();
    void completeOnboarding();
    
    void render(RenderEngine& renderer) override;
    bool handleEvent(Core::Event& event) override;
    bool onMouseDown(const Core::MouseButtonPressEvent& event) override;
    
    std::function<void()> onCompleted;
    std::function<void()> onSkipped;
    
private:
    Core::NexusEngine* engine_ = nullptr;
    Config::SettingsManager* settings_ = nullptr;
    
    std::vector<OnboardingStep> steps_;
    int currentStep_ = 0;
    bool completed_ = false;
    
    bool skipButtonHovered_ = false;
    bool prevButtonHovered_ = false;
    
    void createSteps();
    void renderStep(RenderEngine& renderer, const OnboardingStep& step);
    void renderProgress(RenderEngine& renderer);
    void renderButtons(RenderEngine& renderer);
    
    bool isFirstRun() const;
    void markOnboardingComplete();
};

} // namespace NexusForge::UI
