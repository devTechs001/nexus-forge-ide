// core/platform/platform_abstraction.cpp
#include "platform_abstraction.hpp"

#ifndef NEXUS_PLATFORM_LINUX

#include <chrono>
#include <thread>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(_WIN32) || defined(_WIN64)
    #define NOMINMAX
    #include <windows.h>
    #include <shlobj.h>
    #include <shellapi.h>
    #include <direct.h>
#else
    #include <dirent.h>
    #include <pwd.h>
    #include <unistd.h>
    #include <sys/wait.h>
    #include <pthread.h>
    #if defined(__linux__)
        #include <sys/syscall.h>
    #elif defined(__APPLE__)
        #include <TargetConditionals.h>
        #include <mach-o/dyld.h>
    #endif
#endif

namespace NexusForge::Platform {

PlatformType PlatformAbstraction::currentPlatform_ = PlatformType::Linux;
bool PlatformAbstraction::initialized_ = false;

bool PlatformAbstraction::initialize() {
#if defined(_WIN32) || defined(_WIN64)
    currentPlatform_ = PlatformType::Windows;
#elif defined(__ANDROID__)
    currentPlatform_ = PlatformType::Android;
#elif defined(__APPLE__)
    #if TARGET_OS_IPHONE
        currentPlatform_ = PlatformType::iOS;
    #else
        currentPlatform_ = PlatformType::MacOS;
    #endif
#elif defined(__EMSCRIPTEN__)
    currentPlatform_ = PlatformType::Web;
#else
    currentPlatform_ = PlatformType::Linux;
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

#ifdef _WIN32
    const char* home = getenv("USERPROFILE");
    paths.homeDir = home ? home : ".";
    const char* appData = getenv("APPDATA");
    paths.configDir = appData ? appData : (paths.homeDir + "/AppData/Roaming");
    paths.dataDir = paths.configDir;
    const char* localAppData = getenv("LOCALAPPDATA");
    paths.cacheDir = localAppData ? (std::string(localAppData) + "/NexusForge/Cache") : (paths.homeDir + "/.cache");
    const char* tmp = getenv("TEMP");
    paths.tempDir = tmp ? tmp : "C:/Temp";
#else
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        if (pw && pw->pw_dir) {
            home = pw->pw_dir;
        }
    }
    paths.homeDir = home ? home : "/tmp";
    paths.dataDir = paths.homeDir + "/.local/share";
    paths.configDir = paths.homeDir + "/.config";
    paths.cacheDir = paths.homeDir + "/.cache";
    paths.tempDir = "/tmp";
#endif
    paths.documentsDir = paths.homeDir + "/Documents";
    paths.downloadsDir = paths.homeDir + "/Downloads";
    paths.appDataDir = paths.configDir + "/NexusForge";

    return paths;
}

std::string PlatformAbstraction::getCurrentWorkingDirectory() {
    char buffer[4096];
#ifdef _WIN32
    if (_getcwd(buffer, sizeof(buffer))) {
        return std::string(buffer);
    }
#else
    if (getcwd(buffer, sizeof(buffer))) {
        return std::string(buffer);
    }
#endif
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
    return static_cast<int>(GetCurrentThreadId());
#elif defined(__linux__) && defined(SYS_gettid)
    return static_cast<int>(syscall(SYS_gettid));
#else
    return static_cast<int>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
#endif
}

std::string PlatformAbstraction::getHostname() {
    char hostname[256];
#ifdef _WIN32
    DWORD size = sizeof(hostname);
    if (GetComputerNameA(hostname, &size)) {
        return hostname;
    }
#else
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        hostname[sizeof(hostname) - 1] = '\0';
        return hostname;
    }
#endif
    return "localhost";
}

