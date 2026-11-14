// ui/screens/main_window.cpp
#include "main_window.hpp"
#include "../renderer/software_backend.hpp"
#include "../../core/platform/platform_abstraction.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <sstream>
#include <unordered_set>

namespace NexusForge::UI {

namespace {

// Phoenix Dark Theme Palette
const Color COL_TITLEBAR      (0.067f, 0.078f, 0.118f, 1.0f); // #11141E
const Color COL_ACTIVITYBAR   (0.059f, 0.071f, 0.106f, 1.0f); // #0F121B
const Color COL_SIDEBAR       (0.078f, 0.094f, 0.141f, 1.0f); // #141824
const Color COL_EDITOR_BG     (0.094f, 0.110f, 0.161f, 1.0f); // #181C29
const Color COL_TABBAR_BG     (0.071f, 0.082f, 0.125f, 1.0f); // #121520
const Color COL_PANEL_BG      (0.075f, 0.088f, 0.133f, 1.0f); // #131622
const Color COL_SURFACE       (0.118f, 0.141f, 0.208f, 1.0f); // #1E2435
const Color COL_SURFACE_HOVER (0.153f, 0.184f, 0.271f, 1.0f); // #272F45
const Color COL_BORDER        (0.165f, 0.200f, 0.295f, 1.0f); // #2A334B
const Color COL_ACCENT        (0.290f, 0.545f, 1.000f, 1.0f); // #4A8BFF
const Color COL_ACCENT_VIOLET (0.560f, 0.400f, 0.980f, 1.0f); // #8F66FA
const Color COL_STATUSBAR     (0.125f, 0.310f, 0.720f, 1.0f); // #204FB8
const Color COL_TEXT_PRIMARY  (0.910f, 0.935f, 0.985f, 1.0f);
const Color COL_TEXT_MUTED    (0.560f, 0.620f, 0.750f, 1.0f);
const Color COL_TEXT_DIM      (0.390f, 0.440f, 0.560f, 1.0f);
const Color COL_GREEN         (0.320f, 0.820f, 0.560f, 1.0f);
const Color COL_AMBER         (0.960f, 0.730f, 0.300f, 1.0f);
const Color COL_CYAN          (0.320f, 0.800f, 0.940f, 1.0f);
const Color COL_CORAL         (0.960f, 0.450f, 0.480f, 1.0f);

std::vector<std::string> splitLines(const std::string& content) {
    std::vector<std::string> out;
    std::string cur;
    for (char ch : content) {
        if (ch == '\r') continue;
        if (ch == '\n') {
            out.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(ch);
        }
    }
    out.push_back(cur);
    if (out.empty()) out.push_back("");
    return out;
}

std::string detectLanguage(const std::string& filename) {
    auto endsWith = [&](const char* suffix) {
        size_t slen = std::char_traits<char>::length(suffix);
        return filename.size() >= slen &&
               filename.compare(filename.size() - slen, slen, suffix) == 0;
    };
    if (endsWith(".cpp") || endsWith(".hpp") || endsWith(".cc") || endsWith(".h") || endsWith(".c")) return "C++";
    if (endsWith(".py")) return "Python";
    if (endsWith(".sh") || endsWith(".bash")) return "Shell";
    if (endsWith(".json")) return "JSON";
    if (endsWith(".md")) return "Markdown";
    if (endsWith(".js") || endsWith(".ts")) return "TypeScript";
    if (filename == "CMakeLists.txt" || endsWith(".cmake")) return "CMake";
    return "Plain Text";
}

Color fileBadgeColor(const std::string& name, bool isDir) {
    if (isDir) return Color(0.94f, 0.74f, 0.34f, 1.0f);
    std::string lang = detectLanguage(name);
    if (lang == "C++")        return Color(0.36f, 0.65f, 1.00f, 1.0f);
    if (lang == "Shell")      return Color(0.35f, 0.85f, 0.56f, 1.0f);
    if (lang == "Python")     return Color(0.96f, 0.78f, 0.30f, 1.0f);
    if (lang == "CMake")      return Color(0.92f, 0.44f, 0.52f, 1.0f);
    if (lang == "Markdown")   return Color(0.66f, 0.54f, 0.98f, 1.0f);
    if (lang == "JSON")       return Color(0.95f, 0.62f, 0.30f, 1.0f);
    return Color(0.62f, 0.68f, 0.80f, 1.0f);
}

std::string fileBadgeTag(const std::string& name, bool isDir) {
    if (isDir) return "D";
    std::string lang = detectLanguage(name);
    if (lang == "C++")      return "C";
    if (lang == "Shell")    return "$";
    if (lang == "Python")   return "P";
    if (lang == "CMake")    return "B";
    if (lang == "Markdown") return "M";
    if (lang == "JSON")     return "{";
    return "F";
}

bool isCppKeyword(const std::string& word) {
    static const std::unordered_set<std::string> kws = {
        "alignas", "auto", "bool", "break", "case", "catch", "char", "class",
        "const", "constexpr", "continue", "default", "delete", "do", "double",
        "else", "enum", "explicit", "extern", "false", "float", "for", "friend",
        "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept",
        "nullptr", "operator", "override", "private", "protected", "public",
        "return", "short", "signed", "sizeof", "static", "struct", "switch",
        "template", "this", "throw", "true", "try", "typedef", "typename",
        "unsigned", "using", "virtual", "void", "volatile", "while",
        // Shell / Python / CMake keywords
        "def", "import", "from", "elif", "None", "True", "False", "function",
        "local", "then", "fi", "esac", "done", "set", "project", "target_link_libraries"
    };
    return kws.count(word) > 0;
}

bool isCppType(const std::string& word) {
    static const std::unordered_set<std::string> types = {
        "std", "string", "vector", "unique_ptr", "shared_ptr", "size_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t", "int32_t", "int64_t",
        "Rect", "Color", "Vec2", "Mat4", "MainWindow", "SplashScreen",
        "NexusEngine", "RenderEngine", "PlatformAbstraction", "TabInfo"
    };
    return types.count(word) > 0;
}

} // anonymous namespace

MainWindow::MainWindow() = default;

MainWindow::~MainWindow() {
    close();
}

bool MainWindow::initialize(Core::NexusEngine* engine, Config::SettingsManager* settings) {
    engine_ = engine;
    settings_ = settings;

    // Create native window first so software backend binds directly to it
    nativeWindow_ = Platform::PlatformAbstraction::createMainWindow("NexusForge IDE", windowWidth_, windowHeight_);
    if (!nativeWindow_) {
        return false;
    }

    // Initialize renderer with software backend targeting the native window
    renderer_ = std::make_unique<RenderEngine>();
    auto backend = std::make_unique<SoftwareRenderBackend>(nativeWindow_);
    renderer_->setBackend(std::move(backend));
    if (!renderer_->initialize(RenderAPI::Auto)) {
        return false;
    }

    // Initialize gesture handler for mobile
    gestureHandler_ = std::make_unique<Mobile::GestureHandler>();

    // Setup UI components & workspace metadata
    workspaceRoot_ = Platform::PlatformAbstraction::getCurrentWorkingDirectory();
    size_t slash = workspaceRoot_.find_last_of("/\\");
    workspaceName_ = (slash != std::string::npos && slash + 1 < workspaceRoot_.size())
                         ? workspaceRoot_.substr(slash + 1)
                         : workspaceRoot_;
    if (workspaceName_.empty()) workspaceName_ = "workspace";

    detectGitBranch();
    loadWorkspaceTree();

    createTitleBar();
    createActivityBar();
    createSidebar();
    createEditorArea();
    createBottomPanel();
    createStatusBar();
    createOverlays();
    initPaletteCommands();

    // Open Welcome tab by default, plus main.cpp if present so the IDE immediately feels rich
    openWelcomeTab();
    if (Platform::PlatformAbstraction::fileExists("main.cpp")) {
        openFile("main.cpp");
        // Keep Welcome active first so the user sees the Welcome IDE dashboard,
        // with main.cpp one click away in the tab bar!
        if (!tabs_.empty()) {
            setActiveTab(tabs_.front().id);
        }
    }

    updateLayout();
    setupKeyboardShortcuts();
    return true;
}

void MainWindow::show() {
    if (nativeWindow_) {
        Platform::PlatformAbstraction::showWindow(nativeWindow_);
        render();
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
    windowWidth_ = std::max(320, width);
    windowHeight_ = std::max(240, height);
    if (nativeWindow_) {
        Platform::PlatformAbstraction::setWindowSize(nativeWindow_, windowWidth_, windowHeight_);
    }
    updateLayout();
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

bool MainWindow::isMaximized() const { return maximized_; }
bool MainWindow::isFullscreen() const { return fullscreen_; }

void MainWindow::setLayoutMode(Mobile::FormFactor formFactor) {
    if (currentFormFactor_ != formFactor) {
        currentFormFactor_ = formFactor;
        updateLayout();
    }
}

void MainWindow::setSidebarVisible(bool visible) {
    sidebarVisible_ = visible;
    if (sidebar_) sidebar_->setVisible(visible);
    updateLayout();
}

void MainWindow::setPanelVisible(bool visible) {
    panelVisible_ = visible;
    if (bottomPanel_) bottomPanel_->setVisible(visible);
    updateLayout();
}

void MainWindow::setSidebarWidth(float width) {
    sidebarWidth_ = std::clamp(width, 160.0f, 480.0f);
    if (sidebar_) sidebar_->setSize(sidebarWidth_, sidebar_->getSize().y);
    updateLayout();
}

void MainWindow::setPanelHeight(float height) {
    panelHeight_ = std::clamp(height, 100.0f, 420.0f);
    if (bottomPanel_) bottomPanel_->setSize(bottomPanel_->getSize().x, panelHeight_);
    updateLayout();
}

void MainWindow::setActivityBarItems(const std::vector<ActivityBarItem>& items) {
    activityItems_ = items;
}

void MainWindow::setActiveActivityItem(const std::string& id) {
    activeActivityId_ = id;
    for (auto& item : activityItems_) {
        item.active = (item.id == id);
        if (item.active) {
            activePanelType_ = item.panelType;
        }
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
    if (type == PanelType::Terminal) {
        activeBottomTab_ = 1;
        panelVisible_ = true;
        updateLayout();
        return;
    }
    if (type == PanelType::Output) {
        activeBottomTab_ = 0;
        panelVisible_ = true;
        updateLayout();
        return;
    }
    if (type == PanelType::Problems) {
        activeBottomTab_ = 2;
        panelVisible_ = true;
        updateLayout();
        return;
    }

    activePanelType_ = type;
    sidebarVisible_ = true;
    for (auto& item : activityItems_) {
        item.active = (item.panelType == type);
        if (item.active) activeActivityId_ = item.id;
    }
    updateLayout();
}

void MainWindow::hidePanel() {
    sidebarVisible_ = false;
    updateLayout();
}

void MainWindow::openWelcomeTab() {
    for (const auto& existing : tabs_) {
        if (existing.isWelcome) {
            setActiveTab(existing.id);
            return;
        }
    }
    TabInfo tab;
    tab.id = generateTabId();
    tab.title = "Welcome";
    tab.language = "Welcome";
    tab.isWelcome = true;
    tabs_.insert(tabs_.begin(), std::move(tab));
    activeTabId_ = tabs_.front().id;
    updateWindowTitle();
}

void MainWindow::addOutputLog(const std::string& line) {
    outputLogs_.push_back(line);
    if (outputLogs_.size() > 200) {
        outputLogs_.erase(outputLogs_.begin());
    }
}

void MainWindow::saveActiveFile() {
    TabInfo* tab = getActiveTab();
    if (!tab || tab->isWelcome) return;
    if (tab->filePath.empty()) {
        showNotification("Untitled file has no path yet", "info", 3000);
        return;
    }
    std::string content;
    for (size_t i = 0; i < tab->lines.size(); ++i) {
        content += tab->lines[i];
        if (i + 1 < tab->lines.size()) content.push_back('\n');
    }
    if (Platform::PlatformAbstraction::writeFile(tab->filePath, content)) {
        tab->modified = false;
        updateWindowTitle();
        showNotification("Saved " + tab->title, "info", 3000);
        addOutputLog("[save] Saved " + tab->filePath);
        if (onFileSaved) onFileSaved(tab->filePath);
    } else {
        showNotification("Failed to save " + tab->title, "error", 4000);
    }
}

bool MainWindow::saveFramePPM(const std::string& path) const {
    if (!renderer_) return false;
    auto* sw = dynamic_cast<SoftwareRenderBackend*>(renderer_->getBackend());
    return sw ? sw->savePPM(path) : false;
}

void MainWindow::openFile(const std::string& path) {
    for (const auto& existing : tabs_) {
        if (!path.empty() && existing.filePath == path) {
            setActiveTab(existing.id);
            updateWindowTitle();
            return;
        }
    }

    std::string tabId = generateTabId();
    TabInfo tab;
    tab.id = tabId;
    tab.filePath = path;
    size_t sepPos = path.find_last_of("/\\");
    tab.title = (sepPos != std::string::npos) ? path.substr(sepPos + 1) : path;
    if (tab.title.empty()) tab.title = "Untitled";

    tab.language = detectLanguage(tab.title);
    if (!path.empty() && Platform::PlatformAbstraction::fileExists(path)) {
        std::string text = Platform::PlatformAbstraction::readFile(path);
        tab.lines = splitLines(text);
    } else {
        tab.lines = {""};
    }

    tabs_.push_back(std::move(tab));
    activeTabId_ = tabId;

    if (onFileOpened) {
        onFileOpened(path);
    }
    addOutputLog("[editor] Opened " + (path.empty() ? "Untitled" : path));
    updateWindowTitle();
}

void MainWindow::openFiles(const std::vector<std::string>& paths) {
    for (const auto& path : paths) {
        openFile(path);
    }
}

void MainWindow::newFile(const std::string& language) {
    std::string tabId = generateTabId();
    TabInfo tab;
    tab.id = tabId;
    tab.title = "Untitled-" + std::to_string(tabs_.size() + 1);
    tab.language = language.empty() ? "C++" : language;
    tab.lines = {
        "// New file created in NexusForge IDE",
        "#include <iostream>",
        "",
        "int main() {",
        "    std::cout << \"Hello from NexusForge IDE!\" << std::endl;",
        "    return 0;",
        "}",
        ""
    };
    tab.modified = true;
    tabs_.push_back(std::move(tab));
    activeTabId_ = tabId;
    if (onActiveTabChanged) onActiveTabChanged(tabId);
    showNotification("Created new editor buffer", "info", 2500);
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
        updateWindowTitle();
    }
}

void MainWindow::closeAllTabs() {
    while (!tabs_.empty()) {
        closeTab(tabs_.back().id);
    }
}

void MainWindow::closeOtherTabs(const std::string& keepId) {
    std::vector<std::string> toClose;
    for (const auto& tab : tabs_) {
        if (tab.id != keepId && !tab.pinned) {
            toClose.push_back(tab.id);
        }
    }
    for (const auto& id : toClose) {
        closeTab(id);
    }
}

void MainWindow::setActiveTab(const std::string& id) {
    activeTabId_ = id;
    if (onActiveTabChanged) {
        onActiveTabChanged(id);
    }
    updateWindowTitle();
}

void MainWindow::nextTab() {
    if (tabs_.empty()) return;
    auto it = std::find_if(tabs_.begin(), tabs_.end(),
        [this](const TabInfo& tab) { return tab.id == activeTabId_; });
    size_t index = (it == tabs_.end()) ? 0 : (static_cast<size_t>(it - tabs_.begin()) + 1) % tabs_.size();
    setActiveTab(tabs_[index].id);
}

void MainWindow::previousTab() {
    if (tabs_.empty()) return;
    auto it = std::find_if(tabs_.begin(), tabs_.end(),
        [this](const TabInfo& tab) { return tab.id == activeTabId_; });
    size_t index = (it == tabs_.end())
                       ? (tabs_.size() - 1)
                       : (static_cast<size_t>(it - tabs_.begin()) + tabs_.size() - 1) % tabs_.size();
    setActiveTab(tabs_[index].id);
}

void MainWindow::moveTab(const std::string& id, int newIndex) {
    if (tabs_.empty()) return;
    auto it = std::find_if(tabs_.begin(), tabs_.end(),
        [&id](const TabInfo& tab) { return tab.id == id; });
    if (it == tabs_.end()) return;

    TabInfo tab = std::move(*it);
    tabs_.erase(it);
    size_t clamped = static_cast<size_t>(std::clamp(newIndex, 0, static_cast<int>(tabs_.size())));
    tabs_.insert(tabs_.begin() + static_cast<std::ptrdiff_t>(clamped), std::move(tab));
}

void MainWindow::pinTab(const std::string& id) {
    for (auto& tab : tabs_) {
        if (tab.id == id) { tab.pinned = true; break; }
    }
}

void MainWindow::unpinTab(const std::string& id) {
    for (auto& tab : tabs_) {
        if (tab.id == id) { tab.pinned = false; break; }
    }
}

void MainWindow::addCustomPanel(const std::string& id, const std::string& title, Widget::Ptr content) {
    (void)id; (void)title; (void)content;
}
void MainWindow::removeCustomPanel(const std::string& id) { (void)id; }
void MainWindow::splitHorizontal() {}
void MainWindow::splitVertical() {}
void MainWindow::closeSplit() {}
void MainWindow::focusNextSplit() {}
void MainWindow::focusPreviousSplit() {}
void MainWindow::showDebugConsole() { activeBottomTab_ = 3; panelVisible_ = true; updateLayout(); }
void MainWindow::setStatusBarItems(const std::vector<StatusBarItem>& items) { statusBarItems_ = items; }
void MainWindow::updateStatusBarItem(const std::string& id, const std::string& text) {
    for (auto& item : statusBarItems_) if (item.id == id) { item.text = text; break; }
}
void MainWindow::addStatusBarItem(const StatusBarItem& item) { statusBarItems_.push_back(item); }
void MainWindow::removeStatusBarItem(const std::string& id) {
    statusBarItems_.erase(std::remove_if(statusBarItems_.begin(), statusBarItems_.end(),
        [&id](const StatusBarItem& i) { return i.id == id; }), statusBarItems_.end());
}
void MainWindow::showGoToLine() {}
void MainWindow::showGoToSymbol() {}
void MainWindow::showProgressNotification(const std::string& title, std::function<void(std::function<void(float)>)> task) {
    showNotification(title, "info", 3000);
    if (task) task([](float) {});
}
std::string MainWindow::showOpenDialog(const std::string& title, const std::vector<std::string>& filters) {
    return Platform::PlatformAbstraction::openFileDialog(title, filters, workspaceRoot_);
}
std::vector<std::string> MainWindow::showOpenMultipleDialog(const std::string& title, const std::vector<std::string>& filters) {
    (void)title; (void)filters; return {};
}
std::string MainWindow::showSaveDialog(const std::string& title, const std::string& defaultName, const std::vector<std::string>& filters) {
    return Platform::PlatformAbstraction::saveFileDialog(title, filters, defaultName);
}
std::string MainWindow::showFolderDialog(const std::string& title) {
    return Platform::PlatformAbstraction::selectDirectoryDialog(title, workspaceRoot_);
}
int MainWindow::showMessageBox(const std::string& title, const std::string& message, const std::vector<std::string>& buttons) {
    (void)title; (void)message; (void)buttons; return 0;
}
void MainWindow::updateTabTitle(const std::string& id) { (void)id; updateWindowTitle(); }

void MainWindow::showTerminal() {
    activeBottomTab_ = 1;
    panelVisible_ = true;
    updateLayout();
}

void MainWindow::showProblems() {
    activeBottomTab_ = 2;
    panelVisible_ = true;
    updateLayout();
}

void MainWindow::showOutput() {
    activeBottomTab_ = 0;
    panelVisible_ = true;
    updateLayout();
}

void MainWindow::hideBottomPanel() {
    panelVisible_ = false;
    updateLayout();
}

void MainWindow::showCommandPalette() {
    commandPaletteOpen_ = true;
    commandPaletteSelected_ = 0;
}

void MainWindow::showQuickOpen() {
    showCommandPalette();
}

void MainWindow::showNotification(const std::string& message,
                                  const std::string& type, int durationMs) {
    (void)type;
    statusNotification_ = message;
    statusNotificationTimer_ = static_cast<double>(durationMs) / 1000.0;
}

void MainWindow::update(double deltaTime) {
    elapsedTime_ += deltaTime;
    cursorBlinkTime_ += deltaTime;

    if (statusNotificationTimer_ > 0.0) {
        statusNotificationTimer_ -= deltaTime;
        if (statusNotificationTimer_ <= 0.0) {
            statusNotification_ = "Ready";
        }
    }

    if (gestureHandler_) {
        gestureHandler_->update(deltaTime);
    }
    if (rootWidget_) {
        rootWidget_->update(deltaTime);
    }
}

void MainWindow::render() {
    if (!renderer_ || !nativeWindow_) return;

    renderer_->beginFrame();
    renderer_->setViewport(0, 0, windowWidth_, windowHeight_);
    renderer_->setProjection(Mat4::ortho(0.0f, static_cast<float>(windowWidth_),
                                         static_cast<float>(windowHeight_), 0.0f, -1.0f, 1.0f));
    renderer_->clear(COL_EDITOR_BG);

    renderTitleBar();
    renderActivityBar();
    if (sidebarVisible_) {
        renderSidebar();
    }
    renderTabBar();
    renderEditorArea();
    if (panelVisible_) {
        renderBottomPanel();
    }
    renderStatusBar();

    if (rootWidget_) {
        rootWidget_->render(*renderer_);
    }

    if (commandPaletteOpen_) {
        renderCommandPalette();
    }

    renderer_->endFrame();
    renderer_->present();
}

bool MainWindow::handlePlatformEvent(const Platform::PlatformEvent& pe) {
    if (pe.window != nullptr && pe.window != nativeWindow_) return false;

    switch (pe.type) {
        case Platform::PlatformEventType::WindowClose: {
            Core::WindowCloseEvent ev;
            return handleEvent(ev);
        }
        case Platform::PlatformEventType::WindowResize: {
            Core::WindowResizeEvent ev(pe.width, pe.height);
            return handleEvent(ev);
        }
        case Platform::PlatformEventType::KeyPress: {
            Core::KeyPressEvent ev(pe.keyCode, pe.scanCode, pe.modifiers, false);
            return handleEvent(ev);
        }
        case Platform::PlatformEventType::CharInput: {
            Core::CharInputEvent ev(pe.codepoint);
            return handleEvent(ev);
        }
        case Platform::PlatformEventType::MouseMove: {
            mouseX_ = static_cast<float>(pe.x);
            mouseY_ = static_cast<float>(pe.y);
            // Update hovered explorer row
            hoveredExplorerIndex_ = -1;
            if (sidebarVisible_ && activePanelType_ == PanelType::Explorer &&
                sidebarRect_.contains(mouseX_, mouseY_)) {
                float listTop = sidebarRect_.y + 56.0f;
                if (mouseY_ >= listTop) {
                    int idx = explorerScroll_ + static_cast<int>((mouseY_ - listTop) / 24.0f);
                    if (idx >= 0 && idx < static_cast<int>(explorerEntries_.size())) {
                        hoveredExplorerIndex_ = idx;
                    }
                }
            }
            return true;
        }
        case Platform::PlatformEventType::MouseButtonPress: {
            mouseX_ = static_cast<float>(pe.x);
            mouseY_ = static_cast<float>(pe.y);
            return handleMouseClick(mouseX_, mouseY_, pe.button);
        }
        case Platform::PlatformEventType::MouseScroll: {
            return handleMouseScroll(static_cast<float>(pe.x), static_cast<float>(pe.y), pe.scrollY);
        }
        default:
            return false;
    }
}

bool MainWindow::handleEvent(Core::Event& event) {
    if (auto* resizeEvent = dynamic_cast<Core::WindowResizeEvent*>(&event)) {
        windowWidth_ = std::max(320, resizeEvent->width);
        windowHeight_ = std::max(240, resizeEvent->height);
        if (onResize) {
            onResize(windowWidth_, windowHeight_);
        }
        updateLayout();
        return true;
    }

    if (dynamic_cast<Core::WindowCloseEvent*>(&event)) {
        if (onClose) {
            onClose();
        }
        return true;
    }

    if (auto* keyEvent = dynamic_cast<Core::KeyPressEvent*>(&event)) {
        if (commandPaletteOpen_) {
            if (keyEvent->keyCode == Platform::KeyCode::Escape) {
                commandPaletteOpen_ = false;
                return true;
            }
            if (keyEvent->keyCode == Platform::KeyCode::Down && !paletteCommands_.empty()) {
                commandPaletteSelected_ = (commandPaletteSelected_ + 1) % static_cast<int>(paletteCommands_.size());
                return true;
            }
            if (keyEvent->keyCode == Platform::KeyCode::Up && !paletteCommands_.empty()) {
                commandPaletteSelected_ = (commandPaletteSelected_ + static_cast<int>(paletteCommands_.size()) - 1)
                                          % static_cast<int>(paletteCommands_.size());
                return true;
            }
            if (keyEvent->keyCode == Platform::KeyCode::Enter &&
                commandPaletteSelected_ >= 0 &&
                commandPaletteSelected_ < static_cast<int>(paletteCommands_.size())) {
                auto act = paletteCommands_[static_cast<size_t>(commandPaletteSelected_)].action;
                commandPaletteOpen_ = false;
                if (act) act();
                return true;
            }
            return true;
        }

        if (handleKeyboardShortcut(*keyEvent)) {
            return true;
        }
        if (handleEditorKeyPress(*keyEvent)) {
            return true;
        }
    }

    if (auto* charEvent = dynamic_cast<Core::CharInputEvent*>(&event)) {
        if (!commandPaletteOpen_) {
            return handleEditorCharInput(charEvent->codepoint);
        }
    }

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
    return nullptr;
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

void MainWindow::createTitleBar() {}

void MainWindow::createActivityBar() {
    activityItems_ = {
        {"explorer",   "E", "Explorer",       PanelType::Explorer,      0, true},
        {"search",     "S", "Search",         PanelType::Search,        0, false},
        {"git",        "G", "Source Control", PanelType::SourceControl, 0, false},
        {"debug",      "D", "Run and Debug",  PanelType::Debug,         0, false},
        {"extensions", "X", "Extensions",     PanelType::Extensions,    0, false},
        {"ai",         "A", "Phoenix AI",     PanelType::AIChat,        0, false}
    };
    activeActivityId_ = "explorer";
    activePanelType_ = PanelType::Explorer;
}

void MainWindow::createSidebar() {}
void MainWindow::createEditorArea() {}
void MainWindow::createBottomPanel() {
    addOutputLog("[boot] NexusForge IDE 1.0.0 (Phoenix) started");
    addOutputLog("[core] Workspace: " + workspaceRoot_);
    addOutputLog("[ui]   Software 2D renderer active (60 FPS)");
    addOutputLog("[ai]   Phoenix AI Assistant initialized and ready");
}
void MainWindow::createStatusBar() {}
void MainWindow::createMobileLayout() {}
void MainWindow::createOverlays() {}

void MainWindow::detectGitBranch() {
    std::string head = Platform::PlatformAbstraction::readFile(".git/HEAD");
    const std::string prefix = "ref: refs/heads/";
    if (head.rfind(prefix, 0) == 0) {
        gitBranch_ = head.substr(prefix.size());
        while (!gitBranch_.empty() && (gitBranch_.back() == '\n' || gitBranch_.back() == '\r')) {
            gitBranch_.pop_back();
        }
    } else if (!head.empty()) {
        gitBranch_ = head.substr(0, std::min<size_t>(7, head.size()));
    }
}

void MainWindow::loadWorkspaceTree() {
    explorerEntries_.clear();
    auto files = Platform::PlatformAbstraction::listDirectory(".");
    std::sort(files.begin(), files.end(), [](const Platform::FileInfo& a, const Platform::FileInfo& b) {
        if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
        return a.name < b.name;
    });

    for (const auto& f : files) {
        if (f.name == ".git" || f.name == "out" || f.name == ".cache") continue;
        ExplorerEntry e;
        e.name = f.name;
        e.path = f.name;
        e.isDirectory = f.isDirectory;
        e.expanded = false;
        e.depth = 0;
        explorerEntries_.push_back(std::move(e));
    }
}

void MainWindow::toggleExplorerFolder(size_t index) {
    if (index >= explorerEntries_.size()) return;
    ExplorerEntry& entry = explorerEntries_[index];
    if (!entry.isDirectory) return;

    if (entry.expanded) {
        entry.expanded = false;
        int parentDepth = entry.depth;
        size_t eraseEnd = index + 1;
        while (eraseEnd < explorerEntries_.size() && explorerEntries_[eraseEnd].depth > parentDepth) {
            ++eraseEnd;
        }
        explorerEntries_.erase(explorerEntries_.begin() + static_cast<std::ptrdiff_t>(index + 1),
                               explorerEntries_.begin() + static_cast<std::ptrdiff_t>(eraseEnd));
    } else {
        entry.expanded = true;
        auto children = Platform::PlatformAbstraction::listDirectory(entry.path);
        std::sort(children.begin(), children.end(), [](const Platform::FileInfo& a, const Platform::FileInfo& b) {
            if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
            return a.name < b.name;
        });
        std::vector<ExplorerEntry> inserted;
        for (const auto& f : children) {
            if (f.name == ".git" || f.name == "out") continue;
            ExplorerEntry child;
            child.name = f.name;
            child.path = entry.path + "/" + f.name;
            child.isDirectory = f.isDirectory;
            child.depth = entry.depth + 1;
            inserted.push_back(std::move(child));
        }
        explorerEntries_.insert(explorerEntries_.begin() + static_cast<std::ptrdiff_t>(index + 1),
                                inserted.begin(), inserted.end());
    }
}

void MainWindow::initPaletteCommands() {
    paletteCommands_ = {
        {"File: New File",           "Ctrl+N",       [this]() { newFile("C++"); }},
        {"File: Save Active File",   "Ctrl+S",       [this]() { saveActiveFile(); }},
        {"View: Open Welcome Page",  "",             [this]() { openWelcomeTab(); }},
        {"View: Toggle Sidebar",     "Ctrl+B",       [this]() { setSidebarVisible(!sidebarVisible_); }},
        {"View: Toggle Bottom Panel","Ctrl+`",       [this]() { setPanelVisible(!panelVisible_); }},
        {"View: Show Explorer",      "",             [this]() { showPanel(PanelType::Explorer); }},
        {"View: Show Phoenix AI",    "",             [this]() { showPanel(PanelType::AIChat); }},
        {"View: Show Terminal",      "",             [this]() { showTerminal(); }},
        {"App: Quit NexusForge IDE", "Ctrl+Q",       [this]() { if (onClose) onClose(); }}
    };
}

void MainWindow::layoutDesktop() {
    float w = static_cast<float>(windowWidth_);
    float h = static_cast<float>(windowHeight_);

    const float titleH = 36.0f;
    const float statusH = 26.0f;
    const float activityW = 48.0f;
    const float tabH = 34.0f;
    const float crumbH = 24.0f;

    float bodyH = std::max(100.0f, h - titleH - statusH);
    float sideW = sidebarVisible_ ? std::min(sidebarWidth_, w * 0.35f) : 0.0f;
    float workX = activityW + sideW;
    float workW = std::max(200.0f, w - workX);
    float botH  = panelVisible_ ? std::min(panelHeight_, bodyH * 0.45f) : 0.0f;
    float edTotalH = std::max(80.0f, bodyH - botH);

    titleBarRect_    = Rect(0.0f, 0.0f, w, titleH);
    float pillW      = std::clamp(w * 0.30f, 220.0f, 400.0f);
    searchPillRect_  = Rect((w - pillW) * 0.5f, 6.0f, pillW, 24.0f);
    activityBarRect_ = Rect(0.0f, titleH, activityW, bodyH);
    sidebarRect_     = Rect(activityW, titleH, sideW, bodyH);
    tabBarRect_      = Rect(workX, titleH, workW, tabH);
    breadcrumbRect_  = Rect(workX, titleH + tabH, workW, crumbH);
    editorRect_      = Rect(workX, titleH + tabH + crumbH, workW, std::max(40.0f, edTotalH - tabH - crumbH));
    bottomPanelRect_ = Rect(workX, titleH + edTotalH, workW, botH);
    statusBarRect_   = Rect(0.0f, h - statusH, w, statusH);
}

void MainWindow::layoutTablet() { layoutDesktop(); }
void MainWindow::layoutPhone()  { layoutDesktop(); }

void MainWindow::updateLayout() {
    switch (currentFormFactor_) {
        case Mobile::FormFactor::Phone:  layoutPhone();   break;
        case Mobile::FormFactor::Tablet: layoutTablet();  break;
        default:                         layoutDesktop(); break;
    }
}

void MainWindow::renderTitleBar() {
    renderer_->drawRect(titleBarRect_, COL_TITLEBAR);
    renderer_->drawLine(0.0f, titleBarRect_.bottom() - 0.5f, titleBarRect_.right(),
                        titleBarRect_.bottom() - 0.5f, COL_BORDER, 1.0f);

    // Top 2px brand accent strip
    renderer_->drawHorizontalGradient(Rect(0.0f, 0.0f, titleBarRect_.width, 2.0f),
                                      COL_ACCENT, COL_ACCENT_VIOLET);

    // Mini hexagon NF brand mark on top-left
    float cx = 24.0f, cy = 19.0f;
    renderer_->beginPath();
    for (int i = 0; i < 6; ++i) {
        float ang = i * 3.14159265f / 3.0f - 3.14159265f / 6.0f;
        float hx = cx + std::cos(ang) * 10.5f;
        float hy = cy + std::sin(ang) * 10.5f;
        if (i == 0) renderer_->moveTo(hx, hy);
        else renderer_->lineTo(hx, hy);
    }
    renderer_->closePath();
    renderer_->fillPath(COL_ACCENT);
    renderer_->drawTextCentered("NF", Rect(cx - 9.0f, cy - 6.0f, 18.0f, 12.0f),
                                Color::White(), "default", 8.5f);

    // Menu bar labels
    static const char* menus[] = {"File", "Edit", "View", "Go", "Run", "Terminal", "Help"};
    float mx = 54.0f;
    for (const char* m : menus) {
        Vec2 sz = renderer_->measureText(m, "default", 12.0f);
        Rect mr(mx - 6.0f, 7.0f, sz.x + 12.0f, 22.0f);
        if (mr.contains(mouseX_, mouseY_)) {
            renderer_->drawRoundedRect(mr, COL_SURFACE, 4.0f);
        }
        renderer_->drawText(m, mx, 12.0f, COL_TEXT_MUTED, "default", 12.0f);
        mx += sz.x + 18.0f;
    }

    // Center clickable Command Palette search pill
    bool pillHovered = searchPillRect_.contains(mouseX_, mouseY_);
    renderer_->drawRoundedRect(searchPillRect_, pillHovered ? COL_SURFACE_HOVER : COL_SURFACE, 6.0f);
    renderer_->drawTextCentered(
        workspaceName_ + "  -  Command Palette (Ctrl+P)",
        searchPillRect_, COL_TEXT_MUTED, "default", 11.5f);

    // Right side quick toggles: AI badge + Panel + Sidebar
    float rx = titleBarRect_.right() - 16.0f;
    Rect panelBtn(rx - 56.0f, 7.0f, 56.0f, 22.0f);
    renderer_->drawRoundedRect(panelBtn, panelVisible_ ? Color(0.18f, 0.28f, 0.48f, 1.0f) : COL_SURFACE, 4.0f);
    renderer_->drawTextCentered("Panel", panelBtn, panelVisible_ ? COL_TEXT_PRIMARY : COL_TEXT_MUTED, "default", 11.0f);

    Rect sideBtn(panelBtn.x - 68.0f, 7.0f, 62.0f, 22.0f);
    renderer_->drawRoundedRect(sideBtn, sidebarVisible_ ? Color(0.18f, 0.28f, 0.48f, 1.0f) : COL_SURFACE, 4.0f);
    renderer_->drawTextCentered("Sidebar", sideBtn, sidebarVisible_ ? COL_TEXT_PRIMARY : COL_TEXT_MUTED, "default", 11.0f);

    Rect aiPill(sideBtn.x - 86.0f, 7.0f, 78.0f, 22.0f);
    renderer_->drawRoundedRect(aiPill, Color(0.28f, 0.20f, 0.52f, 0.85f), 11.0f);
    renderer_->drawTextCentered("AI Ready", aiPill, Color(0.84f, 0.78f, 1.0f, 1.0f), "default", 11.0f);
}

void MainWindow::drawActivityIcon(PanelType type, float cx, float cy, const Color& color) {
    switch (type) {
        case PanelType::Explorer:
            // Stacked file sheets icon
            renderer_->drawRect(Rect(cx - 8.0f, cy - 9.0f, 12.0f, 15.0f), Color(color.r, color.g, color.b, 0.22f));
            renderer_->drawLine(cx - 8.0f, cy - 9.0f, cx + 4.0f, cy - 9.0f, color, 1.6f);
            renderer_->drawLine(cx - 8.0f, cy - 9.0f, cx - 8.0f, cy + 6.0f, color, 1.6f);
            renderer_->drawLine(cx - 8.0f, cy + 6.0f, cx + 4.0f, cy + 6.0f, color, 1.6f);
            renderer_->drawLine(cx + 4.0f, cy - 9.0f, cx + 4.0f, cy + 6.0f, color, 1.6f);
            renderer_->drawLine(cx - 3.0f, cy + 9.0f, cx + 8.0f, cy + 9.0f, color, 1.6f);
            renderer_->drawLine(cx + 8.0f, cy - 4.0f, cx + 8.0f, cy + 9.0f, color, 1.6f);
            break;
        case PanelType::Search:
            // Magnifying glass
            renderer_->drawCircle(cx - 2.0f, cy - 2.0f, 6.5f, color, false);
            renderer_->drawLine(cx + 3.0f, cy + 3.0f, cx + 8.5f, cy + 8.5f, color, 2.0f);
            break;
        case PanelType::SourceControl:
            // Git branch nodes
            renderer_->drawLine(cx - 4.0f, cy - 6.0f, cx - 4.0f, cy + 6.0f, color, 1.8f);
            renderer_->drawLine(cx - 4.0f, cy + 2.0f, cx + 5.0f, cy - 3.0f, color, 1.8f);
            renderer_->drawCircle(cx - 4.0f, cy - 7.0f, 2.8f, color, true);
            renderer_->drawCircle(cx - 4.0f, cy + 7.0f, 2.8f, color, true);
            renderer_->drawCircle(cx + 5.0f, cy - 4.0f, 2.8f, color, true);
            break;
        case PanelType::Debug:
            // Play triangle
            renderer_->drawTriangle(Vec2(cx - 5.0f, cy - 8.0f),
                                    Vec2(cx - 5.0f, cy + 8.0f),
                                    Vec2(cx + 8.0f, cy), color, true);
            break;
        case PanelType::Extensions:
            // Four grid blocks
            renderer_->drawRoundedRect(Rect(cx - 8.0f, cy - 8.0f, 6.5f, 6.5f), color, 1.5f);
            renderer_->drawRoundedRect(Rect(cx + 1.5f, cy - 8.0f, 6.5f, 6.5f), Color(color.r, color.g, color.b, 0.55f), 1.5f);
            renderer_->drawRoundedRect(Rect(cx - 8.0f, cy + 1.5f, 6.5f, 6.5f), color, 1.5f);
            renderer_->drawRoundedRect(Rect(cx + 1.5f, cy + 1.5f, 6.5f, 6.5f), color, 1.5f);
            break;
        case PanelType::AIChat:
            // Four-point Phoenix AI sparkle
            renderer_->beginPath();
            renderer_->moveTo(cx, cy - 9.0f);
            renderer_->lineTo(cx + 2.8f, cy - 2.8f);
            renderer_->lineTo(cx + 9.0f, cy);
            renderer_->lineTo(cx + 2.8f, cy + 2.8f);
            renderer_->lineTo(cx, cy + 9.0f);
            renderer_->lineTo(cx - 2.8f, cy + 2.8f);
            renderer_->lineTo(cx - 9.0f, cy);
            renderer_->lineTo(cx - 2.8f, cy - 2.8f);
            renderer_->closePath();
            renderer_->fillPath(color);
            break;
        default:
            renderer_->drawCircle(cx, cy, 6.0f, color, false);
            break;
    }
}

void MainWindow::renderActivityBar() {
    renderer_->drawRect(activityBarRect_, COL_ACTIVITYBAR);
    renderer_->drawLine(activityBarRect_.right() - 0.5f, activityBarRect_.y,
                        activityBarRect_.right() - 0.5f, activityBarRect_.bottom(), COL_BORDER, 1.0f);

    float itemH = 48.0f;
    for (size_t i = 0; i < activityItems_.size(); ++i) {
        const auto& item = activityItems_[i];
        Rect r(activityBarRect_.x, activityBarRect_.y + 8.0f + static_cast<float>(i) * itemH,
               activityBarRect_.width, itemH);
        bool active = sidebarVisible_ && (item.panelType == activePanelType_);
        bool hovered = r.contains(mouseX_, mouseY_);

        if (active) {
            renderer_->drawRect(Rect(r.x, r.y + 6.0f, 3.0f, r.height - 12.0f), COL_ACCENT);
            renderer_->drawRect(Rect(r.x + 3.0f, r.y + 2.0f, r.width - 6.0f, r.height - 4.0f),
                                Color(0.24f, 0.45f, 0.90f, 0.14f));
        } else if (hovered) {
            renderer_->drawRect(Rect(r.x + 3.0f, r.y + 2.0f, r.width - 6.0f, r.height - 4.0f),
                                Color(1.0f, 1.0f, 1.0f, 0.05f));
        }

        Color iconCol = active ? COL_TEXT_PRIMARY : (hovered ? Color(0.80f, 0.85f, 0.95f, 1.0f) : COL_TEXT_DIM);
        if (item.panelType == PanelType::AIChat && active) iconCol = COL_ACCENT_VIOLET;
        drawActivityIcon(item.panelType, r.x + r.width * 0.5f, r.y + r.height * 0.5f, iconCol);
    }
}

void MainWindow::renderSidebar() {
    if (sidebarRect_.width <= 1.0f) return;
    renderer_->drawRect(sidebarRect_, COL_SIDEBAR);
    renderer_->drawLine(sidebarRect_.right() - 0.5f, sidebarRect_.y,
                        sidebarRect_.right() - 0.5f, sidebarRect_.bottom(), COL_BORDER, 1.0f);

    renderer_->pushClipRect(sidebarRect_);

    // Panel header
    const char* title = "EXPLORER";
    switch (activePanelType_) {
        case PanelType::Explorer:      title = "EXPLORER"; break;
        case PanelType::Search:        title = "SEARCH"; break;
        case PanelType::SourceControl: title = "SOURCE CONTROL"; break;
        case PanelType::Debug:         title = "RUN AND DEBUG"; break;
        case PanelType::Extensions:    title = "EXTENSIONS"; break;
        case PanelType::AIChat:        title = "PHOENIX AI"; break;
        default: break;
    }

    renderer_->drawText(title, sidebarRect_.x + 14.0f, sidebarRect_.y + 11.0f,
                        COL_TEXT_MUTED, "default", 11.0f);
    renderer_->drawLine(sidebarRect_.x, sidebarRect_.y + 32.0f,
                        sidebarRect_.right(), sidebarRect_.y + 32.0f, COL_BORDER, 1.0f);

    if (activePanelType_ == PanelType::Explorer) {
        renderExplorerSidebar();
    } else if (activePanelType_ == PanelType::Search) {
        float y = sidebarRect_.y + 44.0f;
        renderer_->drawRoundedRect(Rect(sidebarRect_.x + 12.0f, y, sidebarRect_.width - 24.0f, 28.0f),
                                   COL_SURFACE, 5.0f);
        renderer_->drawText("Search workspace...", sidebarRect_.x + 20.0f, y + 8.0f,
                            COL_TEXT_DIM, "default", 11.5f);
        y += 42.0f;
        renderer_->drawText("Indexed files: " + std::to_string(explorerEntries_.size()),
                            sidebarRect_.x + 14.0f, y, COL_TEXT_MUTED, "default", 11.5f);
    } else if (activePanelType_ == PanelType::SourceControl) {
        float y = sidebarRect_.y + 44.0f;
        renderer_->drawRoundedRect(Rect(sidebarRect_.x + 12.0f, y, sidebarRect_.width - 24.0f, 54.0f),
                                   COL_SURFACE, 6.0f);
        renderer_->drawText("Branch: " + gitBranch_, sidebarRect_.x + 22.0f, y + 12.0f,
                            COL_ACCENT, "default", 12.5f);
        renderer_->drawText("Git integration active", sidebarRect_.x + 22.0f, y + 32.0f,
                            COL_TEXT_MUTED, "default", 11.0f);
    } else if (activePanelType_ == PanelType::Debug) {
        float y = sidebarRect_.y + 44.0f;
        renderer_->drawRoundedRect(Rect(sidebarRect_.x + 12.0f, y, sidebarRect_.width - 24.0f, 34.0f),
                                   Color(0.18f, 0.36f, 0.72f, 1.0f), 6.0f);
        renderer_->drawTextCentered("Launch NexusForge", Rect(sidebarRect_.x + 12.0f, y, sidebarRect_.width - 24.0f, 34.0f),
                                    Color::White(), "default", 12.0f);
        renderer_->drawText("DAP Debugger ready", sidebarRect_.x + 14.0f, y + 48.0f,
                            COL_TEXT_MUTED, "default", 11.5f);
    } else if (activePanelType_ == PanelType::Extensions) {
        static const char* exts[] = {
            "C/C++ IntelliSense",
            "Tree-Sitter Syntax",
            "Phoenix AI Copilot",
            "Integrated Terminal",
            "Git Source Control"
        };
        float y = sidebarRect_.y + 42.0f;
        for (const char* ext : exts) {
            Rect card(sidebarRect_.x + 10.0f, y, sidebarRect_.width - 20.0f, 44.0f);
            renderer_->drawRoundedRect(card, COL_SURFACE, 6.0f);
            renderer_->drawText(ext, card.x + 10.0f, card.y + 8.0f, COL_TEXT_PRIMARY, "default", 12.0f);
            renderer_->drawText("Built-in  -  Active", card.x + 10.0f, card.y + 25.0f, COL_GREEN, "default", 10.5f);
            y += 52.0f;
        }
    } else if (activePanelType_ == PanelType::AIChat) {
        float y = sidebarRect_.y + 44.0f;
        Rect banner(sidebarRect_.x + 10.0f, y, sidebarRect_.width - 20.0f, 78.0f);
        renderer_->drawRoundedRect(banner, Color(0.17f, 0.14f, 0.32f, 1.0f), 8.0f);
        renderer_->drawText("Phoenix AI Assistant", banner.x + 12.0f, banner.y + 12.0f,
                            Color(0.84f, 0.78f, 1.0f, 1.0f), "default", 13.0f);
        renderer_->drawText("Code completion, refactoring", banner.x + 12.0f, banner.y + 34.0f,
                            COL_TEXT_MUTED, "default", 11.0f);
        renderer_->drawText("and chat ready locally.", banner.x + 12.0f, banner.y + 50.0f,
                            COL_TEXT_MUTED, "default", 11.0f);
    }

    renderer_->popClipRect();
}

void MainWindow::renderExplorerSidebar() {
    // Project root folder header
    float y = sidebarRect_.y + 34.0f;
    renderer_->drawRect(Rect(sidebarRect_.x, y, sidebarRect_.width, 22.0f),
                        Color(0.095f, 0.115f, 0.175f, 1.0f));
    renderer_->drawText("v " + workspaceName_, sidebarRect_.x + 10.0f, y + 5.0f,
                        COL_TEXT_PRIMARY, "default", 11.5f);

    float listTop = sidebarRect_.y + 56.0f;
    float rowH = 24.0f;
    int maxVisible = std::max(1, static_cast<int>((sidebarRect_.bottom() - listTop) / rowH));
    TabInfo* activeTab = getActiveTab();

    for (int i = 0; i < maxVisible; ++i) {
        int idx = explorerScroll_ + i;
        if (idx < 0 || idx >= static_cast<int>(explorerEntries_.size())) break;

        const auto& entry = explorerEntries_[static_cast<size_t>(idx)];
        Rect rowRect(sidebarRect_.x + 2.0f, listTop + static_cast<float>(i) * rowH,
                     sidebarRect_.width - 4.0f, rowH);

        bool isActive = (activeTab && !activeTab->filePath.empty() && activeTab->filePath == entry.path);
        bool isHovered = (idx == hoveredExplorerIndex_);

        if (isActive) {
            renderer_->drawRoundedRect(rowRect, Color(0.20f, 0.36f, 0.68f, 0.50f), 4.0f);
        } else if (isHovered) {
            renderer_->drawRoundedRect(rowRect, COL_SURFACE, 4.0f);
        }

        float indent = 12.0f + static_cast<float>(entry.depth) * 14.0f;
        float bx = rowRect.x + indent;

        if (entry.isDirectory) {
            renderer_->drawText(entry.expanded ? "v" : ">", bx, rowRect.y + 6.0f,
                                COL_TEXT_MUTED, "default", 11.0f);
            bx += 12.0f;
        }

        // Colored file/folder badge pill
        Color badgeCol = fileBadgeColor(entry.name, entry.isDirectory);
        Rect badge(bx, rowRect.y + 5.0f, 14.0f, 14.0f);
        renderer_->drawRoundedRect(badge, Color(badgeCol.r, badgeCol.g, badgeCol.b, 0.22f), 3.0f);
        renderer_->drawTextCentered(fileBadgeTag(entry.name, entry.isDirectory), badge, badgeCol, "default", 9.0f);

        renderer_->drawText(
            entry.name,
            bx + 20.0f, rowRect.y + 6.0f,
            sidebarRect_.right() - (bx + 26.0f),
            isActive ? Color::White() : (entry.isDirectory ? COL_TEXT_PRIMARY : COL_TEXT_MUTED),
            "default", 12.0f
        );
    }
}

void MainWindow::renderTabBar() {
    renderer_->drawRect(tabBarRect_, COL_TABBAR_BG);
    renderer_->drawLine(tabBarRect_.x, tabBarRect_.bottom() - 0.5f,
                        tabBarRect_.right(), tabBarRect_.bottom() - 0.5f, COL_BORDER, 1.0f);

    renderer_->pushClipRect(tabBarRect_);

    float tx = tabBarRect_.x;
    for (const auto& tab : tabs_) {
        bool active = (tab.id == activeTabId_);
        std::string label = tab.modified ? ("* " + tab.title) : tab.title;
        Vec2 textSz = renderer_->measureText(label, "default", 12.0f);
        float tabW = std::clamp(textSz.x + 56.0f, 108.0f, 220.0f);
        Rect r(tx, tabBarRect_.y, tabW, tabBarRect_.height);

        if (active) {
            renderer_->drawRect(r, COL_EDITOR_BG);
            renderer_->drawRect(Rect(r.x, r.y, r.width, 2.5f), COL_ACCENT);
        } else if (r.contains(mouseX_, mouseY_)) {
            renderer_->drawRect(r, COL_SURFACE);
        }
        renderer_->drawLine(r.right() - 0.5f, r.y + 4.0f, r.right() - 0.5f, r.bottom() - 4.0f, COL_BORDER, 1.0f);

        // Dot color indicator
        Color dotCol = tab.isWelcome ? COL_ACCENT_VIOLET : fileBadgeColor(tab.title, false);
        renderer_->drawCircle(r.x + 14.0f, r.y + r.height * 0.5f, 3.5f, dotCol, true);

        renderer_->drawText(label, r.x + 24.0f, r.y + 10.5f, r.width - 44.0f,
                            active ? COL_TEXT_PRIMARY : COL_TEXT_MUTED, "default", 12.0f);

        // Close 'x' button
        Rect closeR(r.right() - 22.0f, r.y + 9.0f, 16.0f, 16.0f);
        if (closeR.contains(mouseX_, mouseY_)) {
            renderer_->drawRoundedRect(closeR, COL_SURFACE_HOVER, 3.0f);
        }
        renderer_->drawTextCentered("x", closeR, COL_TEXT_DIM, "default", 10.5f);

        tx += tabW;
    }

    // '+' New file button
    Rect plusR(tx + 6.0f, tabBarRect_.y + 6.0f, 22.0f, 22.0f);
    if (plusR.contains(mouseX_, mouseY_)) {
        renderer_->drawRoundedRect(plusR, COL_SURFACE_HOVER, 4.0f);
    }
    renderer_->drawTextCentered("+", plusR, COL_TEXT_MUTED, "default", 14.0f);

    renderer_->popClipRect();

    // Breadcrumb strip below tabs
    renderer_->drawRect(breadcrumbRect_, COL_EDITOR_BG);
    renderer_->drawLine(breadcrumbRect_.x, breadcrumbRect_.bottom() - 0.5f,
                        breadcrumbRect_.right(), breadcrumbRect_.bottom() - 0.5f,
                        Color(0.14f, 0.17f, 0.25f, 1.0f), 1.0f);

    TabInfo* active = getActiveTab();
    std::string crumb = workspaceName_;
    if (active) {
        crumb += "  >  " + (active->filePath.empty() ? active->title : active->filePath);
    }
    renderer_->drawText(crumb, breadcrumbRect_.x + 14.0f, breadcrumbRect_.y + 6.0f,
                        COL_TEXT_DIM, "default", 11.0f);
}

void MainWindow::renderEditorArea() {
    renderer_->drawRect(editorRect_, COL_EDITOR_BG);
    renderer_->pushClipRect(editorRect_);

    TabInfo* active = getActiveTab();
    if (!active || active->isWelcome) {
        renderWelcomeView(editorRect_);
    } else {
        renderCodeEditorView(*active, editorRect_);
    }

    renderer_->popClipRect();
}

void MainWindow::renderWelcomeView(const Rect& area) {
    // Soft background gradient accent
    renderer_->drawRadialGradient(
        Rect(area.x + area.width * 0.5f - 260.0f, area.y - 80.0f, 520.0f, 320.0f),
        Color(0.25f, 0.48f, 1.0f, 0.10f),
        Color(0.25f, 0.48f, 1.0f, 0.0f)
    );

    float padX = area.x + 44.0f;
    float y = area.y + 32.0f;

    // Header: Hexagon logo + NEXUSFORGE IDE title
    float cx = padX + 28.0f, cy = y + 28.0f;
    renderer_->beginPath();
    for (int i = 0; i < 6; ++i) {
        float ang = i * 3.14159265f / 3.0f - 3.14159265f / 6.0f;
        float hx = cx + std::cos(ang) * 28.0f;
        float hy = cy + std::sin(ang) * 28.0f;
        if (i == 0) renderer_->moveTo(hx, hy);
        else renderer_->lineTo(hx, hy);
    }
    renderer_->closePath();
    renderer_->fillPath(COL_ACCENT);
    renderer_->drawTextCentered("NF", Rect(cx - 20.0f, cy - 13.0f, 40.0f, 26.0f),
                                Color::White(), "default", 19.0f);

    renderer_->drawText("NEXUSFORGE IDE", padX + 74.0f, y + 6.0f, COL_TEXT_PRIMARY, "default", 24.0f);
    renderer_->drawText("Phoenix Edition  v1.0.0  -  Code Without Boundaries",
                        padX + 74.0f, y + 36.0f, COL_TEXT_MUTED, "default", 13.0f);

    y += 84.0f;

    // Two-column layout inside welcome view
    float col1W = std::min(340.0f, (area.width - 120.0f) * 0.48f);
    float col2X = padX + col1W + 36.0f;
    float col2W = std::max(200.0f, area.right() - col2X - 40.0f);

    // Column 1: Quick Actions
    renderer_->drawText("Start", padX, y, COL_TEXT_PRIMARY, "default", 15.0f);
    float ay = y + 28.0f;
    struct ActionCard { const char* tag; const char* title; const char* sub; Color col; };
    ActionCard actions[] = {
        {"+", "New C++ File",      "Create a new editable buffer (Ctrl+N)", COL_ACCENT},
        {"C", "Open main.cpp",     "Inspect entry point with highlighting", COL_CYAN},
        {"$", "Toggle Terminal",   "Show or hide bottom panel (Ctrl+`)", COL_GREEN},
        {"P", "Command Palette",   "Run commands & switch views (Ctrl+P)", COL_ACCENT_VIOLET}
    };
    for (const auto& act : actions) {
        Rect card(padX, ay, col1W, 48.0f);
        bool hov = card.contains(mouseX_, mouseY_);
        renderer_->drawRoundedRect(card, hov ? COL_SURFACE_HOVER : COL_SURFACE, 7.0f);
        Rect iconR(card.x + 12.0f, card.y + 11.0f, 26.0f, 26.0f);
        renderer_->drawRoundedRect(iconR, Color(act.col.r, act.col.g, act.col.b, 0.22f), 5.0f);
        renderer_->drawTextCentered(act.tag, iconR, act.col, "default", 13.0f);
        renderer_->drawText(act.title, card.x + 48.0f, card.y + 8.0f, COL_TEXT_PRIMARY, "default", 12.5f);
        renderer_->drawText(act.sub, card.x + 48.0f, card.y + 26.0f, card.width - 56.0f,
                            COL_TEXT_DIM, "default", 10.5f);
        ay += 58.0f;
    }

    // Column 2: Workspace Files + Shortcuts
    renderer_->drawText("Workspace Files (" + workspaceName_ + ")", col2X, y, COL_TEXT_PRIMARY, "default", 15.0f);
    float fy = y + 28.0f;
    int shown = 0;
    for (const auto& entry : explorerEntries_) {
        if (entry.isDirectory || shown >= 6) continue;
        Rect row(col2X, fy, col2W, 30.0f);
        bool hov = row.contains(mouseX_, mouseY_);
        if (hov) {
            renderer_->drawRoundedRect(row, COL_SURFACE, 5.0f);
        }
        Color bc = fileBadgeColor(entry.name, false);
        Rect badge(row.x + 8.0f, row.y + 7.0f, 16.0f, 16.0f);
        renderer_->drawRoundedRect(badge, Color(bc.r, bc.g, bc.b, 0.22f), 3.0f);
        renderer_->drawTextCentered(fileBadgeTag(entry.name, false), badge, bc, "default", 9.5f);
        renderer_->drawText(entry.name, row.x + 32.0f, row.y + 8.0f, COL_TEXT_PRIMARY, "default", 12.5f);
        renderer_->drawText(detectLanguage(entry.name), row.right() - 74.0f, row.y + 9.0f,
                            COL_TEXT_DIM, "default", 11.0f);
        fy += 34.0f;
        ++shown;
    }

    // Shortcuts hint box
    float tipY = std::max(ay, fy) + 14.0f;
    if (tipY + 46.0f < area.bottom()) {
        Rect tipBox(padX, tipY, area.width - 88.0f, 44.0f);
        renderer_->drawRoundedRect(tipBox, Color(0.11f, 0.14f, 0.22f, 1.0f), 6.0f);
        renderer_->drawText("Tip: Click any file in the Explorer sidebar to open it, or press Ctrl+P for Command Palette.",
                            tipBox.x + 14.0f, tipBox.y + 15.0f, tipBox.width - 28.0f,
                            COL_TEXT_MUTED, "default", 11.5f);
    }
}

void MainWindow::drawHighlightedLine(const std::string& line, const std::string& lang,
                                     float x, float y, float maxX, float fontSize) {
    if (line.empty()) return;
    float cx = x;
    size_t i = 0;
    size_t n = line.size();

    auto emitToken = [&](const std::string& tok, const Color& col) {
        if (tok.empty() || cx >= maxX) return;
        renderer_->drawText(tok, cx, y, std::max(0.0f, maxX - cx), col, "mono", fontSize);
        cx += renderer_->measureText(tok, "mono", fontSize).x;
    };

    // Full-line comments & preprocessor directives
    size_t firstNonSpace = line.find_first_not_of(" \t");
    if (firstNonSpace != std::string::npos) {
        if ((lang == "C++" && line.compare(firstNonSpace, 2, "//") == 0) ||
            ((lang == "Shell" || lang == "Python" || lang == "CMake") && line[firstNonSpace] == '#')) {
            emitToken(line, Color(0.45f, 0.54f, 0.65f, 1.0f));
            return;
        }
        if (lang == "C++" && line[firstNonSpace] == '#') {
            emitToken(line, Color(0.76f, 0.50f, 0.96f, 1.0f));
            return;
        }
    }

    while (i < n && cx < maxX) {
        char c = line[i];

        // Inline // comment
        if (lang == "C++" && c == '/' && i + 1 < n && line[i + 1] == '/') {
            emitToken(line.substr(i), Color(0.45f, 0.54f, 0.65f, 1.0f));
            break;
        }

        // String literal
        if (c == '"' || c == '\'') {
            char quote = c;
            size_t start = i++;
            while (i < n) {
                if (line[i] == '\\' && i + 1 < n) {
                    i += 2;
                    continue;
                }
                if (line[i] == quote) {
                    ++i;
                    break;
                }
                ++i;
            }
            emitToken(line.substr(start, i - start), COL_GREEN);
            continue;
        }

        // Numbers
        if (std::isdigit(static_cast<unsigned char>(c))) {
            size_t start = i++;
            while (i < n && (std::isalnum(static_cast<unsigned char>(line[i])) || line[i] == '.')) ++i;
            emitToken(line.substr(start, i - start), COL_AMBER);
            continue;
        }

        // Identifiers / keywords
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            size_t start = i++;
            while (i < n && (std::isalnum(static_cast<unsigned char>(line[i])) || line[i] == '_')) ++i;
            std::string word = line.substr(start, i - start);
            Color col = COL_TEXT_PRIMARY;
            if (isCppKeyword(word)) col = Color(0.74f, 0.52f, 1.00f, 1.0f);
            else if (isCppType(word)) col = COL_CYAN;
            else if (i < n && line[i] == '(') col = Color(0.42f, 0.72f, 1.00f, 1.0f);
            emitToken(word, col);
            continue;
        }

        // Whitespace or punctuation run
        size_t start = i++;
        while (i < n) {
            char nc = line[i];
            if (std::isalnum(static_cast<unsigned char>(nc)) || nc == '_' ||
                nc == '"' || nc == '\'' || (lang == "C++" && nc == '/' && i + 1 < n && line[i + 1] == '/')) {
                break;
            }
            ++i;
        }
        emitToken(line.substr(start, i - start), Color(0.70f, 0.76f, 0.88f, 1.0f));
    }
}

void MainWindow::renderCodeEditorView(TabInfo& tab, const Rect& area) {
    if (tab.lines.empty()) tab.lines.push_back("");
    tab.cursorLine = std::clamp(tab.cursorLine, 0, static_cast<int>(tab.lines.size()) - 1);
    tab.cursorCol = std::clamp(tab.cursorCol, 0, static_cast<int>(tab.lines[static_cast<size_t>(tab.cursorLine)].size()));

    const float gutterW = 54.0f;
    const float minimapW = (area.width > 520.0f) ? 68.0f : 0.0f;
    const float lineH = 20.0f;
    const float fontSize = 13.0f;

    float codeX = area.x + gutterW + 10.0f;
    float codeMaxX = area.right() - minimapW - 8.0f;

    // Gutter background
    renderer_->drawRect(Rect(area.x, area.y, gutterW, area.height), Color(0.082f, 0.096f, 0.145f, 1.0f));
    renderer_->drawLine(area.x + gutterW - 0.5f, area.y, area.x + gutterW - 0.5f, area.bottom(), COL_BORDER, 1.0f);

    int visibleLines = std::max(1, static_cast<int>(area.height / lineH) + 1);
    int maxScroll = std::max(0, static_cast<int>(tab.lines.size()) - visibleLines + 3);
    tab.scrollLine = std::clamp(tab.scrollLine, 0, maxScroll);

    for (int i = 0; i < visibleLines; ++i) {
        int lineIdx = tab.scrollLine + i;
        if (lineIdx >= static_cast<int>(tab.lines.size())) break;

        float y = area.y + 6.0f + static_cast<float>(i) * lineH;
        bool isCurrent = (lineIdx == tab.cursorLine);

        if (isCurrent) {
            renderer_->drawRect(Rect(area.x + gutterW, y - 2.0f, area.width - gutterW - minimapW, lineH),
                                Color(0.16f, 0.21f, 0.34f, 0.55f));
        }

        // Line number
        char numBuf[16];
        std::snprintf(numBuf, sizeof(numBuf), "%4d", lineIdx + 1);
        renderer_->drawText(numBuf, area.x + 10.0f, y + 2.0f,
                            isCurrent ? COL_TEXT_PRIMARY : COL_TEXT_DIM, "mono", 12.0f);

        // Syntax-highlighted source line
        drawHighlightedLine(tab.lines[static_cast<size_t>(lineIdx)], tab.language,
                            codeX, y + 2.0f, codeMaxX, fontSize);

        // Blinking cursor caret on current line
        if (isCurrent && (static_cast<int>(cursorBlinkTime_ * 2.0) % 2 == 0)) {
            std::string before = tab.lines[static_cast<size_t>(lineIdx)].substr(0, static_cast<size_t>(tab.cursorCol));
            float caretX = codeX + renderer_->measureText(before, "mono", fontSize).x;
            if (caretX < codeMaxX) {
                renderer_->drawRect(Rect(caretX, y, 2.0f, lineH - 3.0f), COL_ACCENT);
            }
        }
    }

    // Minimap strip on right
    if (minimapW > 0.0f) {
        Rect mm(area.right() - minimapW, area.y, minimapW, area.height);
        renderer_->drawRect(mm, Color(0.080f, 0.094f, 0.142f, 1.0f));
        renderer_->drawLine(mm.x + 0.5f, mm.y, mm.x + 0.5f, mm.bottom(), COL_BORDER, 1.0f);

        int totalLines = std::max(1, static_cast<int>(tab.lines.size()));
        float scaleY = std::min(3.0f, area.height / static_cast<float>(totalLines));
        float vpY = mm.y + static_cast<float>(tab.scrollLine) * scaleY;
        float vpH = std::max(18.0f, static_cast<float>(visibleLines) * scaleY);
        renderer_->drawRect(Rect(mm.x + 2.0f, vpY, mm.width - 4.0f, std::min(vpH, mm.height - (vpY - mm.y))),
                            Color(0.32f, 0.55f, 1.0f, 0.16f));

        int maxMiniLines = std::min(totalLines, static_cast<int>(mm.height / 3.0f));
        for (int i = 0; i < maxMiniLines; ++i) {
            const std::string& ln = tab.lines[static_cast<size_t>(i)];
            if (ln.empty()) continue;
            size_t first = ln.find_first_not_of(" \t");
            if (first == std::string::npos) continue;
            float mx = mm.x + 6.0f + std::min(16.0f, static_cast<float>(first) * 0.7f);
            float mw = std::min(mm.right() - mx - 4.0f, static_cast<float>(ln.size() - first) * 0.65f);
            if (mw > 1.0f) {
                renderer_->drawRect(Rect(mx, mm.y + 4.0f + static_cast<float>(i) * 3.0f, mw, 1.5f),
                                    Color(0.55f, 0.65f, 0.85f, 0.35f));
            }
        }
    }
}

void MainWindow::renderBottomPanel() {
    if (bottomPanelRect_.height <= 1.0f) return;
    renderer_->drawRect(bottomPanelRect_, COL_PANEL_BG);
    renderer_->drawLine(bottomPanelRect_.x, bottomPanelRect_.y + 0.5f,
                        bottomPanelRect_.right(), bottomPanelRect_.y + 0.5f, COL_BORDER, 1.0f);

    renderer_->pushClipRect(bottomPanelRect_);

    static const char* panelTabs[] = {"OUTPUT", "TERMINAL", "PROBLEMS", "DEBUG CONSOLE"};
    float tx = bottomPanelRect_.x + 14.0f;
    for (int i = 0; i < 4; ++i) {
        Vec2 sz = renderer_->measureText(panelTabs[i], "default", 11.0f);
        bool active = (activeBottomTab_ == i);
        renderer_->drawText(panelTabs[i], tx, bottomPanelRect_.y + 9.0f,
                            active ? COL_TEXT_PRIMARY : COL_TEXT_DIM, "default", 11.0f);
        if (active) {
            renderer_->drawRect(Rect(tx, bottomPanelRect_.y + 27.0f, sz.x, 2.0f), COL_ACCENT);
        }
        tx += sz.x + 22.0f;
    }

    // Close button on top right of bottom panel
    Rect closeR(bottomPanelRect_.right() - 28.0f, bottomPanelRect_.y + 6.0f, 18.0f, 18.0f);
    if (closeR.contains(mouseX_, mouseY_)) {
        renderer_->drawRoundedRect(closeR, COL_SURFACE_HOVER, 4.0f);
    }
    renderer_->drawTextCentered("x", closeR, COL_TEXT_MUTED, "default", 11.0f);

    renderer_->drawLine(bottomPanelRect_.x, bottomPanelRect_.y + 29.5f,
                        bottomPanelRect_.right(), bottomPanelRect_.y + 29.5f,
                        Color(0.14f, 0.17f, 0.25f, 1.0f), 1.0f);

    float contentY = bottomPanelRect_.y + 38.0f;
    float contentX = bottomPanelRect_.x + 16.0f;

    if (activeBottomTab_ == 0) {
        // OUTPUT logs
        int maxLines = std::max(1, static_cast<int>((bottomPanelRect_.bottom() - contentY - 6.0f) / 18.0f));
        int start = std::max(0, static_cast<int>(outputLogs_.size()) - maxLines);
        for (int i = start; i < static_cast<int>(outputLogs_.size()); ++i) {
            const std::string& ln = outputLogs_[static_cast<size_t>(i)];
            Color col = COL_TEXT_MUTED;
            if (ln.find("[boot]") == 0 || ln.find("[save]") == 0) col = COL_GREEN;
            else if (ln.find("[ai]") == 0) col = COL_ACCENT_VIOLET;
            else if (ln.find("[ui]") == 0) col = COL_CYAN;
            renderer_->drawText(ln, contentX, contentY, bottomPanelRect_.width - 32.0f, col, "mono", 12.0f);
            contentY += 18.0f;
        }
    } else if (activeBottomTab_ == 1) {
        // TERMINAL
        std::string user = Platform::PlatformAbstraction::getUserName();
        std::string host = Platform::PlatformAbstraction::getHostname();
        renderer_->drawText("NexusForge Integrated Terminal (Linux PTY)", contentX, contentY,
                            COL_TEXT_DIM, "mono", 11.5f);
        contentY += 20.0f;
        renderer_->drawText(user + "@" + host + ":" + workspaceRoot_ + " $ ", contentX, contentY,
                            COL_GREEN, "mono", 12.5f);
        float px = contentX + renderer_->measureText(user + "@" + host + ":" + workspaceRoot_ + " $ ", "mono", 12.5f).x;
        if (static_cast<int>(cursorBlinkTime_ * 2.0) % 2 == 0) {
            renderer_->drawRect(Rect(px, contentY, 7.0f, 14.0f), COL_TEXT_PRIMARY);
        }
        contentY += 24.0f;
        renderer_->drawText("Build: bash build.sh   |   Run: ./run.sh   |   Toggle Panel: Ctrl+`",
                            contentX, contentY, COL_TEXT_DIM, "mono", 11.5f);
    } else if (activeBottomTab_ == 2) {
        renderer_->drawText("No problems have been detected in the workspace.",
                            contentX, contentY, COL_GREEN, "default", 12.0f);
    } else {
        renderer_->drawText("Debug Console ready. Start a debug session from Run and Debug.",
                            contentX, contentY, COL_TEXT_MUTED, "mono", 12.0f);
    }

    renderer_->popClipRect();
}

void MainWindow::renderStatusBar() {
    renderer_->drawRect(statusBarRect_, COL_STATUSBAR);

    float y = statusBarRect_.y + 6.5f;
    float lx = statusBarRect_.x + 12.0f;

    // Branch + Problems + Live Notification
    std::string leftText = "git: " + gitBranch_ + "   |   0 Err  0 Warn   |   " + statusNotification_;
    renderer_->drawText(leftText, lx, y, Color::White(), "default", 11.5f);

    // Right status indicators
    TabInfo* active = getActiveTab();
    std::string cursorStr = (active && !active->isWelcome)
                                ? ("Ln " + std::to_string(active->cursorLine + 1) +
                                   ", Col " + std::to_string(active->cursorCol + 1))
                                : "Workspace";
    std::string langStr = active ? active->language : "Plain Text";
    int fps = (engine_ && engine_->getStats().fps > 1.0)
                  ? static_cast<int>(std::lround(std::min(120.0, engine_->getStats().fps)))
                  : 60;

    std::string rightText = cursorStr + "   |   UTF-8   |   " + langStr +
                            "   |   Phoenix AI   |   " + std::to_string(fps) + " FPS";
    Vec2 rsz = renderer_->measureText(rightText, "default", 11.5f);
    renderer_->drawText(rightText, statusBarRect_.right() - rsz.x - 14.0f, y,
                        Color(0.92f, 0.96f, 1.0f, 1.0f), "default", 11.5f);
}

void MainWindow::renderCommandPalette() {
    // Translucent backdrop
    renderer_->drawRect(Rect(0.0f, 0.0f, static_cast<float>(windowWidth_), static_cast<float>(windowHeight_)),
                        Color(0.02f, 0.03f, 0.06f, 0.55f));

    float modalW = std::min(520.0f, static_cast<float>(windowWidth_) - 40.0f);
    float itemH = 32.0f;
    float modalH = 48.0f + static_cast<float>(paletteCommands_.size()) * itemH + 10.0f;
    float mx = (static_cast<float>(windowWidth_) - modalW) * 0.5f;
    float my = 56.0f;

    Rect modal(mx, my, modalW, modalH);
    renderer_->drawRoundedRect(modal, Color(0.10f, 0.12f, 0.19f, 1.0f), 10.0f);
    renderer_->drawLine(mx + 10.0f, my + 40.0f, mx + modalW - 10.0f, my + 40.0f, COL_BORDER, 1.0f);
    renderer_->drawText("> Command Palette  (Up/Down + Enter, Esc to close)",
                        mx + 16.0f, my + 14.0f, COL_ACCENT, "default", 12.5f);

    float iy = my + 46.0f;
    for (size_t i = 0; i < paletteCommands_.size(); ++i) {
        Rect row(mx + 8.0f, iy, modalW - 16.0f, itemH - 2.0f);
        bool sel = (static_cast<int>(i) == commandPaletteSelected_) || row.contains(mouseX_, mouseY_);
        if (sel) {
            renderer_->drawRoundedRect(row, Color(0.20f, 0.36f, 0.72f, 0.55f), 5.0f);
        }
        renderer_->drawText(paletteCommands_[i].label, row.x + 12.0f, row.y + 8.0f,
                            COL_TEXT_PRIMARY, "default", 12.0f);
        if (!paletteCommands_[i].shortcut.empty()) {
            Vec2 ss = renderer_->measureText(paletteCommands_[i].shortcut, "default", 11.0f);
            renderer_->drawText(paletteCommands_[i].shortcut, row.right() - ss.x - 12.0f, row.y + 9.0f,
                                COL_TEXT_MUTED, "default", 11.0f);
        }
        iy += itemH;
    }
}

bool MainWindow::handleMouseClick(float x, float y, int button) {
    if (button != 0) return false;

    // Command palette modal clicks
    if (commandPaletteOpen_) {
        float modalW = std::min(520.0f, static_cast<float>(windowWidth_) - 40.0f);
        float itemH = 32.0f;
        float modalH = 48.0f + static_cast<float>(paletteCommands_.size()) * itemH + 10.0f;
        float mx = (static_cast<float>(windowWidth_) - modalW) * 0.5f;
        float my = 56.0f;
        Rect modal(mx, my, modalW, modalH);
        if (!modal.contains(x, y)) {
            commandPaletteOpen_ = false;
            return true;
        }
        float iy = my + 46.0f;
        for (size_t i = 0; i < paletteCommands_.size(); ++i) {
            Rect row(mx + 8.0f, iy, modalW - 16.0f, itemH - 2.0f);
            if (row.contains(x, y)) {
                auto act = paletteCommands_[i].action;
                commandPaletteOpen_ = false;
                if (act) act();
                return true;
            }
            iy += itemH;
        }
        return true;
    }

    // Top search pill -> Command Palette
    if (searchPillRect_.contains(x, y)) {
        showCommandPalette();
        return true;
    }

    // Top-right Sidebar / Panel / AI buttons
    if (titleBarRect_.contains(x, y)) {
        float rx = titleBarRect_.right() - 16.0f;
        Rect panelBtn(rx - 56.0f, 7.0f, 56.0f, 22.0f);
        Rect sideBtn(panelBtn.x - 68.0f, 7.0f, 62.0f, 22.0f);
        Rect aiPill(sideBtn.x - 86.0f, 7.0f, 78.0f, 22.0f);
        if (panelBtn.contains(x, y)) { setPanelVisible(!panelVisible_); return true; }
        if (sideBtn.contains(x, y))  { setSidebarVisible(!sidebarVisible_); return true; }
        if (aiPill.contains(x, y))   { showPanel(PanelType::AIChat); return true; }
    }

    // Activity Bar icons
    if (activityBarRect_.contains(x, y)) {
        int idx = static_cast<int>((y - (activityBarRect_.y + 8.0f)) / 48.0f);
        if (idx >= 0 && idx < static_cast<int>(activityItems_.size())) {
            PanelType clicked = activityItems_[static_cast<size_t>(idx)].panelType;
            if (sidebarVisible_ && activePanelType_ == clicked) {
                setSidebarVisible(false);
            } else {
                showPanel(clicked);
            }
            return true;
        }
    }

    // Explorer file/folder clicks
    if (sidebarVisible_ && activePanelType_ == PanelType::Explorer && sidebarRect_.contains(x, y)) {
        float listTop = sidebarRect_.y + 56.0f;
        if (y >= listTop) {
            int idx = explorerScroll_ + static_cast<int>((y - listTop) / 24.0f);
            if (idx >= 0 && idx < static_cast<int>(explorerEntries_.size())) {
                const auto entry = explorerEntries_[static_cast<size_t>(idx)];
                if (entry.isDirectory) {
                    toggleExplorerFolder(static_cast<size_t>(idx));
                } else {
                    openFile(entry.path);
                }
                return true;
            }
        }
    }

    // Tab bar clicks (activate tab, close 'x', or '+' new file)
    if (tabBarRect_.contains(x, y)) {
        float tx = tabBarRect_.x;
        for (const auto& tab : tabs_) {
            std::string label = tab.modified ? ("* " + tab.title) : tab.title;
            Vec2 textSz = renderer_->measureText(label, "default", 12.0f);
            float tabW = std::clamp(textSz.x + 56.0f, 108.0f, 220.0f);
            Rect r(tx, tabBarRect_.y, tabW, tabBarRect_.height);
            if (r.contains(x, y)) {
                Rect closeR(r.right() - 22.0f, r.y + 9.0f, 16.0f, 16.0f);
                std::string id = tab.id;
                if (closeR.contains(x, y)) {
                    closeTab(id);
                } else {
                    setActiveTab(id);
                }
                return true;
            }
            tx += tabW;
        }
        Rect plusR(tx + 6.0f, tabBarRect_.y + 6.0f, 22.0f, 22.0f);
        if (plusR.contains(x, y)) {
            newFile("C++");
            return true;
        }
    }

    // Bottom panel tab clicks & close button
    if (panelVisible_ && bottomPanelRect_.contains(x, y)) {
        Rect closeR(bottomPanelRect_.right() - 28.0f, bottomPanelRect_.y + 6.0f, 18.0f, 18.0f);
        if (closeR.contains(x, y)) {
            hideBottomPanel();
            return true;
        }
        if (y <= bottomPanelRect_.y + 30.0f) {
            static const char* panelTabs[] = {"OUTPUT", "TERMINAL", "PROBLEMS", "DEBUG CONSOLE"};
            float tx = bottomPanelRect_.x + 14.0f;
            for (int i = 0; i < 4; ++i) {
                Vec2 sz = renderer_->measureText(panelTabs[i], "default", 11.0f);
                if (x >= tx - 4.0f && x <= tx + sz.x + 6.0f) {
                    activeBottomTab_ = i;
                    return true;
                }
                tx += sz.x + 22.0f;
            }
        }
    }

    // Editor / Welcome area clicks
    if (editorRect_.contains(x, y)) {
        TabInfo* active = getActiveTab();
        if (!active || active->isWelcome) {
            float padX = editorRect_.x + 44.0f;
            float baseY = editorRect_.y + 32.0f + 84.0f;
            float col1W = std::min(340.0f, (editorRect_.width - 120.0f) * 0.48f);
            float ay = baseY + 28.0f;
            for (int i = 0; i < 4; ++i) {
                Rect card(padX, ay, col1W, 48.0f);
                if (card.contains(x, y)) {
                    if (i == 0) newFile("C++");
                    else if (i == 1 && Platform::PlatformAbstraction::fileExists("main.cpp")) openFile("main.cpp");
                    else if (i == 1) newFile("C++");
                    else if (i == 2) setPanelVisible(!panelVisible_);
                    else if (i == 3) showCommandPalette();
                    return true;
                }
                ay += 58.0f;
            }

            float col2X = padX + col1W + 36.0f;
            float col2W = std::max(200.0f, editorRect_.right() - col2X - 40.0f);
            float fy = baseY + 28.0f;
            int shown = 0;
            for (const auto& entry : explorerEntries_) {
                if (entry.isDirectory || shown >= 6) continue;
                Rect row(col2X, fy, col2W, 30.0f);
                if (row.contains(x, y)) {
                    openFile(entry.path);
                    return true;
                }
                fy += 34.0f;
                ++shown;
            }
        } else {
            // Place text cursor at clicked line/column
            int clickedLine = active->scrollLine + static_cast<int>((y - (editorRect_.y + 6.0f)) / 20.0f);
            active->cursorLine = std::clamp(clickedLine, 0, std::max(0, static_cast<int>(active->lines.size()) - 1));
            float codeX = editorRect_.x + 64.0f;
            float charW = std::max(1.0f, renderer_->measureText("M", "mono", 13.0f).x);
            int col = static_cast<int>(std::lround((x - codeX) / charW));
            active->cursorCol = std::clamp(col, 0, static_cast<int>(active->lines[static_cast<size_t>(active->cursorLine)].size()));
            cursorBlinkTime_ = 0.0;
            return true;
        }
    }

    return false;
}

bool MainWindow::handleMouseScroll(float x, float y, double scrollY) {
    int delta = (scrollY > 0.0) ? -3 : 3;
    if (sidebarVisible_ && sidebarRect_.contains(x, y)) {
        int maxS = std::max(0, static_cast<int>(explorerEntries_.size()) - 10);
        explorerScroll_ = std::clamp(explorerScroll_ + delta, 0, maxS);
        return true;
    }
    if (editorRect_.contains(x, y)) {
        TabInfo* active = getActiveTab();
        if (active && !active->isWelcome) {
            int maxS = std::max(0, static_cast<int>(active->lines.size()) - 5);
            active->scrollLine = std::clamp(active->scrollLine + delta, 0, maxS);
            return true;
        }
    }
    return false;
}

bool MainWindow::handleEditorKeyPress(const Core::KeyPressEvent& event) {
    TabInfo* tab = getActiveTab();
    if (!tab || tab->isWelcome) return false;
    if (tab->lines.empty()) tab->lines.push_back("");

    cursorBlinkTime_ = 0.0;
    auto ensureVisible = [&]() {
        int vis = std::max(1, static_cast<int>(editorRect_.height / 20.0f) - 1);
        if (tab->cursorLine < tab->scrollLine) tab->scrollLine = tab->cursorLine;
        if (tab->cursorLine >= tab->scrollLine + vis) tab->scrollLine = tab->cursorLine - vis + 1;
    };

    switch (event.keyCode) {
        case Platform::KeyCode::Up:
            if (tab->cursorLine > 0) --tab->cursorLine;
            tab->cursorCol = std::min(tab->cursorCol, static_cast<int>(tab->lines[static_cast<size_t>(tab->cursorLine)].size()));
            ensureVisible();
            return true;
        case Platform::KeyCode::Down:
            if (tab->cursorLine + 1 < static_cast<int>(tab->lines.size())) ++tab->cursorLine;
            tab->cursorCol = std::min(tab->cursorCol, static_cast<int>(tab->lines[static_cast<size_t>(tab->cursorLine)].size()));
            ensureVisible();
            return true;
        case Platform::KeyCode::Left:
            if (tab->cursorCol > 0) --tab->cursorCol;
            else if (tab->cursorLine > 0) {
                --tab->cursorLine;
                tab->cursorCol = static_cast<int>(tab->lines[static_cast<size_t>(tab->cursorLine)].size());
            }
            ensureVisible();
            return true;
        case Platform::KeyCode::Right:
            if (tab->cursorCol < static_cast<int>(tab->lines[static_cast<size_t>(tab->cursorLine)].size())) {
                ++tab->cursorCol;
            } else if (tab->cursorLine + 1 < static_cast<int>(tab->lines.size())) {
                ++tab->cursorLine;
                tab->cursorCol = 0;
            }
            ensureVisible();
            return true;
        case Platform::KeyCode::Home:
            tab->cursorCol = 0;
            return true;
        case Platform::KeyCode::End:
            tab->cursorCol = static_cast<int>(tab->lines[static_cast<size_t>(tab->cursorLine)].size());
            return true;
        case Platform::KeyCode::PageUp:
            tab->cursorLine = std::max(0, tab->cursorLine - 15);
            tab->cursorCol = std::min(tab->cursorCol, static_cast<int>(tab->lines[static_cast<size_t>(tab->cursorLine)].size()));
            ensureVisible();
            return true;
        case Platform::KeyCode::PageDown:
            tab->cursorLine = std::min(static_cast<int>(tab->lines.size()) - 1, tab->cursorLine + 15);
            tab->cursorCol = std::min(tab->cursorCol, static_cast<int>(tab->lines[static_cast<size_t>(tab->cursorLine)].size()));
            ensureVisible();
            return true;
        case Platform::KeyCode::Enter: {
            std::string& cur = tab->lines[static_cast<size_t>(tab->cursorLine)];
            std::string tail = cur.substr(static_cast<size_t>(tab->cursorCol));
            cur.erase(static_cast<size_t>(tab->cursorCol));
            tab->lines.insert(tab->lines.begin() + tab->cursorLine + 1, std::move(tail));
            ++tab->cursorLine;
            tab->cursorCol = 0;
            tab->modified = true;
            updateWindowTitle();
            ensureVisible();
            return true;
        }
        case Platform::KeyCode::Tab: {
            std::string& cur = tab->lines[static_cast<size_t>(tab->cursorLine)];
            cur.insert(static_cast<size_t>(tab->cursorCol), "    ");
            tab->cursorCol += 4;
            tab->modified = true;
            updateWindowTitle();
            return true;
        }
        case Platform::KeyCode::Backspace: {
            std::string& cur = tab->lines[static_cast<size_t>(tab->cursorLine)];
            if (tab->cursorCol > 0) {
                cur.erase(static_cast<size_t>(tab->cursorCol - 1), 1);
                --tab->cursorCol;
                tab->modified = true;
                updateWindowTitle();
            } else if (tab->cursorLine > 0) {
                std::string moved = std::move(cur);
                tab->lines.erase(tab->lines.begin() + tab->cursorLine);
                --tab->cursorLine;
                tab->cursorCol = static_cast<int>(tab->lines[static_cast<size_t>(tab->cursorLine)].size());
                tab->lines[static_cast<size_t>(tab->cursorLine)] += moved;
                tab->modified = true;
                updateWindowTitle();
            }
            ensureVisible();
            return true;
        }
        case Platform::KeyCode::Delete: {
            std::string& cur = tab->lines[static_cast<size_t>(tab->cursorLine)];
            if (tab->cursorCol < static_cast<int>(cur.size())) {
                cur.erase(static_cast<size_t>(tab->cursorCol), 1);
                tab->modified = true;
                updateWindowTitle();
            } else if (tab->cursorLine + 1 < static_cast<int>(tab->lines.size())) {
                cur += tab->lines[static_cast<size_t>(tab->cursorLine + 1)];
                tab->lines.erase(tab->lines.begin() + tab->cursorLine + 1);
                tab->modified = true;
                updateWindowTitle();
            }
            return true;
        }
        default:
            return false;
    }
}

bool MainWindow::handleEditorCharInput(uint32_t codepoint) {
    if (codepoint < 32 || codepoint >= 127) return false;
    TabInfo* tab = getActiveTab();
    if (!tab || tab->isWelcome) return false;
    if (tab->lines.empty()) tab->lines.push_back("");

    std::string& cur = tab->lines[static_cast<size_t>(tab->cursorLine)];
    cur.insert(static_cast<size_t>(tab->cursorCol), 1, static_cast<char>(codepoint));
    ++tab->cursorCol;
    tab->modified = true;
    cursorBlinkTime_ = 0.0;
    updateWindowTitle();
    return true;
}

void MainWindow::setupKeyboardShortcuts() {}

bool MainWindow::handleKeyboardShortcut(const Core::KeyPressEvent& event) {
    bool ctrl = (event.modifiers & 0x0002) != 0;
    bool shift = (event.modifiers & 0x0001) != 0;

    if (ctrl && shift) {
        switch (event.keyCode) {
            case 'P':
                showCommandPalette();
                return true;
            case 'O':
                showGoToSymbol();
                return true;
        }
    }

    if (ctrl) {
        switch (event.keyCode) {
            case 'N':
                newFile("C++");
                return true;
            case 'O':
                showQuickOpen();
                return true;
            case 'S':
                saveActiveFile();
                return true;
            case 'P':
                showCommandPalette();
                return true;
            case '`':
                if (panelVisible_) hideBottomPanel();
                else showTerminal();
                return true;
            case 'B':
                setSidebarVisible(!sidebarVisible_);
                return true;
            case 'W':
                if (!activeTabId_.empty()) closeTab(activeTabId_);
                return true;
            case 'Q':
                if (onClose) onClose();
                return true;
        }
    }

    return false;
}

} // namespace NexusForge::UI
