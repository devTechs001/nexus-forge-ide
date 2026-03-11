// ui/screens/main_window.cpp
#include "main_window.hpp"
#include "../../core/platform/platform_abstraction.hpp"

namespace NexusForge::UI {

MainWindow::MainWindow() = default;

MainWindow::~MainWindow() {
    close();
}

bool MainWindow::initialize(Core::NexusEngine* engine, Config::SettingsManager* settings) {
    engine_ = engine;
    settings_ = settings;

    // Initialize renderer
    renderer_ = std::make_unique<RenderEngine>();
    if (!renderer_->initialize(RenderAPI::Auto)) {
        return false;
    }

    // Initialize gesture handler for mobile
    gestureHandler_ = std::make_unique<Mobile::GestureHandler>();

    // Create native window
    if (!Platform::PlatformAbstraction::createMainWindow(
            windowWidth_, windowHeight_, &nativeWindow_)) {
        return false;
    }

    // Setup UI components
    createTitleBar();
    createActivityBar();
    createSidebar();
    createEditorArea();
    createBottomPanel();
    createStatusBar();
    createOverlays();

    // Check if mobile layout needed
    updateLayout();

    // Setup keyboard shortcuts
    setupKeyboardShortcuts();

    return true;
}

void MainWindow::show() {
    if (nativeWindow_) {
        Platform::PlatformAbstraction::showWindow(nativeWindow_);
    }
}

void MainWindow::close() {
    if (nativeWindow_) {
        Platform::PlatformAbstraction::destroyWindow(nativeWindow_);
        nativeWindow_ = nullptr;
    }

    renderer_.reset();
    gestureHandler_.reset();
}

void MainWindow::setTitle(const std::string& title) {
    if (nativeWindow_) {
        Platform::PlatformAbstraction::setWindowTitle(nativeWindow_, title.c_str());
    }
}

void MainWindow::setSize(int width, int height) {
    windowWidth_ = width;
    windowHeight_ = height;
    if (nativeWindow_) {
        Platform::PlatformAbstraction::setWindowSize(nativeWindow_, width, height);
    }
}

void MainWindow::setPosition(int x, int y) {
    if (nativeWindow_) {
        Platform::PlatformAbstraction::setWindowPosition(nativeWindow_, x, y);
    }
}

void MainWindow::maximize() {
    if (nativeWindow_) {
        Platform::PlatformAbstraction::maximizeWindow(nativeWindow_);
        maximized_ = true;
    }
}

void MainWindow::minimize() {
    if (nativeWindow_) {
        Platform::PlatformAbstraction::minimizeWindow(nativeWindow_);
    }
}

void MainWindow::restore() {
    if (nativeWindow_) {
        Platform::PlatformAbstraction::restoreWindow(nativeWindow_);
        maximized_ = false;
        fullscreen_ = false;
    }
}

void MainWindow::toggleFullscreen() {
    fullscreen_ = !fullscreen_;
    if (nativeWindow_) {
        Platform::PlatformAbstraction::setWindowFullscreen(nativeWindow_, fullscreen_);
    }
}

bool MainWindow::isMaximized() const {
    return maximized_;
}

bool MainWindow::isFullscreen() const {
    return fullscreen_;
}

void MainWindow::setLayoutMode(Mobile::FormFactor formFactor) {
    if (currentFormFactor_ != formFactor) {
        currentFormFactor_ = formFactor;
        updateLayout();
    }
}

void MainWindow::setSidebarVisible(bool visible) {
    sidebarVisible_ = visible;
    if (sidebar_) {
        sidebar_->setVisible(visible);
    }
}

void MainWindow::setPanelVisible(bool visible) {
    panelVisible_ = visible;
    if (bottomPanel_) {
        bottomPanel_->setVisible(visible);
    }
}

void MainWindow::setSidebarWidth(float width) {
    sidebarWidth_ = width;
    if (sidebar_) {
        sidebar_->setSize(width, sidebar_->getSize().y);
    }
}

void MainWindow::setPanelHeight(float height) {
    panelHeight_ = height;
    if (bottomPanel_) {
        bottomPanel_->setSize(bottomPanel_->getSize().x, height);
    }
}

void MainWindow::setActivityBarItems(const std::vector<ActivityBarItem>& items) {
    activityItems_ = items;
    // Update activity bar UI
}

void MainWindow::setActiveActivityItem(const std::string& id) {
    activeActivityId_ = id;
    for (auto& item : activityItems_) {
        item.active = (item.id == id);
    }
}

void MainWindow::updateActivityBadge(const std::string& id, int count) {
    for (auto& item : activityItems_) {
        if (item.id == id) {
            item.badgeCount = count;
            break;
        }
    }
}

void MainWindow::showPanel(PanelType type) {
    activePanelType_ = type;
    sidebarVisible_ = true;
    // Update sidebar content based on panel type
}

void MainWindow::hidePanel() {
    sidebarVisible_ = false;
}

void MainWindow::openFile(const std::string& path) {
    // Create new tab or activate existing
    std::string tabId = generateTabId();

    TabInfo tab;
    tab.id = tabId;
    tab.filePath = path;
    tab.title = path.substr(path.find_last_of("/\\") + 1);

    // Determine language from extension
    std::string ext = path.substr(path.find_last_of('.') + 1);
    tab.language = ext;

    tabs_.push_back(tab);
    activeTabId_ = tabId;

    if (onFileOpened) {
        onFileOpened(path);
    }

    updateWindowTitle();
}

void MainWindow::closeTab(const std::string& id) {
    auto it = std::find_if(tabs_.begin(), tabs_.end(),
        [&id](const TabInfo& tab) { return tab.id == id; });

    if (it != tabs_.end()) {
        std::string filePath = it->filePath;
        tabs_.erase(it);

        if (activeTabId_ == id) {
            activeTabId_ = tabs_.empty() ? "" : tabs_.back().id;
        }

        if (onFileClosed) {
            onFileClosed(filePath);
        }
    }
}

void MainWindow::setActiveTab(const std::string& id) {
    activeTabId_ = id;
    if (onActiveTabChanged) {
        onActiveTabChanged(id);
    }
}

void MainWindow::nextTab() {
    if (tabs_.empty()) return;

    auto it = std::find_if(tabs_.begin(), tabs_.end(),
        [this](const TabInfo& tab) { return tab.id == activeTabId_; });

    size_t index = (it - tabs_.begin() + 1) % tabs_.size();
    activeTabId_ = tabs_[index].id;
}

void MainWindow::previousTab() {
    if (tabs_.empty()) return;

    auto it = std::find_if(tabs_.begin(), tabs_.end(),
        [this](const TabInfo& tab) { return tab.id == activeTabId_; });

    size_t index = (it - tabs_.begin() + tabs_.size() - 1) % tabs_.size();
    activeTabId_ = tabs_[index].id;
}

void MainWindow::showTerminal() {
    showPanel(PanelType::Terminal);
    panelVisible_ = true;
}

void MainWindow::showProblems() {
    showPanel(PanelType::Problems);
    panelVisible_ = true;
}

void MainWindow::showOutput() {
    showPanel(PanelType::Output);
    panelVisible_ = true;
}

void MainWindow::hideBottomPanel() {
    panelVisible_ = false;
}

void MainWindow::showCommandPalette() {
    if (commandPalette_) {
        commandPalette_->setVisible(true);
        commandPalette_->requestFocus();
    }
}

void MainWindow::showQuickOpen() {
    if (quickOpen_) {
        quickOpen_->setVisible(true);
        quickOpen_->requestFocus();
    }
}

void MainWindow::showNotification(const std::string& message,
                                   const std::string& type, int durationMs) {
    // Create and show notification toast
}

void MainWindow::update(double deltaTime) {
    if (gestureHandler_) {
        gestureHandler_->update(deltaTime);
    }

    // Update animations
    if (rootWidget_) {
        rootWidget_->update(deltaTime);
    }
}

void MainWindow::render() {
    if (!renderer_ || !nativeWindow_) return;

    renderer_->beginFrame();
    renderer_->setViewport(0, 0, windowWidth_, windowHeight_);
    renderer_->setProjection(Mat4::ortho(0, windowWidth_, windowHeight_, 0, -1, 1));

    // Clear background
    renderer_->clear(Color(0.1f, 0.1f, 0.12f, 1.0f));

    // Render UI components
    if (rootWidget_) {
        rootWidget_->render(*renderer_);
    }

    renderer_->endFrame();
    renderer_->present();
}

bool MainWindow::handleEvent(Core::Event& event) {
    // Handle window events
    if (auto* resizeEvent = dynamic_cast<Core::WindowResizeEvent*>(&event)) {
        windowWidth_ = resizeEvent->width;
        windowHeight_ = resizeEvent->height;
        if (onResize) {
            onResize(windowWidth_, windowHeight_);
        }
        updateLayout();
        return true;
    }

    if (auto* closeEvent = dynamic_cast<Core::WindowCloseEvent*>(&event)) {
        if (onClose) {
            onClose();
        }
        return true;
    }

    // Handle keyboard shortcuts
    if (auto* keyEvent = dynamic_cast<Core::KeyPressEvent*>(&event)) {
        if (handleKeyboardShortcut(*keyEvent)) {
            return true;
        }
    }

    // Pass to widgets
    if (rootWidget_) {
        return rootWidget_->handleEvent(event);
    }

    return false;
}

TabInfo* MainWindow::getActiveTab() {
    auto it = std::find_if(tabs_.begin(), tabs_.end(),
        [this](const TabInfo& tab) { return tab.id == activeTabId_; });
    return (it != tabs_.end()) ? &(*it) : nullptr;
}

CodeEditor* MainWindow::getActiveEditor() {
    // Return editor widget from active tab
    return nullptr;  // Implementation depends on tab content
}

std::string MainWindow::generateTabId() {
    static int counter = 0;
    return "tab_" + std::to_string(++counter);
}

void MainWindow::updateWindowTitle() {
    std::string title = "NexusForge IDE";

    TabInfo* active = getActiveTab();
    if (active) {
        title = active->title + " - " + title;
        if (active->modified) {
            title = "* " + title;
        }
    }

    setTitle(title);
}

void MainWindow::createTitleBar() {
    // Create title bar widget
}

void MainWindow::createActivityBar() {
    // Create activity bar widget
}

void MainWindow::createSidebar() {
    // Create sidebar widget
}

void MainWindow::createEditorArea() {
    // Create editor area widget
}

void MainWindow::createBottomPanel() {
    // Create bottom panel widget
}

void MainWindow::createStatusBar() {
    // Create status bar widget
}

void MainWindow::createMobileLayout() {
    // Create mobile-specific layout
}

void MainWindow::createOverlays() {
    // Create command palette, quick open, notifications
}

void MainWindow::layoutDesktop() {
    // Desktop layout
}

void MainWindow::layoutTablet() {
    // Tablet layout
}

void MainWindow::layoutPhone() {
    // Phone layout
}

void MainWindow::updateLayout() {
    switch (currentFormFactor_) {
        case Mobile::FormFactor::Phone:
            layoutPhone();
            break;
        case Mobile::FormFactor::Tablet:
            layoutTablet();
            break;
        default:
            layoutDesktop();
            break;
    }
}

void MainWindow::setupKeyboardShortcuts() {
    // Setup default keyboard shortcuts
}

bool MainWindow::handleKeyboardShortcut(const Core::KeyPressEvent& event) {
    // Handle Ctrl/Cmd based shortcuts
    bool ctrl = (event.modifiers & 0x0002) != 0;  // Ctrl mask
    bool shift = (event.modifiers & 0x0001) != 0;  // Shift mask

    if (ctrl) {
        switch (event.keyCode) {
            case 'N':  // New file
                return true;
            case 'O':  // Open file
                showQuickOpen();
                return true;
            case 'S':  // Save
                return true;
            case 'P':  // Command palette
                showCommandPalette();
                return true;
            case '`':  // Toggle terminal
                if (panelVisible_) {
                    hideBottomPanel();
                } else {
                    showTerminal();
                }
                return true;
            case 'B':  // Toggle sidebar
                setSidebarVisible(!sidebarVisible_);
                return true;
        }
    }

    if (ctrl && shift) {
        switch (event.keyCode) {
            case 'P':  // Go to symbol
                return true;
        }
    }

    return false;
}

} // namespace NexusForge::UI
