// core/platform/platform_linux.cpp
#include "platform_abstraction.hpp"

#ifdef NEXUS_PLATFORM_LINUX

#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <pwd.h>
#include <dlfcn.h>
#include <pthread.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

namespace NexusForge::Platform {

static Display* g_display = nullptr;
static Window g_rootWindow = 0;

PlatformType getCurrentPlatform() {
    return PlatformType::Linux;
}

std::string getPlatformName() {
    return "Linux";
}

bool isPlatform(PlatformType platform) {
    return platform == PlatformType::Linux;
}

bool initialize() {
    g_display = XOpenDisplay(nullptr);
    if (g_display) {
        g_rootWindow = DefaultRootWindow(g_display);
    }
    return true;
}

void shutdown() {
    if (g_display) {
        XCloseDisplay(g_display);
        g_display = nullptr;
    }
}

void processEvents() {
    if (!g_display) return;
    
    XEvent event;
    while (XPending(g_display)) {
        XNextEvent(g_display, &event);
        // Process X11 events
    }
}

void pumpEvents() {
    processEvents();
}

void* createMainWindow(int width, int height, void** nativeWindow) {
    if (!g_display) return nullptr;
    
    // Create a simple X11 window
    XSetWindowAttributes attrs;
    attrs.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask;
    
    Window window = XCreateWindow(
        g_display, g_rootWindow,
        0, 0, width, height, 0,
        CopyFromParent, InputOutput, CopyFromParent,
        CWEventMask, &attrs
    );
    
    XStoreName(g_display, window, "NexusForge IDE");
    XMapWindow(g_display, window);
    XFlush(g_display);
    
    if (nativeWindow) *nativeWindow = reinterpret_cast<void*>(window);
    return reinterpret_cast<void*>(window);
}

void* createSplashWindow(int width, int height, void** nativeWindow) {
    return createMainWindow(width, height, nativeWindow);
}

void destroyWindow(void* window) {
    if (window && g_display) {
        XDestroyWindow(g_display, reinterpret_cast<Window>(window));
        XFlush(g_display);
    }
}

void showWindow(void* window) {
    if (window && g_display) {
        XMapWindow(g_display, reinterpret_cast<Window>(window));
        XFlush(g_display);
    }
}

void hideWindow(void* window) {
    if (window && g_display) {
        XUnmapWindow(g_display, reinterpret_cast<Window>(window));
        XFlush(g_display);
    }
}

void setWindowTitle(void* window, const char* title) {
    if (window && g_display && title) {
        XStoreName(g_display, reinterpret_cast<Window>(window), title);
        XFlush(g_display);
    }
}

void setWindowSize(void* window, int width, int height) {
    if (window && g_display) {
        XResizeWindow(g_display, reinterpret_cast<Window>(window), width, height);
        XFlush(g_display);
    }
}

void setWindowPosition(void* window, int x, int y) {
    if (window && g_display) {
        XMoveWindow(g_display, reinterpret_cast<Window>(window), x, y);
        XFlush(g_display);
    }
}

void maximizeWindow(void* window) {
    // X11 window manager specific
}

void minimizeWindow(void* window) {
    if (window && g_display) {
        XIconifyWindow(g_display, reinterpret_cast<Window>(window), DefaultScreen(g_display));
        XFlush(g_display);
    }
}

void restoreWindow(void* window) {
    if (window && g_display) {
        XMapWindow(g_display, reinterpret_cast<Window>(window));
        XFlush(g_display);
    }
}

void setWindowFullscreen(void* window, bool fullscreen) {
    // X11 fullscreen implementation
}

std::string getExecutablePath() {
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX - 1);
    if (count != -1) {
        path[count] = '\0';
        return std::string(path);
    }
    return "";
}

std::string getCurrentDirectory() {
    char path[PATH_MAX];
    if (getcwd(path, sizeof(path))) {
        return std::string(path);
    }
    return "";
}

bool setCurrentDirectory(const std::string& path) {
    return chdir(path.c_str()) == 0;
}

bool fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool directoryExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

bool createDirectory(const std::string& path) {
    return mkdir(path.c_str(), 0755) == 0;
}

bool deleteFile(const std::string& path) {
    return unlink(path.c_str()) == 0;
}

