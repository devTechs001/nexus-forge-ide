// ui/framework/ui_core.cpp
#include "ui_core.hpp"

namespace NexusForge::UI {

UICore& UICore::getInstance() {
    static UICore instance;
    return instance;
}

bool UICore::initialize(void* windowHandle, int width, int height) {
    context_.width = static_cast<float>(width);
    context_.height = static_cast<float>(height);
    
    context_.renderEngine = std::make_unique<RenderEngine>();
    if (!context_.renderEngine->initialize()) {
        return false;
    }
    
    context_.layoutEngine = std::make_unique<LayoutEngine>();
    context_.themeManager = std::make_unique<ThemeManager>();
    
    context_.renderEngine->setViewport(0, 0, width, height);
    context_.renderEngine->setProjection(Mat4::ortho(0, static_cast<float>(width), 
                                                      static_cast<float>(height), 0, -1, 1));
    
    return true;
}

void UICore::shutdown() {
    context_.focusedWidget = nullptr;
    context_.hoveredWidget = nullptr;
    context_.captureWidget = nullptr;
    context_.rootWidget = nullptr;
    rootWidget_.reset();
    if (context_.renderEngine) {
        context_.renderEngine->shutdown();
        context_.renderEngine.reset();
    }
    context_.layoutEngine.reset();
    context_.themeManager.reset();
}

void UICore::render() {
    if (!context_.renderEngine) return;

    context_.renderEngine->beginFrame();
    context_.renderEngine->clear(Color::fromHex("#1E1E2E"));

    if (rootWidget_) {
        rootWidget_->render(*context_.renderEngine);
    }

    context_.renderEngine->endFrame();
    context_.renderEngine->present();
}

void UICore::update(double deltaTime) {
    if (rootWidget_) {
        rootWidget_->update(deltaTime);
    }

    if (layoutDirty_) {
        performLayout();
    }
}

void UICore::handleEvent(Core::Event& event) {
    if (!rootWidget_) return;

    // Dispatch to widget hierarchy first
    bool handled = rootWidget_->handleEvent(event);

    // Fallback to focused widget only if not already handled and distinct from root
    if (!handled && !event.handled && context_.focusedWidget &&
        context_.focusedWidget != rootWidget_.get()) {
        context_.focusedWidget->handleEvent(event);
    }
}

void UICore::setRootWidget(Widget::Ptr root) {
    context_.focusedWidget = nullptr;
    context_.hoveredWidget = nullptr;
    context_.captureWidget = nullptr;
    rootWidget_ = std::move(root);
    context_.rootWidget = rootWidget_.get();
    invalidateLayout();
}

void UICore::setTheme(const std::string& themeId) {
    if (context_.themeManager) {
        context_.themeManager->setTheme(themeId);
    }
}

const Theme* UICore::getCurrentTheme() const {
    if (context_.themeManager) {
        return context_.themeManager->getCurrentTheme();
    }
    return nullptr;
}

void UICore::invalidateLayout() {
    layoutDirty_ = true;
}

void UICore::performLayout() {
    if (rootWidget_ && context_.layoutEngine) {
        context_.layoutEngine->layout(rootWidget_.get(), context_.width, context_.height);
    }
    layoutDirty_ = false;
}

void UICore::setFocusedWidget(Widget::Ptr widget) {
    if (context_.focusedWidget && context_.focusedWidget != widget.get()) {
        context_.focusedWidget->onFocusLost();
    }
    context_.focusedWidget = widget.get();
    if (context_.focusedWidget) {
        context_.focusedWidget->onFocusGained();
    }
}

namespace {
Widget::Ptr hitTestRecursive(const Widget::Ptr& widget, float x, float y) {
    if (!widget || !widget->isVisible()) return nullptr;
    if (!widget->getBounds().contains(x, y)) return nullptr;

    const auto& children = widget->getChildren();
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        if (auto hit = hitTestRecursive(*it, x, y)) {
            return hit;
        }
    }
    return widget;
}
} // namespace

Widget::Ptr UICore::hitTest(float x, float y) {
    return hitTestRecursive(rootWidget_, x, y);
}

void UICore::setDPIScale(float scale) {
    context_.dpiScale = scale;
    invalidateLayout();
}

void UICore::setSize(float width, float height) {
    context_.width = width;
    context_.height = height;
    
    if (context_.renderEngine) {
        context_.renderEngine->setViewport(0, 0, 
                                            static_cast<int>(width), 
                                            static_cast<int>(height));
        context_.renderEngine->setProjection(Mat4::ortho(0, width, height, 0, -1, 1));
    }
    
    invalidateLayout();
}

} // namespace NexusForge::UI
