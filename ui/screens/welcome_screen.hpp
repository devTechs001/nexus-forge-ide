// ui/screens/welcome_screen.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include "../renderer/render_engine.hpp"
#include "../../core/engine/nexus_core.hpp"

#include <vector>
#include <string>
#include <functional>

namespace NexusForge::UI {

// Recent file entry
struct RecentFile {
    std::string path;
    std::string name;
    std::string time;
    bool hovered = false;
};

// Recent workspace entry
struct RecentWorkspace {
    std::string name;
    std::string path;
    std::string type;
    bool hovered = false;
};

// Help link
struct HelpLink {
    std::string icon;
    std::string text;
    std::string url;
};

// Welcome screen class
class WelcomeScreen {
public:
    WelcomeScreen();
    ~WelcomeScreen();

    bool initialize(Core::NexusEngine* engine);
    void show();
    void hide();

    // Data loading
    void loadRecentFiles();
    void loadWorkspaceInfo();

    // Render
    void render(RenderEngine& renderer);

    // Event handling
    bool onMouseDown(const Core::MouseButtonPressEvent& event);
    bool onMouseMove(const Core::MouseMoveEvent& event);

    // Callbacks
    std::function<void(const std::string&)> onFileSelected;
    std::function<void(const std::string&)> onWorkspaceSelected;
    std::function<void()> onNewFile;
    std::function<void()> onOpenFile;
    std::function<void()> onCloneRepository;

private:
    Core::NexusEngine* engine_ = nullptr;
    bool visible_ = false;

    std::vector<RecentFile> recentFiles_;
    std::vector<RecentWorkspace> recentWorkspaces_;
    std::vector<HelpLink> helpLinks_;

    // Tip of the day
    std::string currentTip_;
    int tipIndex_ = 0;

    // Render helpers
    void drawHeader(RenderEngine& renderer, const Rect& bounds);
    void drawContent(RenderEngine& renderer, const Rect& bounds);
    void drawFooter(RenderEngine& renderer, const Rect& bounds);

    void drawActionButton(RenderEngine& renderer, float x, float y,
                          const std::string& icon, const std::string& title,
                          const std::string& description);
    void drawHelpLink(RenderEngine& renderer, float x, float y,
                      const std::string& icon, const std::string& text);

    // Utilities
    std::string getFileIcon(const std::string& path) const;
    std::string truncatePath(const std::string& path, size_t maxLen) const;
    void loadTips();
};

} // namespace NexusForge::UI
