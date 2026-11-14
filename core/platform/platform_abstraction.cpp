// core/platform/platform_abstraction.cpp
#include "platform_abstraction.hpp"
#include <chrono>
#include <thread>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>
#include <unistd.h>

#if defined(_WIN32) || defined(_WIN64)
    #define NOMINMAX
    #include <windows.h>
    #include <shlobj.h>
    #include <direct.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <pwd.h>
    #include <sys/types.h>
#elif defined(__APPLE__)
    #include <unistd.h>
    #include <pwd.h>
    #include <sys/types.h>
    #include <mach-o/dyld.h>
#endif

namespace NexusForge::Platform {

PlatformType PlatformAbstraction::currentPlatform_ = PlatformType::Linux;
bool PlatformAbstraction::initialized_ = false;

bool PlatformAbstraction::initialize() {
#if defined(_WIN32) || defined(_WIN64)
    currentPlatform_ = PlatformType::Windows;
#elif defined(__linux__) && !defined(__ANDROID__)
    currentPlatform_ = PlatformType::Linux;
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        currentPlatform_ = PlatformType::iOS;
    #else
        currentPlatform_ = PlatformType::MacOS;
    #endif
#elif defined(__ANDROID__)
    currentPlatform_ = PlatformType::Android;
#elif defined(__EMSCRIPTEN__)
    currentPlatform_ = PlatformType::Web;
#endif

    initialized_ = true;
    return true;
}

void PlatformAbstraction::shutdown() {
    initialized_ = false;
}

PlatformType PlatformAbstraction::getCurrentPlatform() {
    return currentPlatform_;
}

std::string PlatformAbstraction::getPlatformName() {
    switch (currentPlatform_) {
        case PlatformType::Windows: return "Windows";
        case PlatformType::Linux: return "Linux";
        case PlatformType::MacOS: return "macOS";
        case PlatformType::Android: return "Android";
        case PlatformType::iOS: return "iOS";
        case PlatformType::Web: return "Web";
        default: return "Unknown";
    }
}

SystemInfo PlatformAbstraction::getSystemInfo() {
    SystemInfo info;
    info.platform = currentPlatform_;
    info.osName = getPlatformName();
    info.osVersion = "1.0";
    info.architecture = "x86_64";
    info.cpuCount = 4;
    info.totalMemory = 8ULL * 1024 * 1024 * 1024;
    info.availableMemory = 4ULL * 1024 * 1024 * 1024;
    return info;
}

DirectoryPaths PlatformAbstraction::getDirectoryPaths() {
    DirectoryPaths paths;
    
    const char* home = getenv("HOME");
    if (!home) {
        home = getpwuid(getuid())->pw_dir;
    }
    paths.homeDir = home ? home : "/root";
    paths.dataDir = paths.homeDir + "/.local/share";
    paths.configDir = paths.homeDir + "/.config";
    paths.cacheDir = paths.homeDir + "/.cache";
    paths.tempDir = "/tmp";
    paths.documentsDir = paths.homeDir + "/Documents";
    paths.downloadsDir = paths.homeDir + "/Downloads";
    paths.appDataDir = paths.configDir + "/NexusForge";
    
    return paths;
}

std::string PlatformAbstraction::getCurrentWorkingDirectory() {
    char buffer[4096];
    if (getcwd(buffer, sizeof(buffer))) {
        return std::string(buffer);
    }
    return ".";
}

bool PlatformAbstraction::setCurrentWorkingDirectory(const std::string& path) {
#ifdef _WIN32
    return _chdir(path.c_str()) == 0;
#else
    return chdir(path.c_str()) == 0;
#endif
}

bool PlatformAbstraction::fileExists(const std::string& path) {
#ifdef _WIN32
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
#else
    return access(path.c_str(), F_OK) == 0;
#endif
}

bool PlatformAbstraction::directoryExists(const std::string& path) {
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

bool PlatformAbstraction::createDirectory(const std::string& path) {
#ifdef _WIN32
    return CreateDirectoryA(path.c_str(), nullptr) != 0;
#else
    return mkdir(path.c_str(), 0755) == 0;
#endif
}

bool PlatformAbstraction::deleteFile(const std::string& path) {
#ifdef _WIN32
    return DeleteFileA(path.c_str()) != 0;
#else
    return unlink(path.c_str()) == 0;
#endif
}

bool PlatformAbstraction::deleteDirectory(const std::string& path) {
#ifdef _WIN32
    return RemoveDirectoryA(path.c_str()) != 0;
#else
    return rmdir(path.c_str()) == 0;
#endif
}

std::vector<FileInfo> PlatformAbstraction::listDirectory(const std::string& path) {
    std::vector<FileInfo> result;
#ifdef _WIN32
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA((path + "/*").c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
                FileInfo info;
                info.name = findData.cFileName;
                info.path = path + "/" + info.name;
                info.isFile = !(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                info.isDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                result.push_back(info);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(path.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                FileInfo info;
                info.name = entry->d_name;
                info.path = path + "/" + info.name;
                
                struct stat st;
                if (stat(info.path.c_str(), &st) == 0) {
                    info.isFile = S_ISREG(st.st_mode);
                    info.isDirectory = S_ISDIR(st.st_mode);
                    info.size = st.st_size;
                }
                result.push_back(info);
            }
        }
        closedir(dir);
    }
#endif
    return result;
}

std::string PlatformAbstraction::getAbsolutePath(const std::string& relativePath) {
#ifdef _WIN32
    char fullPath[MAX_PATH];
    if (GetFullPathNameA(relativePath.c_str(), MAX_PATH, fullPath, nullptr)) {
        return fullPath;
    }
#else
    char* abs = realpath(relativePath.c_str(), nullptr);
    if (abs) {
        std::string result(abs);
        free(abs);
        return result;
    }
#endif
    return relativePath;
}

std::string PlatformAbstraction::getEnvironmentVariable(const std::string& name) {
    const char* value = getenv(name.c_str());
    return value ? std::string(value) : "";
}

bool PlatformAbstraction::setEnvironmentVariable(const std::string& name, const std::string& value) {
#ifdef _WIN32
    return SetEnvironmentVariableA(name.c_str(), value.c_str()) != 0;
#else
    return setenv(name.c_str(), value.c_str(), 1) == 0;
#endif
}

void PlatformAbstraction::sleep(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void PlatformAbstraction::processEvents() {
    // Platform-specific event processing
}

void PlatformAbstraction::pumpEvents() {
    processEvents();
}

std::string PlatformAbstraction::getClipboardText() {
    return "";
}

void PlatformAbstraction::setClipboardText(const std::string& text) {
    // Platform-specific implementation
}

bool PlatformAbstraction::hasClipboardText() {
    return !getClipboardText().empty();
}

int PlatformAbstraction::getCurrentProcessId() {
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

bool PlatformAbstraction::openExternal(const std::string& url) {
#ifdef _WIN32
    return ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL) > (HINSTANCE)32;
#elif defined(__linux__)
    return system(("xdg-open \"" + url + "\"").c_str()) == 0;
#elif defined(__APPLE__)
    return system(("open \"" + url + "\"").c_str()) == 0;
#endif
    return false;
}

bool PlatformAbstraction::openFileWithDefaultApp(const std::string& path) {
    return openExternal(path);
}

std::string PlatformAbstraction::executeCommand(const std::string& command, int* exitCode) {
    std::string result;
#ifdef _WIN32
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif
    if (pipe) {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            result += buffer;
        }
        int status = 
#ifdef _WIN32
            _pclose(pipe);
#else
            pclose(pipe);
#endif
        if (exitCode) {
#ifdef _WIN32
            *exitCode = status;
#else
            *exitCode = WEXITSTATUS(status);
#endif
        }
    }
    return result;
}

void PlatformAbstraction::setThreadName(const std::string& name) {
#ifdef _WIN32
    // Windows thread naming
#elif defined(__linux__)
    pthread_setname_np(pthread_self(), name.c_str());
#endif
}

int PlatformAbstraction::getCurrentThreadId() {
#ifdef _WIN32
    return GetCurrentThreadId();
#else
    return gettid();
#endif
}

std::string PlatformAbstraction::getHostname() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return hostname;
    }
    return "localhost";
}

