// core/platform/platform_ios.cpp
#include "platform_abstraction.hpp"

#ifdef NEXUS_PLATFORM_IOS

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <dlfcn.h>
#include <pthread.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>

namespace NexusForge::Platform {

PlatformType getCurrentPlatform() {
    return PlatformType::iOS;
}

std::string getPlatformName() {
    return "iOS";
}

bool isPlatform(PlatformType platform) {
    return platform == PlatformType::iOS;
}

bool initialize() {
    return true;
}

void shutdown() {
}

void processEvents() {
    // iOS event processing via runloop
}

void pumpEvents() {
    processEvents();
}

void* createMainWindow(int width, int height, void** nativeWindow) {
    // iOS window creation via UIKit
    return nullptr;
}

void* createSplashWindow(int width, int height, void** nativeWindow) {
    return createMainWindow(width, height, nativeWindow);
}

void destroyWindow(void* window) {
}

void showWindow(void* window) {
}

void hideWindow(void* window) {
}

void setWindowTitle(void* window, const char* title) {
}

void setWindowSize(void* window, int width, int height) {
}

void setWindowPosition(void* window, int x, int y) {
}

void maximizeWindow(void* window) {
}

void minimizeWindow(void* window) {
}

void restoreWindow(void* window) {
}

void setWindowFullscreen(void* window, bool fullscreen) {
}

std::string getExecutablePath() {
    return [[[NSBundle mainBundle] executablePath] UTF8String];
}

std::string getCurrentDirectory() {
    return [[NSFileManager defaultManager] currentDirectoryPath] UTF8String];
}

bool setCurrentDirectory(const std::string& path) {
    return chdir(path.c_str()) == 0;
}

bool fileExists(const std::string& path) {
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0;
}

bool directoryExists(const std::string& path) {
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode);
}

bool createDirectory(const std::string& path) {
    return mkdir(path.c_str(), 0755) == 0;
}

bool deleteFile(const std::string& path) {
    return unlink(path.c_str()) == 0;
}

bool deleteDirectory(const std::string& path, bool recursive) {
    return rmdir(path.c_str()) == 0;
}

std::string readTextFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool writeTextFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << content;
    return true;
}

std::vector<uint8_t> readBinaryFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return {};
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

bool writeBinaryFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
}

std::string joinPath(const std::string& base, const std::string& path) {
    if (base.empty()) return path;
    if (path.empty()) return base;
    if (path[0] == '/') return path;
    if (base.back() == '/') return base + path;
    return base + "/" + path;
}

std::string getDirectoryName(const std::string& path) {
    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) return "";
    return path.substr(0, pos);
}

