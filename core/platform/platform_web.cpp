// core/platform/platform_web.cpp
// Web/Emscripten-specific platform implementation

#include "platform_abstraction.hpp"

#ifdef NEXUS_PLATFORM_WEB

#include <emscripten.h>
#include <emscripten/html5.h>

namespace NexusForge::Platform {

// Web-specific implementations would go here
// This file provides Emscripten/Web-specific overrides for the platform abstraction

// JavaScript interop functions
extern "C" {
    // Called from JavaScript to notify of keyboard events
    void nexus_on_keydown(int keyCode, int modifiers);
    void nexus_on_keyup(int keyCode, int modifiers);
    
    // Called from JavaScript to notify of mouse events
    void nexus_on_mousedown(int button, double x, double y);
    void nexus_on_mouseup(int button, double x, double y);
    void nexus_on_mousemove(double x, double y);
    
    // Called from JavaScript to notify of touch events
    void nexus_on_touchstart(int id, double x, double y, double pressure);
    void nexus_on_touchmove(int id, double x, double y, double pressure);
    void nexus_on_touchend(int id, double x, double y);
    
    // Called from JavaScript to notify of resize
    void nexus_on_resize(int width, int height);
    
    // Called from JavaScript to get clipboard text
    const char* nexus_get_clipboard_text();
    
    // Called from JavaScript to set clipboard text
    void nexus_set_clipboard_text(const char* text);
}

// Window management implementations for Web
void* PlatformAbstraction::createMainWindow(const std::string& title, int width, int height) {
    return reinterpret_cast<void*>(1);
}
void* PlatformAbstraction::createSplashWindow(int width, int height) { return reinterpret_cast<void*>(2); }
void PlatformAbstraction::destroyWindow(void* window) {}
void PlatformAbstraction::showWindow(void* window) {}
void PlatformAbstraction::hideWindow(void* window) {}
void PlatformAbstraction::setWindowTitle(void* window, const char* title) {}
void PlatformAbstraction::setWindowSize(void* window, int width, int height) {}
void PlatformAbstraction::setWindowPosition(void* window, int x, int y) {}
void PlatformAbstraction::maximizeWindow(void* window) {}
void PlatformAbstraction::minimizeWindow(void* window) {}
void PlatformAbstraction::restoreWindow(void* window) {}
void PlatformAbstraction::setWindowFullscreen(void* window, bool fullscreen) {}
bool PlatformAbstraction::isWindowFullscreen(void* window) { return false; }
void* PlatformAbstraction::getNativeWindowHandle(void* window) { return window; }

} // namespace NexusForge::Platform

#endif // NEXUS_PLATFORM_WEB