std::string PlatformAbstraction::getUserName() {
    const char* user = getenv("USER");
    if (!user) {
        user = getpwuid(getuid())->pw_name;
    }
    return user ? user : "unknown";
}

bool PlatformAbstraction::isNetworkAvailable() {
#ifdef _WIN32
    return system("ping -n 1 8.8.8.8 > nul 2>&1") == 0;
#else
    return system("ping -c1 8.8.8.8 > /dev/null 2>&1") == 0;
#endif
}

std::string PlatformAbstraction::openFileDialog(const std::string& title,
                                                  const std::vector<std::string>& filters,
                                                  const std::string& defaultPath) {
#ifdef _WIN32
    // Windows file dialog
    return "";
#elif defined(__linux__)
    std::string cmd = "zenity --file-selection --title=\"" + title + "\"";
    if (!defaultPath.empty()) {
        cmd += " --filename=\"" + defaultPath + "\"";
    }
    return executeCommand(cmd);
#endif
    return "";
}

std::string PlatformAbstraction::saveFileDialog(const std::string& title,
                                                  const std::vector<std::string>& filters,
                                                  const std::string& defaultPath) {
#ifdef __linux__
    std::string cmd = "zenity --file-selection --save --title=\"" + title + "\"";
    if (!defaultPath.empty()) {
        cmd += " --filename=\"" + defaultPath + "\"";
    }
    return executeCommand(cmd);
#endif
    return "";
}

std::string PlatformAbstraction::selectDirectoryDialog(const std::string& title,
                                                        const std::string& defaultPath) {
#ifdef __linux__
    std::string cmd = "zenity --file-selection --directory --title=\"" + title + "\"";
    if (!defaultPath.empty()) {
        cmd += " --filename=\"" + defaultPath + "\"";
    }
    return executeCommand(cmd);
#endif
    return "";
}

PlatformAbstraction::MessageBoxResult PlatformAbstraction::showMessageBox(
    const std::string& title,
    const std::string& message,
    MessageBoxType type,
    MessageBoxButton buttons) {
    
#ifdef __linux__
    std::string cmd = "zenity --";
    switch (type) {
        case MessageBoxType::Info: cmd += "info"; break;
        case MessageBoxType::Warning: cmd += "warning"; break;
        case MessageBoxType::Error: cmd += "error"; break;
        case MessageBoxType::Question: cmd += "question"; break;
    }
    cmd += " --title=\"" + title + "\" --text=\"" + message + "\"";
    
    int exitCode;
    executeCommand(cmd, &exitCode);
    
    if (exitCode == 0) return MessageBoxResult::OK;
    return MessageBoxResult::Cancel;
#endif
    return MessageBoxResult::OK;
}

} // namespace NexusForge::Platform