std::string getFileName(const std::string& path) {
    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

std::string getExtension(const std::string& path) {
    size_t pos = path.find_last_of('.');
    if (pos == std::string::npos) return "";
    return path.substr(pos + 1);
}

std::string normalizePath(const std::string& path) {
    std::string result = path;
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

std::string getAbsolutePath(const std::string& relativePath) {
    if (relativePath.empty() || relativePath[0] == '/') return relativePath;
    return joinPath(getCurrentDirectory(), relativePath);
}

std::string getRelativePath(const std::string& basePath, const std::string& path) {
    return path;
}

std::string getEnvironmentVariable(const std::string& name) {
    const char* value = getenv(name.c_str());
    return value ? value : "";
}

bool setEnvironmentVariable(const std::string& name, const std::string& value) {
    return setenv(name.c_str(), value.c_str(), 1) == 0;
}

std::string getHomeDirectory() {
    return NSHomeDirectory() ? [NSHomeDirectory() UTF8String] : "";
}

std::string getDataDirectory() {
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    return paths.count > 0 ? [[paths objectAtIndex:0] UTF8String] : "";
}

std::string getConfigDirectory() {
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
    return paths.count > 0 ? [[paths objectAtIndex:0] UTF8String] : "";
}

std::string getTempDirectory() {
    return NSTemporaryDirectory() ? [NSTemporaryDirectory() UTF8String] : "";
}

std::string getClipboardText() {
    return [[UIPasteboard generalPasteboard] string] ? [[[UIPasteboard generalPasteboard] string] UTF8String] : "";
}

void setClipboardText(const std::string& text) {
    [UIPasteboard generalPasteboard].string = [NSString stringWithUTF8String:text.c_str()];
}

bool hasClipboardText() {
    return [[UIPasteboard generalPasteboard] string] != nil;
}

void setCursor(CursorType cursor) {
    // Not applicable on iOS
}

void showCursor() {}
void hideCursor() {}
bool isCursorVisible() { return false; }

uint64_t getCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

double getCurrentTimeSeconds() {
    return std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

void sleep(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void* createThread(void (*func)(void*), void* arg) {
    pthread_t* thread = new pthread_t;
    pthread_create(thread, nullptr, (void*(*)(void*))func, arg);
    return thread;
}

void joinThread(void* thread) {
    if (thread) {
        pthread_join(*reinterpret_cast<pthread_t*>(thread), nullptr);
        delete reinterpret_cast<pthread_t*>(thread);
    }
}

void detachThread(void* thread) {
    if (thread) {
        pthread_detach(*reinterpret_cast<pthread_t*>(thread));
        delete reinterpret_cast<pthread_t*>(thread);
    }
}

void yieldThread() {
    std::this_thread::yield();
}

void* loadLibrary(const std::string& path) {
    return dlopen(path.c_str(), RTLD_LAZY);
}

void unloadLibrary(void* handle) {
    if (handle) dlclose(handle);
}

void* getProcAddress(void* handle, const std::string& name) {
    return dlsym(handle, name.c_str());
}

int getCurrentProcessId() {
    return getpid();
}

int getCurrentThreadId() {
    return pthread_mach_thread_np(pthread_self());
}

bool openUrl(const std::string& url) {
    NSURL* nsUrl = [NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]];
    return [[UIApplication sharedApplication] openURL:nsUrl];
}

bool openFile(const std::string& path) {
    // iOS file handling via UIDocumentInteractionController
    return false;
}

bool openFolder(const std::string& path) {
    return false;
}

SystemInfo getSystemInfo() {
    SystemInfo info;
    
    int mib[2];
    size_t len;
    
    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    len = sizeof(info.cpuCount);
    sysctl(mib, 2, &info.cpuCount, &len, NULL, 0);
    
    info.osVersion = "iOS";
    info.machineName = "iOS Device";
    
    return info;
}

PowerStatus getPowerStatus() {
    UIDeviceBatteryState state = [[UIDevice currentDevice] batteryState];
    if (state == UIDeviceBatteryStateCharging || state == UIDeviceBatteryStateFull) {
        return PowerStatus::AC;
    } else if (state == UIDeviceBatteryStateUnplugged) {
        return PowerStatus::Battery;
    }
    return PowerStatus::Unknown;
}

int getBatteryLevel() {
    [[UIDevice currentDevice] setBatteryMonitoringEnabled:YES];
    return (int)([[UIDevice currentDevice] batteryLevel] * 100);
}

void showNotification(const std::string& title, const std::string& message) {
    // iOS local notification
}

void showWarning(const std::string& title, const std::string& message) {
    showNotification(title, message);
}

void showError(const std::string& title, const std::string& message) {
    showNotification(title, message);
}

std::string showOpenFileDialog(const std::string& title,
                                const std::vector<std::pair<std::string, std::string>>& filters) {
    // iOS document picker
    return "";
}

std::vector<std::string> showOpenMultipleFileDialog(const std::string& title,
                                                     const std::vector<std::pair<std::string, std::string>>& filters) {
    return {};
}

std::string showSaveFileDialog(const std::string& title,
                                const std::string& defaultName,
                                const std::vector<std::pair<std::string, std::string>>& filters) {
    return "";
}

std::string showFolderDialog(const std::string& title) {
    return "";
}

int showMessageBox(const std::string& title, const std::string& message,
                   MessageBoxType type, MessageBoxButtons buttons) {
    // iOS alert view
    return 0;
}

} // namespace NexusForge::Platform

#endif // NEXUS_PLATFORM_IOS
