// core/platform/platform_abstraction.hpp
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace NexusForge::Platform {

// Platform type
enum class PlatformType {
    Windows,
    Linux,
    MacOS,
    Android,
    iOS,
    Web,
    Unknown
};

// File system operations
struct FileStats {
    size_t size;
    int64_t createdTime;
    int64_t modifiedTime;
    int64_t accessedTime;
    bool isDirectory;
    bool isFile;
    bool isSymlink;
};

// Window handle
using WindowHandle = void*;

// Platform abstraction layer
class PlatformAbstraction {
public:
    // Initialization
    static bool initialize();
    static void shutdown();

    // Platform info
    static PlatformType getPlatformType();
    static std::string getPlatformName();
    static std::string getOSVersion();
    static std::string getCPUInfo();
    static size_t getTotalMemory();
    static size_t getAvailableMemory();

    // File system
    static std::string getCurrentDirectory();
    static bool setCurrentDirectory(const std::string& path);
    static bool fileExists(const std::string& path);
    static bool directoryExists(const std::string& path);
    static bool createDirectory(const std::string& path);
    static bool deleteFile(const std::string& path);
    static bool deleteDirectory(const std::string& path);
    static std::vector<std::string> listDirectory(const std::string& path);
    static FileStats getFileStats(const std::string& path);
    static std::string getExecutablePath();
    static std::string getUserDataPath();
    static std::string getTempPath();

    // File I/O
    static std::string readFile(const std::string& path);
    static bool writeFile(const std::string& path, const std::string& content);
    static std::vector<uint8_t> readBinaryFile(const std::string& path);
    static bool writeBinaryFile(const std::string& path, const std::vector<uint8_t>& data);

    // Window management
    static WindowHandle createWindow(int width, int height, const std::string& title);
    static bool createSplashWindow(int width, int height, WindowHandle* handle);
    static void destroyWindow(WindowHandle window);
    static void setWindowTitle(WindowHandle window, const std::string& title);
    static void setWindowSize(WindowHandle window, int width, int height);
    static void setWindowPosition(WindowHandle window, int x, int y);
    static void showWindow(WindowHandle window);
    static void hideWindow(WindowHandle window);
    static void maximizeWindow(WindowHandle window);
    static void minimizeWindow(WindowHandle window);
    static bool isWindowMaximized(WindowHandle window);
    static bool isWindowMinimized(WindowHandle window);
    static void* getNativeWindowHandle(WindowHandle window);

    // Event processing
    static void processEvents();
    static void pollEvents();
    static void waitEvents();

    // Clipboard
    static std::string getClipboardText();
    static void setClipboardText(const std::string& text);

    // Dynamic library loading
    static void* loadLibrary(const std::string& path);
    static void unloadLibrary(void* handle);
    static void* getProcAddress(void* handle, const std::string& name);

    // Process
    static int executeCommand(const std::string& command, std::string& output);
    static bool openFile(const std::string& path);
    static bool openURL(const std::string& url);

    // Environment
    static std::string getEnvironmentVariable(const std::string& name);
    static bool setEnvironmentVariable(const std::string& name, const std::string& value);

    // Time
    static uint64_t getCurrentTimeMs();
    static uint64_t getHighResolutionTime();

    // Threading
    static void sleep(int milliseconds);
    static void yieldThread();

    // Logging
    static void log(const std::string& message);
    static void logError(const std::string& message);
    static void logWarning(const std::string& message);
};

} // namespace NexusForge::Platform