std::string PlatformAbstraction::getUserName() {
#ifdef _WIN32
    const char* user = getenv("USERNAME");
    return user ? user : "unknown";
#else
    const char* user = getenv("USER");
    if (!user) {
        struct passwd* pw = getpwuid(getuid());
        if (pw && pw->pw_name) {
            user = pw->pw_name;
        }
    }
    return user ? user : "unknown";
#endif
}

bool PlatformAbstraction::copyFile(const std::string& source, const std::string& dest) {
    std::ifstream src(source, std::ios::binary);
    if (!src.is_open()) return false;
    std::ofstream dst(dest, std::ios::binary);
    if (!dst.is_open()) return false;
    dst << src.rdbuf();
    return dst.good();
}

bool PlatformAbstraction::moveFile(const std::string& source, const std::string& dest) {
    return std::rename(source.c_str(), dest.c_str()) == 0;
}

std::string PlatformAbstraction::joinPath(const std::string& path1, const std::string& path2) {
    if (path1.empty()) return path2;
    if (path2.empty()) return path1;
    if (path2[0] == '/' || path2[0] == '\\') return path2;
    char back = path1.back();
    if (back == '/' || back == '\\') return path1 + path2;
    return path1 + "/" + path2;
}

std::string PlatformAbstraction::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool PlatformAbstraction::writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << content;
    return file.good();
}

std::vector<char> PlatformAbstraction::readBinaryFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return {};
    std::streampos pos = file.tellg();
    if (pos <= 0) return {};
    std::streamsize size = static_cast<std::streamsize>(pos);
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(static_cast<size_t>(size));
    if (!file.read(buffer.data(), size)) {
        buffer.resize(static_cast<size_t>(file.gcount()));
    }
    return buffer;
}

bool PlatformAbstraction::writeBinaryFile(const std::string& path, const std::vector<char>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    if (!data.empty()) {
        file.write(data.data(), static_cast<std::streamsize>(data.size()));
    }
    return file.good();
}

ProcessInfo PlatformAbstraction::getCurrentProcessInfo() {
    ProcessInfo info;
    info.pid = getCurrentProcessId();
    info.name = "NexusForge";
    return info;
}

std::vector<std::string> PlatformAbstraction::getCommandLineArguments() {
    return {};
}

uint64_t PlatformAbstraction::getCurrentTimeMillis() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());
}

uint64_t PlatformAbstraction::getCurrentTimeMicros() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count());
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

void* PlatformAbstraction::createMainWindow(const std::string& title, int width, int height) {
    (void)title; (void)width; (void)height;
    return reinterpret_cast<void*>(1);
}

void* PlatformAbstraction::createSplashWindow(int width, int height) {
    (void)width; (void)height;
    return reinterpret_cast<void*>(2);
}

void PlatformAbstraction::destroyWindow(void* window) {
    (void)window;
}

void PlatformAbstraction::showWindow(void* window) {
    (void)window;
}

void PlatformAbstraction::hideWindow(void* window) {
    (void)window;
}

void PlatformAbstraction::setWindowTitle(void* window, const char* title) {
    (void)window; (void)title;
}

void PlatformAbstraction::setWindowSize(void* window, int width, int height) {
    (void)window; (void)width; (void)height;
}

void PlatformAbstraction::setWindowPosition(void* window, int x, int y) {
    (void)window; (void)x; (void)y;
}

void PlatformAbstraction::maximizeWindow(void* window) {
    (void)window;
}

void PlatformAbstraction::minimizeWindow(void* window) {
    (void)window;
}

void PlatformAbstraction::restoreWindow(void* window) {
    (void)window;
}

void PlatformAbstraction::setWindowFullscreen(void* window, bool fullscreen) {
    (void)window; (void)fullscreen;
}

bool PlatformAbstraction::isWindowFullscreen(void* window) {
    (void)window;
    return false;
}

void* PlatformAbstraction::getNativeWindowHandle(void* window) {
    return window;
}

} // namespace NexusForge::Platform

#endif // !NEXUS_PLATFORM_LINUX
