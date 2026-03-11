// core/platform/platform_web.cpp
#include "platform_abstraction.hpp"

#ifdef NEXUS_PLATFORM_WEB

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace NexusForge::Platform {

PlatformType getCurrentPlatform() {
    return PlatformType::Web;
}

std::string getPlatformName() {
    return "Web (Emscripten)";
}

bool isPlatform(PlatformType platform) {
    return platform == PlatformType::Web;
}

bool initialize() {
    return true;
}

void shutdown() {
}

void processEvents() {
    emscripten_main_loop();
}

void pumpEvents() {
    processEvents();
}

void* createMainWindow(int width, int height, void** nativeWindow) {
    // Web canvas is created by Emscripten
    return (void*)1;
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
    emscripten_set_window_title(title);
}

void setWindowSize(void* window, int width, int height) {
    emscripten_set_canvas_element_size("#canvas", width, height);
}

void setWindowPosition(void* window, int x, int y) {
    // Not applicable for web
}

void maximizeWindow(void* window) {
    // Fullscreen via Emscripten
    EmscriptenFullscreenStrategy strategy = {};
    strategy.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_STRETCH;
    emscripten_request_fullscreen_strategy("#canvas", 1, &strategy);
}

void minimizeWindow(void* window) {
}

void restoreWindow(void* window) {
    emscripten_exit_fullscreen();
}

void setWindowFullscreen(void* window, bool fullscreen) {
    if (fullscreen) {
        emscripten_request_fullscreen("#canvas", 1);
    } else {
        emscripten_exit_fullscreen();
    }
}

std::string getExecutablePath() {
    return "";
}

std::string getCurrentDirectory() {
    return "/";
}

bool setCurrentDirectory(const std::string& path) {
    return emscripten_chdir(path.c_str()) == 0;
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
    return emscripten_mkdir(path.c_str(), 0755) == 0;
}

bool deleteFile(const std::string& path) {
    return emscripten_unlink(path.c_str()) == 0;
}

bool deleteDirectory(const std::string& path, bool recursive) {
    return emscripten_rmdir(path.c_str()) == 0;
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
    return "";
}

bool setEnvironmentVariable(const std::string& name, const std::string& value) {
    return false;
}

std::string getHomeDirectory() {
    return "/home/web_user";
}

std::string getDataDirectory() {
    return "/data";
}

std::string getConfigDirectory() {
    return "/config";
}

std::string getTempDirectory() {
    return "/tmp";
}

std::string getClipboardText() {
    // Web clipboard via JavaScript
    return "";
}

void setClipboardText(const std::string& text) {
    EM_ASM({
        navigator.clipboard.writeText(UTF8ToString($0));
    }, text.c_str());
}

bool hasClipboardText() {
    return false;
}

void setCursor(CursorType cursor) {
    // Web cursor via CSS
}

void showCursor() {
    EM_ASM(document.body.style.cursor = 'default');
}

void hideCursor() {
    EM_ASM(document.body.style.cursor = 'none');
}

bool isCursorVisible() {
    return true;
}

uint64_t getCurrentTimeMs() {
    return emscripten_get_now();
}

double getCurrentTimeSeconds() {
    return emscripten_get_now() / 1000.0;
}

void sleep(int milliseconds) {
    emscripten_sleep(milliseconds);
}

void* createThread(void (*func)(void*), void* arg) {
    // Web threading via Web Workers
    return nullptr;
}

void joinThread(void* thread) {
}

void detachThread(void* thread) {
}

void yieldThread() {
    emscripten_yield();
}

void* loadLibrary(const std::string& path) {
    return nullptr;
}

void unloadLibrary(void* handle) {
}

void* getProcAddress(void* handle, const std::string& name) {
    return nullptr;
}

int getCurrentProcessId() {
    return 1;
}

int getCurrentThreadId() {
    return 1;
}

bool openUrl(const std::string& url) {
    EM_ASM({
        window.open(UTF8ToString($0), '_blank');
    }, url.c_str());
    return true;
}

bool openFile(const std::string& path) {
    return false;
}

bool openFolder(const std::string& path) {
    return false;
}

SystemInfo getSystemInfo() {
    SystemInfo info;
    info.cpuCount = navigator.hardwareConcurrency ? navigator.hardwareConcurrency : 4;
    info.osVersion = "Web";
    info.machineName = "Web Browser";
    return info;
}

PowerStatus getPowerStatus() {
    return PowerStatus::Unknown;
}

int getBatteryLevel() {
    return -1;
}

void showNotification(const std::string& title, const std::string& message) {
    EM_ASM({
        if (Notification.permission === 'granted') {
            new Notification(UTF8ToString($0), { body: UTF8ToString($1) });
        }
    }, title.c_str(), message.c_str());
}

void showWarning(const std::string& title, const std::string& message) {
    showNotification(title, message);
}

void showError(const std::string& title, const std::string& message) {
    showNotification(title, message);
}

std::string showOpenFileDialog(const std::string& title,
                                const std::vector<std::pair<std::string, std::string>>& filters) {
    // Web file input
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
    // Web alert
    EM_ASM({
        alert(UTF8ToString($0) + '\n\n' + UTF8ToString($1));
    }, title.c_str(), message.c_str());
    return 0;
}

} // namespace NexusForge::Platform

#endif // NEXUS_PLATFORM_WEB
