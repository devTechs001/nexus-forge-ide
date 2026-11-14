// core/platform/platform_ios.cpp
// iOS-specific platform implementation

#include "platform_abstraction.hpp"

#ifdef NEXUS_PLATFORM_IOS

#ifdef __OBJC__
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#endif
#include <unistd.h>
#include <sys/stat.h>

namespace NexusForge::Platform {

// Window management implementations
void* PlatformAbstraction::createMainWindow(const std::string& title, int width, int height) { return nullptr; }
void* PlatformAbstraction::createSplashWindow(int width, int height) { return nullptr; }
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

#endif // NEXUS_PLATFORM_IOS
