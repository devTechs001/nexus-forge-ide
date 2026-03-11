// ui/screens/main_window.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include "../widgets/code_editor.hpp"
#include "../mobile/adaptive_layout.hpp"
#include "../../core/engine/nexus_core.hpp"
#include "../../config/settings_manager.hpp"

#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace NexusForge::UI {

// Panel types
enum class PanelType {
    Explorer,
    Search,
    SourceControl,
    Debug,
    Extensions,
    AIChat,
    Terminal,
    Problems,
    Output,
    Custom
};

// Tab info
struct TabInfo {
    std::string id;
    std::string title;
    std::string filePath;
    std::string language;
    bool modified = false;
    bool pinned = false;
    Widget::Ptr content;
};

// Activity bar item
struct ActivityBarItem {
    std::string id;
    std::string icon;
    std::string tooltip;
    PanelType panelType;
    int badgeCount = 0;
    bool active = false;
};

// Status bar item
struct StatusBarItem {
    std::string id;
    std::string text;
    std::string tooltip;
    std::string icon;
    std::string alignment;  // left, right
    int priority = 0;
    std::function<void()> onClick;
};

// Main window class
class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool initialize(Core::NexusEngine* engine, Config::SettingsManager* settings);
    void show();
    void close();

    // Window management
    void setTitle(const std::string& title);
    void setSize(int width, int height);
    void setPosition(int x, int y);
    void maximize();
    void minimize();
    void restore();
    void toggleFullscreen();
    bool isMaximized() const;
    bool isFullscreen() const;

    // Layout
    void setLayoutMode(Mobile::FormFactor formFactor);
    void setSidebarVisible(bool visible);
    void setPanelVisible(bool visible);
    void setSidebarWidth(float width);
    void setPanelHeight(float height);

    bool isSidebarVisible() const { return sidebarVisible_; }
    bool isPanelVisible() const { return panelVisible_; }

    // Activity bar
    void setActivityBarItems(const std::vector<ActivityBarItem>& items);
    void setActiveActivityItem(const std::string& id);
    void updateActivityBadge(const std::string& id, int count);

    // Sidebar
    void showPanel(PanelType type);
    void hidePanel();
    void addCustomPanel(const std::string& id, const std::string& title, Widget::Ptr content);
    void removeCustomPanel(const std::string& id);

    // Tabs / Editors
    void openFile(const std::string& path);
    void openFiles(const std::vector<std::string>& paths);
    void newFile(const std::string& language = "");
    void closeTab(const std::string& id);
    void closeAllTabs();
    void closeOtherTabs(const std::string& keepId);
    void setActiveTab(const std::string& id);
    void nextTab();
    void previousTab();
    void moveTab(const std::string& id, int newIndex);
    void pinTab(const std::string& id);
    void unpinTab(const std::string& id);

    const std::vector<TabInfo>& getTabs() const { return tabs_; }
    TabInfo* getActiveTab();
    CodeEditor* getActiveEditor();

    // Split views
    void splitHorizontal();
    void splitVertical();
    void closeSplit();
    void focusNextSplit();
    void focusPreviousSplit();

    // Bottom panel
    void showTerminal();
    void showProblems();
    void showOutput();
    void showDebugConsole();
    void hideBottomPanel();

    // Status bar
    void setStatusBarItems(const std::vector<StatusBarItem>& items);
    void updateStatusBarItem(const std::string& id, const std::string& text);
    void addStatusBarItem(const StatusBarItem& item);
    void removeStatusBarItem(const std::string& id);

    // Command palette
    void showCommandPalette();
    void showQuickOpen();
    void showGoToLine();
    void showGoToSymbol();

    // Notifications
    void showNotification(const std::string& message,
                          const std::string& type = "info",  // info, warning, error
                          int durationMs = 5000);
    void showProgressNotification(const std::string& title,
                                   std::function<void(std::function<void(float)>)> task);

    // Dialogs
    std::string showOpenDialog(const std::string& title = "Open File",
                                const std::vector<std::string>& filters = {});
    std::vector<std::string> showOpenMultipleDialog(const std::string& title = "Open Files",
                                                     const std::vector<std::string>& filters = {});
    std::string showSaveDialog(const std::string& title = "Save File",
                                const std::string& defaultName = "",
                                const std::vector<std::string>& filters = {});
    std::string showFolderDialog(const std::string& title = "Open Folder");
    int showMessageBox(const std::string& title, const std::string& message,
                       const std::vector<std::string>& buttons = {"OK"});

    // Update loop
    void update(double deltaTime);
    void render();

    // Event handling
    bool handleEvent(Core::Event& event);

    // Callbacks
    std::function<void()> onClose;
    std::function<void(int, int)> onResize;
    std::function<void(const std::string&)> onFileOpened;
    std::function<void(const std::string&)> onFileSaved;
    std::function<void(const std::string&)> onFileClosed;
    std::function<void(const std::string&)> onActiveTabChanged;

private:
    Core::NexusEngine* engine_ = nullptr;
    Config::SettingsManager* settings_ = nullptr;

    std::unique_ptr<RenderEngine> renderer_;
    std::unique_ptr<Mobile::GestureHandler> gestureHandler_;

    // Window state
    void* nativeWindow_ = nullptr;
    int windowWidth_ = 1280;
    int windowHeight_ = 720;
    bool maximized_ = false;
    bool fullscreen_ = false;
    Mobile::FormFactor currentFormFactor_ = Mobile::FormFactor::Desktop;

    // Layout components
    Widget::Ptr rootWidget_;
    Widget::Ptr titleBar_;
    Widget::Ptr activityBar_;
    Widget::Ptr sidebar_;
    Widget::Ptr editorArea_;
    Widget::Ptr bottomPanel_;
    Widget::Ptr statusBar_;

    // Overlays
    Widget::Ptr commandPalette_;
    Widget::Ptr quickOpen_;
    Widget::Ptr notifications_;
    Widget::Ptr contextMenu_;

    // Mobile-specific
    Widget::Ptr mobileToolbar_;
    Widget::Ptr mobileBottomNav_;
    Widget::Ptr mobileDrawer_;

    // State
    std::vector<ActivityBarItem> activityItems_;
    std::string activeActivityId_;
    PanelType activePanelType_ = PanelType::Explorer;

    std::vector<TabInfo> tabs_;
    std::string activeTabId_;

    std::vector<StatusBarItem> statusBarItems_;

    bool sidebarVisible_ = true;
    bool panelVisible_ = false;
    float sidebarWidth_ = 250;
    float panelHeight_ = 200;

    // Initialization helpers
    void createTitleBar();
    void createActivityBar();
    void createSidebar();
    void createEditorArea();
    void createBottomPanel();
    void createStatusBar();
    void createMobileLayout();
    void createOverlays();

    // Layout helpers
    void layoutDesktop();
    void layoutTablet();
    void layoutPhone();
    void updateLayout();

    // Tab management
    std::string generateTabId();
    void updateTabTitle(const std::string& id);
    void updateWindowTitle();

    // Keyboard shortcuts
    void setupKeyboardShortcuts();
    bool handleKeyboardShortcut(const Core::KeyPressEvent& event);
};

} // namespace NexusForge::UI
