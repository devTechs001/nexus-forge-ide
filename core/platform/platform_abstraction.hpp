// core/platform/platform_abstraction.hpp
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>

namespace NexusForge::Platform {

// Platform detection
enum class PlatformType {
    Windows,
    Linux,
    MacOS,
    Android,
    iOS,
    Web,
    Unknown
};

// System information
struct SystemInfo {
    PlatformType platform;
    std::string osName;
    std::string osVersion;
    std::string architecture;
    int cpuCount;
    size_t totalMemory;
    size_t availableMemory;
};

// Directory paths
struct DirectoryPaths {
    std::string homeDir;
    std::string dataDir;
    std::string configDir;
    std::string cacheDir;
    std::string tempDir;
    std::string documentsDir;
    std::string downloadsDir;
    std::string appDataDir;
};

// File information
struct FileInfo {
    std::string path;
    std::string name;
    std::string extension;
    size_t size;
    bool isDirectory;
    bool isFile;
    bool isReadable;
    bool isWritable;
    bool isExecutable;
    uint64_t createdTime;
    uint64_t modifiedTime;
    uint64_t accessedTime;
};

// Process information
struct ProcessInfo {
    int pid;
    std::string name;
    std::string path;
    std::vector<std::string> arguments;
};

// Platform Abstraction Layer
class PlatformAbstraction {
public:
    // Initialization
    static bool initialize();
    static void shutdown();

    // Platform info
    static PlatformType getCurrentPlatform();
    static SystemInfo getSystemInfo();
    static DirectoryPaths getDirectoryPaths();
    static std::string getPlatformName();

    // File system operations
    static bool fileExists(const std::string& path);
    static bool directoryExists(const std::string& path);
    static bool createDirectory(const std::string& path);
    static bool deleteFile(const std::string& path);
    static bool deleteDirectory(const std::string& path);
    static bool copyFile(const std::string& source, const std::string& dest);
    static bool moveFile(const std::string& source, const std::string& dest);
    static std::vector<FileInfo> listDirectory(const std::string& path);
    static std::string getCurrentWorkingDirectory();
    static bool setCurrentWorkingDirectory(const std::string& path);
    static std::string getAbsolutePath(const std::string& relativePath);
    static std::string joinPath(const std::string& path1, const std::string& path2);

    // File I/O
    static std::string readFile(const std::string& path);
    static bool writeFile(const std::string& path, const std::string& content);
    static std::vector<char> readBinaryFile(const std::string& path);
    static bool writeBinaryFile(const std::string& path, const std::vector<char>& data);

    // Process management
    static int getCurrentProcessId();
    static ProcessInfo getCurrentProcessInfo();
    static bool openExternal(const std::string& url);
    static bool openFileWithDefaultApp(const std::string& path);
    static std::string executeCommand(const std::string& command, int* exitCode = nullptr);

    // Environment
    static std::string getEnvironmentVariable(const std::string& name);
    static bool setEnvironmentVariable(const std::string& name, const std::string& value);
    static std::vector<std::string> getCommandLineArguments();

    // Clipboard
    static std::string getClipboardText();
    static void setClipboardText(const std::string& text);
    static bool hasClipboardText();

    // Time
    static uint64_t getCurrentTimeMillis();
    static uint64_t getCurrentTimeMicros();
    static void sleep(int milliseconds);

    // Event processing (platform-specific)
    static void processEvents();
    static void pumpEvents();

    // UI helpers
    static std::string openFileDialog(const std::string& title,
                                       const std::vector<std::string>& filters,
                                       const std::string& defaultPath);
    static std::string saveFileDialog(const std::string& title,
                                       const std::vector<std::string>& filters,
                                       const std::string& defaultPath);
    static std::string selectDirectoryDialog(const std::string& title,
                                              const std::string& defaultPath);

    // MessageBox
    enum class MessageBoxType { Info, Warning, Error, Question };
    enum class MessageBoxButton { OK, OKCancel, YesNo, YesNoCancel };
    enum class MessageBoxResult { OK, Cancel, Yes, No };

    static MessageBoxResult showMessageBox(const std::string& title,
                                            const std::string& message,
                                            MessageBoxType type = MessageBoxType::Info,
                                            MessageBoxButton buttons = MessageBoxButton::OK);

    // Window management (platform-specific)
    static void* createMainWindow(const std::string& title, int width, int height);
    static void* createSplashWindow(int width, int height);
    static void destroyWindow(void* window);
    static void showWindow(void* window);
    static void hideWindow(void* window);
    static void setWindowTitle(void* window, const char* title);
    static void setWindowSize(void* window, int width, int height);
    static void setWindowPosition(void* window, int x, int y);
    static void maximizeWindow(void* window);
    static void minimizeWindow(void* window);
    static void restoreWindow(void* window);
    static void setWindowFullscreen(void* window, bool fullscreen);
    static bool isWindowFullscreen(void* window);
    static void* getNativeWindowHandle(void* window);

    // Threading
    static void setThreadName(const std::string& name);
    static int getCurrentThreadId();

    // Networking
    static std::string getHostname();
    static std::string getUserName();
    static bool isNetworkAvailable();

private:
    static PlatformType currentPlatform_;
    static bool initialized_;
};

} // namespace NexusForge::Platform
