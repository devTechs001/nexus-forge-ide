// ui/screens/welcome_screen.cpp
// Welcome Screen Implementation

#include "welcome_screen.hpp"
#include "../widgets/button.hpp"
#include <chrono>
#include <ctime>
#include <cmath>

namespace NexusForge::UI {

WelcomeScreen::WelcomeScreen() = default;

WelcomeScreen::~WelcomeScreen() = default;

bool WelcomeScreen::initialize(Core::NexusEngine* engine) {
    engine_ = engine;
    loadRecentFiles();
    loadWorkspaceInfo();
    return true;
}

void WelcomeScreen::show() {
    visible_ = true;
    loadRecentFiles();
    loadWorkspaceInfo();
}

void WelcomeScreen::hide() {
    visible_ = false;
}

void WelcomeScreen::loadRecentFiles() {
    recentFiles_ = {
        {"~/projects/myapp/src/main.cpp", "main.cpp", "2 hours ago"},
        {"~/projects/myapp/src/utils.cpp", "utils.cpp", "3 hours ago"},
        {"~/projects/webapp/index.html", "index.html", "Yesterday"},
        {"~/projects/webapp/styles.css", "styles.css", "Yesterday"},
        {"~/projects/config.json", "config.json", "2 days ago"},
    };
}

void WelcomeScreen::loadWorkspaceInfo() {
    recentWorkspaces_ = {
        {"myapp", "~/projects/myapp", "C++"},
        {"webapp", "~/projects/webapp", "Web"},
        {"config", "~/projects", "Config"},
    };
}

void WelcomeScreen::render(RenderEngine& renderer) {
    if (!visible_) return;

    // Use full screen bounds
    Rect bounds(0, 0, 1280, 720);
    
    // Draw background gradient
    renderer.drawGradientRect(bounds, 
                             Color(0.1f, 0.1f, 0.14f),
                             Color(0.08f, 0.08f, 0.1f));
    
    // Draw header
    drawHeader(renderer, bounds);
    
    // Draw main content
    drawContent(renderer, bounds);
    
    // Draw footer
    drawFooter(renderer, bounds);
}

void WelcomeScreen::drawHeader(RenderEngine& renderer, const Rect& bounds) {
    // Logo area
    float logoSize = 80;
    float logoX = bounds.x + 40;
    float logoY = bounds.y + 40;
    
    // Draw stylized logo (hexagon with NF)
    renderer.beginPath();
    for (int i = 0; i < 6; ++i) {
        float angle = i * 3.14159f / 3.0f - 3.14159f / 6.0f;
        float x = logoX + logoSize/2 + std::cos(angle) * logoSize/2;
        float y = logoY + logoSize/2 + std::sin(angle) * logoSize/2;
        if (i == 0) renderer.moveTo(x, y);
        else renderer.lineTo(x, y);
    }
    renderer.closePath();
    renderer.fillPath(Color(0.4f, 0.6f, 1.0f));
    
    // NF text
    renderer.drawText("NF", logoX + 20, logoY + 50, Color(1, 1, 1), "default", 32);
    
    // App name
    renderer.drawText("NEXUSFORGE IDE", bounds.x + 140, bounds.y + 55,
                     Color(1, 1, 1), "default", 28);
    
    // Version
    renderer.drawText("Version 1.0.0 - Phoenix", bounds.x + 140, bounds.y + 85,
                     Color(0.5f, 0.5f, 0.55f), "default", 12);
    
    // Greeting based on time (thread-safe)
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuf{};
    bool hasTime = false;
#ifdef _WIN32
    hasTime = (localtime_s(&tmBuf, &time) == 0);
#else
    hasTime = (localtime_r(&time, &tmBuf) != nullptr);
#endif

    std::string greeting = "Hello";
    if (hasTime) {
        if (tmBuf.tm_hour < 12) {
            greeting = "Good morning";
        } else if (tmBuf.tm_hour < 18) {
            greeting = "Good afternoon";
        } else {
            greeting = "Good evening";
        }
    }
    
    renderer.drawText(greeting + ", Developer", bounds.x + 40, bounds.y + 120,
                     Color(0.7f, 0.7f, 0.75f), "default", 18);
}

void WelcomeScreen::drawContent(RenderEngine& renderer, const Rect& bounds) {
    float contentX = bounds.x + 40;
    float contentY = bounds.y + 160;
    
    // Start section
    renderer.drawText("Start", contentX, contentY, Color(0.9f, 0.9f, 0.9f), "default", 16);
    contentY += 30;
    
    // New file button
    drawActionButton(renderer, contentX, contentY, "📄", "New File", "Create a new file");
    contentY += 50;
    
    // Open file button
    drawActionButton(renderer, contentX, contentY, "📁", "Open File", "Open a file or folder");
    contentY += 50;
    
    // Clone repository button
    drawActionButton(renderer, contentX, contentY, "🔀", "Clone Repository", "Clone from GitHub");
    contentY += 60;
    
    // Recent files section
    renderer.drawText("Recent Files", contentX, contentY, Color(0.9f, 0.9f, 0.9f), "default", 16);
    contentY += 30;
    
    for (const auto& file : recentFiles_) {
        Rect fileRect(contentX, contentY, 400, 30);
        
        // Hover effect
        if (file.hovered) {
            renderer.drawRect(fileRect, Color(0.2f, 0.2f, 0.25f));
        }
        
        // File icon
        std::string icon = getFileIcon(file.path);
        renderer.drawText(icon, contentX + 10, contentY + 20, Color(0.6f, 0.6f, 0.65f), "default", 14);
        
        // File name
        renderer.drawText(file.name, contentX + 35, contentY + 20, Color(0.9f, 0.9f, 0.9f), "default", 14);
        
        // Path (truncated)
        std::string truncatedPath = truncatePath(file.path, 250);
        renderer.drawText(truncatedPath, contentX + 200, contentY + 20, Color(0.5f, 0.5f, 0.55f), "default", 12);
        
        // Time
        renderer.drawText(file.time, contentX + 460, contentY + 20, Color(0.4f, 0.4f, 0.45f), "default", 12);
        
        contentY += 35;
    }
    
    // Recent workspaces section (right side)
    float workspaceX = bounds.x + 500;
    float workspaceY = bounds.y + 160;
    
    renderer.drawText("Recent Workspaces", workspaceX, workspaceY, Color(0.9f, 0.9f, 0.9f), "default", 16);
    workspaceY += 30;
    
    for (const auto& ws : recentWorkspaces_) {
        Rect wsRect(workspaceX, workspaceY, 300, 40);
        
        if (ws.hovered) {
            renderer.drawRect(wsRect, Color(0.2f, 0.2f, 0.25f));
        }
        
        // Workspace icon
        renderer.drawText("📁", workspaceX + 10, workspaceY + 25, Color(0.6f, 0.6f, 0.65f), "default", 16);
        
        // Name
        renderer.drawText(ws.name, workspaceX + 35, workspaceY + 25, Color(0.9f, 0.9f, 0.9f), "default", 14);
        
        // Type badge
        Rect badgeRect(workspaceX + 150, workspaceY + 10, 50, 20);
        renderer.drawRoundedRect(badgeRect, Color(0.2f, 0.4f, 0.7f, 0.3f), 4);
        renderer.drawText(ws.type, workspaceX + 160, workspaceY + 23, Color(0.4f, 0.6f, 1.0f), "default", 11);
        
        workspaceY += 50;
    }
    
    // Help section
    float helpY = bounds.y + 450;
    renderer.drawText("Help", bounds.x + 40, helpY, Color(0.9f, 0.9f, 0.9f), "default", 16);
    helpY += 30;
    
    drawHelpLink(renderer, bounds.x + 40, helpY, "📖", "Documentation");
    drawHelpLink(renderer, bounds.x + 150, helpY, "💬", "Community");
    drawHelpLink(renderer, bounds.x + 260, helpY, "🐛", "Report Issue");
    drawHelpLink(renderer, bounds.x + 370, helpY, "⭐", "Star on GitHub");
}

void WelcomeScreen::drawFooter(RenderEngine& renderer, const Rect& bounds) {
    // Tips section
    Rect footerRect(bounds.x, bounds.y + bounds.height - 80, bounds.width, 80);
    renderer.drawRect(footerRect, Color(0.08f, 0.08f, 0.1f));
    
    renderer.drawText("💡 Tip of the day", bounds.x + 40, bounds.y + bounds.height - 60,
                     Color(0.6f, 0.6f, 0.65f), "default", 12);
    
    std::string tip = "Press Ctrl+P to quickly open files, or Ctrl+Shift+P for the command palette.";
    renderer.drawText(tip, bounds.x + 40, bounds.y + bounds.height - 35,
                     Color(0.5f, 0.5f, 0.55f), "default", 12);
}

void WelcomeScreen::drawActionButton(RenderEngine& renderer, float x, float y,
                                      const std::string& icon, const std::string& title,
                                      const std::string& description) {
    Rect buttonRect(x, y, 200, 40);
    
    // Button background
    renderer.drawRoundedRect(buttonRect, Color(0.2f, 0.2f, 0.25f), 6);
    
    // Icon
    renderer.drawText(icon, x + 15, y + 26, Color(0.4f, 0.6f, 1.0f), "default", 18);
    
    // Title
    renderer.drawText(title, x + 45, y + 25, Color(0.9f, 0.9f, 0.9f), "default", 14);
}

void WelcomeScreen::drawHelpLink(RenderEngine& renderer, float x, float y,
                                  const std::string& icon, const std::string& text) {
    renderer.drawText(icon + " " + text, x, y, Color(0.4f, 0.6f, 1.0f), "default", 12);
}

std::string WelcomeScreen::getFileIcon(const std::string& path) const {
    if (path.ends_with(".cpp") || path.ends_with(".hpp") || path.ends_with(".c") || path.ends_with(".h")) {
        return "🔷";
    } else if (path.ends_with(".py")) {
        return "🐍";
    } else if (path.ends_with(".js") || path.ends_with(".ts")) {
        return "📜";
    } else if (path.ends_with(".html")) {
        return "🌐";
    } else if (path.ends_with(".css")) {
        return "🎨";
    } else if (path.ends_with(".json")) {
        return "📋";
    } else if (path.ends_with(".md")) {
        return "📝";
    } else if (path.ends_with(".rs")) {
        return "🦀";
    } else if (path.ends_with(".go")) {
        return "🔹";
    }
    return "📄";
}

std::string WelcomeScreen::truncatePath(const std::string& path, size_t maxLen) const {
    if (path.length() <= maxLen) return path;
    if (maxLen <= 3) return path.substr(0, maxLen);

    size_t remaining = maxLen - 3;
    size_t startLen = remaining / 2;
    size_t endLen = remaining - startLen;

    return path.substr(0, startLen) + "..." + path.substr(path.length() - endLen);
}

void WelcomeScreen::loadTips() {
    currentTip_ = "Press Ctrl+P to quickly open files, or Ctrl+Shift+P for the command palette.";
}

bool WelcomeScreen::onMouseDown(const Core::MouseButtonPressEvent& event) {
    if (!visible_) return false;
    
    // Handle button clicks
    // In production, use proper hit testing
    
    return false;
}

bool WelcomeScreen::onMouseMove(const Core::MouseMoveEvent& event) {
    if (!visible_) return false;
    
    // Update hover states
    // In production, use proper hit testing
    
    return false;
}

} // namespace NexusForge::UI