bool deleteDirectory(const std::string& path, bool recursive) {
    if (recursive) {
        return rmdir(path.c_str()) == 0;
    }
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
    // Basic normalization
    std::string result = path;
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

std::string getAbsolutePath(const std::string& relativePath) {
    if (relativePath.empty() || relativePath[0] == '/') {
        return relativePath;
    }
    return joinPath(getCurrentDirectory(), relativePath);
}

std::string getRelativePath(const std::string& basePath, const std::string& path) {
    // Simplified implementation
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
    const char* home = getenv("HOME");
    if (home) return home;
    
    struct passwd* pw = getpwuid(getuid());
    if (pw) return pw->pw_dir;
    return "";
}

std::string getDataDirectory() {
    const char* data = getenv("XDG_DATA_HOME");
    if (data) return data;
    return joinPath(getHomeDirectory(), ".local/share");
}

std::string getConfigDirectory() {
    const char* config = getenv("XDG_CONFIG_HOME");
    if (config) return config;
    return joinPath(getHomeDirectory(), ".config");
}

std::string getTempDirectory() {
    const char* temp = getenv("TMPDIR");
    if (temp) return temp;
    return "/tmp";
}

std::string getClipboardText() {
    // X11 clipboard implementation
    return "";
}

void setClipboardText(const std::string& text) {
    // X11 clipboard implementation
}

bool hasClipboardText() {
    return !getClipboardText().empty();
}

void setCursor(CursorType cursor) {
    // X11 cursor implementation
}

void showCursor() {
    // Show cursor
}

void hideCursor() {
    // Hide cursor
}

bool isCursorVisible() {
    return true;
}

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
    return gettid();
}

bool openUrl(const std::string& url) {
    std::string cmd = "xdg-open " + url + " &";
    return system(cmd.c_str()) == 0;
}

bool openFile(const std::string& path) {
    std::string cmd = "xdg-open " + path + " &";
    return system(cmd.c_str()) == 0;
}

bool openFolder(const std::string& path) {
    std::string cmd = "xdg-open " + path + " &";
    return system(cmd.c_str()) == 0;
}

SystemInfo getSystemInfo() {
    SystemInfo info;
    
    // CPU count
    info.cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
    
    // Memory
    info.totalMemory = sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE);
    
    // OS version
    info.osVersion = "Linux";
    
    // Machine name
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        info.machineName = hostname;
    }
    
    return info;
}

PowerStatus getPowerStatus() {
    // Check for power supply
    if (access("/sys/class/power_supply/BAT0", F_OK) == 0) {
        return PowerStatus::Battery;
    }
    return PowerStatus::AC;
}

int getBatteryLevel() {
    std::ifstream capacity("/sys/class/power_supply/BAT0/capacity");
    if (capacity.is_open()) {
        int level;
        capacity >> level;
        return level;
    }
    return -1;
}

void showNotification(const std::string& title, const std::string& message) {
    std::string cmd = "notify-send \"" + title + "\" \"" + message + "\" &";
    system(cmd.c_str());
}

void showWarning(const std::string& title, const std::string& message) {
    showNotification(title, message);
}

void showError(const std::string& title, const std::string& message) {
    showNotification(title, message);
}

std::string showOpenFileDialog(const std::string& title,
                                const std::vector<std::pair<std::string, std::string>>& filters) {
    // Use zenity or kdialog
    std::string cmd = "zenity --file-selection --title=\"" + title + "\"";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    
    char buffer[256];
    std::string result;
    if (fgets(buffer, sizeof(buffer), pipe)) {
        result = buffer;
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }
    }
    pclose(pipe);
    return result;
}

std::vector<std::string> showOpenMultipleFileDialog(const std::string& title,
                                                     const std::vector<std::pair<std::string, std::string>>& filters) {
    // Use zenity
    std::string cmd = "zenity --file-selection --multiple --title=\"" + title + "\"";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return {};
    
    std::vector<std::string> results;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        std::string line = buffer;
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }
        results.push_back(line);
    }
    pclose(pipe);
    return results;
}

std::string showSaveFileDialog(const std::string& title,
                                const std::string& defaultName,
                                const std::vector<std::pair<std::string, std::string>>& filters) {
    std::string cmd = "zenity --file-selection --save --title=\"" + title + "\"";
    if (!defaultName.empty()) {
        cmd += " --filename=\"" + defaultName + "\"";
    }
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    
    char buffer[256];
    std::string result;
    if (fgets(buffer, sizeof(buffer), pipe)) {
        result = buffer;
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }
    }
    pclose(pipe);
    return result;
}

std::string showFolderDialog(const std::string& title) {
    std::string cmd = "zenity --file-selection --directory --title=\"" + title + "\"";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    
    char buffer[256];
    std::string result;
    if (fgets(buffer, sizeof(buffer), pipe)) {
        result = buffer;
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }
    }
    pclose(pipe);
    return result;
}

int showMessageBox(const std::string& title, const std::string& message,
                   MessageBoxType type, MessageBoxButtons buttons) {
    std::string cmd = "zenity --";
    switch (type) {
        case MessageBoxType::Info: cmd += "info"; break;
        case MessageBoxType::Warning: cmd += "warning"; break;
        case MessageBoxType::Error: cmd += "error"; break;
        case MessageBoxType::Question: cmd += "question"; break;
    }
    cmd += " --title=\"" + title + "\" --text=\"" + message + "\"";
    
    int result = system(cmd.c_str());
    return (result == 0) ? 0 : 1;
}

} // namespace NexusForge::Platform

#endif // NEXUS_PLATFORM_LINUX
