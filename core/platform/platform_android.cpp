// core/platform/platform_android.cpp
// Android-specific platform implementation

#include "platform_abstraction.hpp"

#ifdef NEXUS_PLATFORM_ANDROID

#include <android/native_activity.h>
#include <android/input.h>
#include <android/keycodes.h>
#include <jni.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <unistd.h>
#include <sys/stat.h>

namespace NexusForge::Platform {

// Android-specific implementations would go here
// This file provides Android-specific overrides for the platform abstraction

// Global reference to ANativeActivity
static ANativeActivity* g_activity = nullptr;

// Set the native activity (called from JNI)
void setNativeActivity(ANativeActivity* activity) {
    g_activity = activity;
}

ANativeActivity* getNativeActivity() {
    return g_activity;
}

// Window management implementations
void* PlatformAbstraction::createMainWindow(const std::string& title, int width, int height) {
    return g_activity ? g_activity->window : nullptr;
}
void* PlatformAbstraction::createSplashWindow(int width, int height) { return g_activity ? g_activity->window : nullptr; }
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

#endif // NEXUS_PLATFORM_ANDROID
