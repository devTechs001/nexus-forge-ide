// core/platform/platform_abstraction.hpp
#pragma once

#include <string>
#include <functional>
#include <cstdint>

namespace NexusForge::Platform {

// Platform detection
enum class PlatformType {
    Windows,
    Linux,
    macOS,
    Android,
    iOS,
    Web
};

// Get current platform
PlatformType getCurrentPlatform();
std::string getPlatformName();
bool isPlatform(PlatformType platform);

// Initialization
bool initialize();
void shutdown();

// Event processing
void processEvents();
void pumpEvents();

// Window management
void* createMainWindow(int width, int height, void** nativeWindow);
void* createSplashWindow(int width, int height, void** nativeWindow);
void destroyWindow(void* window);
void showWindow(void* window);
void hideWindow(void* window);
void setWindowTitle(void* window, const char* title);
void setWindowSize(void* window, int width, int height);
void setWindowPosition(void* window, int x, int y);
void maximizeWindow(void* window);
void minimizeWindow(void* window);
void restoreWindow(void* window);
void setWindowFullscreen(void* window, bool fullscreen);

// File system
std::string getExecutablePath();
std::string getCurrentDirectory();
bool setCurrentDirectory(const std::string& path);
bool fileExists(const std::string& path);
bool directoryExists(const std::string& path);
bool createDirectory(const std::string& path);
bool deleteFile(const std::string& path);
bool deleteDirectory(const std::string& path, bool recursive);
std::string readTextFile(const std::string& path);
bool writeTextFile(const std::string& path, const std::string& content);
std::vector<uint8_t> readBinaryFile(const std::string& path);
bool writeBinaryFile(const std::string& path, const std::vector<uint8_t>& data);

// Path utilities
std::string joinPath(const std::string& base, const std::string& path);
std::string getDirectoryName(const std::string& path);
std::string getFileName(const std::string& path);
std::string getExtension(const std::string& path);
std::string normalizePath(const std::string& path);
std::string getAbsolutePath(const std::string& relativePath);
std::string getRelativePath(const std::string& basePath, const std::string& path);

// Environment
std::string getEnvironmentVariable(const std::string& name);
bool setEnvironmentVariable(const std::string& name, const std::string& value);
std::string getHomeDirectory();
std::string getDataDirectory();
std::string getConfigDirectory();
std::string getTempDirectory();

// Clipboard
std::string getClipboardText();
void setClipboardText(const std::string& text);
bool hasClipboardText();

// Cursor
enum class CursorType {
    Arrow,
    IBeam,
    Hand,
    Crosshair,
    ResizeH,
    ResizeV,
    ResizeNESW,
    ResizeNWSE,
    Move,
    NotAllowed,
    Wait,
    Progress
};
void setCursor(CursorType cursor);
void showCursor();
void hideCursor();
bool isCursorVisible();

// Time
uint64_t getCurrentTimeMs();
double getCurrentTimeSeconds();
void sleep(int milliseconds);

// Threading
void* createThread(void (*func)(void*), void* arg);
void joinThread(void* thread);
void detachThread(void* thread);
void yieldThread();

// Mutex
void* createMutex();
void lockMutex(void* mutex);
void unlockMutex(void* mutex);
void destroyMutex(void* mutex);

// Dynamic library loading
void* loadLibrary(const std::string& path);
void unloadLibrary(void* handle);
void* getProcAddress(void* handle, const std::string& name);

// Process
int getCurrentProcessId();
int getCurrentThreadId();
bool openUrl(const std::string& url);
bool openFile(const std::string& path);
bool openFolder(const std::string& path);

// System info
struct SystemInfo {
    int cpuCount;
    size_t totalMemory;
    size_t availableMemory;
    std::string osVersion;
    std::string machineName;
};
SystemInfo getSystemInfo();

// Power
enum class PowerStatus {
    Unknown,
    Battery,
    AC,
    UPS
};
PowerStatus getPowerStatus();
int getBatteryLevel();  // 0-100, -1 if unknown

// Notifications
void showNotification(const std::string& title, const std::string& message);
void showWarning(const std::string& title, const std::string& message);
void showError(const std::string& title, const std::string& message);

// Dialogs
std::string showOpenFileDialog(const std::string& title,
                                const std::vector<std::pair<std::string, std::string>>& filters);
std::vector<std::string> showOpenMultipleFileDialog(const std::string& title,
                                                     const std::vector<std::pair<std::string, std::string>>& filters);
std::string showSaveFileDialog(const std::string& title,
                                const std::string& defaultName,
                                const std::vector<std::pair<std::string, std::string>>& filters);
std::string showFolderDialog(const std::string& title);

enum class MessageBoxType {
    Info,
    Warning,
    Error,
    Question
};

enum class MessageBoxButtons {
    OK,
    OKCancel,
    YesNo,
    YesNoCancel
};

int showMessageBox(const std::string& title, const std::string& message,
                   MessageBoxType type = MessageBoxType::Info,
                   MessageBoxButtons buttons = MessageBoxButtons::OK);

} // namespace NexusForge::Platform
