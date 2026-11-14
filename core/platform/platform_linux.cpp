// core/platform/platform_linux.cpp
#include "platform_abstraction.hpp"
#include <chrono>
#include <cstdint>
#include <cstdio>

#ifdef NEXUS_PLATFORM_LINUX

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <array>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <climits>

namespace NexusForge::Platform {

PlatformType PlatformAbstraction::currentPlatform_ = PlatformType::Linux;
bool PlatformAbstraction::initialized_ = false;

bool PlatformAbstraction::initialize() {
    initialized_ = true;
    return true;
}

void PlatformAbstraction::shutdown() {
    initialized_ = false;
}

PlatformType PlatformAbstraction::getCurrentPlatform() {
    return currentPlatform_;
}

SystemInfo PlatformAbstraction::getSystemInfo() {
    SystemInfo info;
    info.platform = PlatformType::Linux;
    
    struct utsname unameData;
    if (uname(&unameData) == 0) {
        info.osName = unameData.sysname;
        info.osVersion = unameData.release;
        info.architecture = unameData.machine;
    }
    
    info.cpuCount = static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
    
    struct sysinfo sysInfo;
    if (sysinfo(&sysInfo) == 0) {
        info.totalMemory = sysInfo.totalram * sysInfo.mem_unit;
        info.availableMemory = sysInfo.freeram * sysInfo.mem_unit;
    }
    
    return info;
}

DirectoryPaths PlatformAbstraction::getDirectoryPaths() {
    DirectoryPaths paths;
    
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
    paths.documentsDir = paths.homeDir + "/Documents";
    paths.downloadsDir = paths.homeDir + "/Downloads";
    paths.appDataDir = paths.configDir + "/NexusForge";
    
    return paths;
}

std::string PlatformAbstraction::getPlatformName() {
    return "Linux";
}

bool PlatformAbstraction::fileExists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

bool PlatformAbstraction::directoryExists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool PlatformAbstraction::createDirectory(const std::string& path) {
    return mkdir(path.c_str(), 0755) == 0 || directoryExists(path);
}

bool PlatformAbstraction::deleteFile(const std::string& path) {
    return unlink(path.c_str()) == 0;
}

bool PlatformAbstraction::deleteDirectory(const std::string& path) {
    return rmdir(path.c_str()) == 0;
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
    return rename(source.c_str(), dest.c_str()) == 0;
}

std::vector<FileInfo> PlatformAbstraction::listDirectory(const std::string& path) {
    std::vector<FileInfo> files;
    DIR* dir = opendir(path.c_str());
    if (!dir) return files;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        
        FileInfo info;
        info.path = path + "/" + name;
        info.name = name;
        
        struct stat st;
        if (stat(info.path.c_str(), &st) == 0) {
            info.isDirectory = S_ISDIR(st.st_mode);
            info.isFile = S_ISREG(st.st_mode);
            info.size = st.st_size;
            info.createdTime = st.st_ctime;
            info.modifiedTime = st.st_mtime;
            info.accessedTime = st.st_atime;
            
            size_t dotPos = name.rfind('.');
            if (dotPos != std::string::npos) {
                info.extension = name.substr(dotPos);
            }
        }
        
        files.push_back(info);
    }
    closedir(dir);
    return files;
}

std::string PlatformAbstraction::getCurrentWorkingDirectory() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        return cwd;
    }
    return ".";
}

bool PlatformAbstraction::setCurrentWorkingDirectory(const std::string& path) {
    return chdir(path.c_str()) == 0;
}

std::string PlatformAbstraction::getAbsolutePath(const std::string& relativePath) {
    char* abs = realpath(relativePath.c_str(), nullptr);
    if (abs) {
        std::string result(abs);
        free(abs);
        return result;
    }
    return relativePath;
}

std::string PlatformAbstraction::joinPath(const std::string& path1, const std::string& path2) {
    if (path1.empty()) return path2;
    if (path2.empty()) return path1;
    if (path2[0] == '/') return path2;
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
    return true;
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

int PlatformAbstraction::getCurrentProcessId() {
    return getpid();
}

ProcessInfo PlatformAbstraction::getCurrentProcessInfo() {
    ProcessInfo info;
    info.pid = getpid();
    
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        info.path = path;
        size_t slashPos = info.path.rfind('/');
        info.name = (slashPos != std::string::npos) ? info.path.substr(slashPos + 1) : info.path;
    }
    
    return info;
}

bool PlatformAbstraction::openExternal(const std::string& url) {
    return system(("xdg-open \"" + url + "\"").c_str()) == 0;
}

bool PlatformAbstraction::openFileWithDefaultApp(const std::string& path) {
    return openExternal(path);
}

std::string PlatformAbstraction::executeCommand(const std::string& command, int* exitCode) {
    std::array<char, 128> buffer;
    std::string result;
    
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return "";
    
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    
    int status = pclose(pipe);
    if (exitCode) {
        *exitCode = WEXITSTATUS(status);
    }
    
    return result;
}

std::string PlatformAbstraction::getEnvironmentVariable(const std::string& name) {
    const char* value = getenv(name.c_str());
    return value ? value : "";
}

bool PlatformAbstraction::setEnvironmentVariable(const std::string& name, const std::string& value) {
    return setenv(name.c_str(), value.c_str(), 1) == 0;
}

std::vector<std::string> PlatformAbstraction::getCommandLineArguments() {
    // Implementation would read /proc/self/cmdline
    return {};
}

std::string PlatformAbstraction::getClipboardText() {
    // Would use X11/Wayland clipboard API
    return "";
}

void PlatformAbstraction::setClipboardText(const std::string& text) {
    // Would use X11/Wayland clipboard API
}

bool PlatformAbstraction::hasClipboardText() {
    return !getClipboardText().empty();
}

uint64_t PlatformAbstraction::getCurrentTimeMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

uint64_t PlatformAbstraction::getCurrentTimeMicros() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
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

std::string PlatformAbstraction::openFileDialog(const std::string& title,
                                                  const std::vector<std::string>& filters,
                                                  const std::string& defaultPath) {
    // Would use zenity or kdialog
    std::string cmd = "zenity --file-selection --title=\"" + title + "\"";
    if (!defaultPath.empty()) {
        cmd += " --filename=\"" + defaultPath + "\"";
    }
    return executeCommand(cmd);
}

std::string PlatformAbstraction::saveFileDialog(const std::string& title,
                                                  const std::vector<std::string>& filters,
                                                  const std::string& defaultPath) {
    std::string cmd = "zenity --file-selection --save --title=\"" + title + "\"";
    if (!defaultPath.empty()) {
        cmd += " --filename=\"" + defaultPath + "\"";
    }
    return executeCommand(cmd);
}

std::string PlatformAbstraction::selectDirectoryDialog(const std::string& title,
                                                        const std::string& defaultPath) {
    std::string cmd = "zenity --file-selection --directory --title=\"" + title + "\"";
    if (!defaultPath.empty()) {
        cmd += " --filename=\"" + defaultPath + "\"";
    }
    return executeCommand(cmd);
}

PlatformAbstraction::MessageBoxResult PlatformAbstraction::showMessageBox(
    const std::string& title,
    const std::string& message,
    MessageBoxType type,
    MessageBoxButton buttons) {
    
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
    if (exitCode == 1) return MessageBoxResult::Cancel;
    return MessageBoxResult::Cancel;
}

void PlatformAbstraction::setThreadName(const std::string& name) {
    // pthread_setname_np not portable, skipping
}

int PlatformAbstraction::getCurrentThreadId() {
    return gettid();
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
        struct passwd* pw = getpwuid(getuid());
        if (pw && pw->pw_name) {
            user = pw->pw_name;
        }
    }
    return user ? user : "unknown";
}

bool PlatformAbstraction::isNetworkAvailable() {
    return system("ping -c1 8.8.8.8 > /dev/null 2>&1") == 0;
}

// Window management implementations (stubs for now - would use GLFW/X11 in production)
void* PlatformAbstraction::createMainWindow(const std::string& title, int width, int height) {
    // In production, would create actual window using GLFW/X11
    return reinterpret_cast<void*>(1);  // Dummy handle
}

void* PlatformAbstraction::createSplashWindow(int width, int height) {
    return reinterpret_cast<void*>(2);  // Dummy handle
}

void PlatformAbstraction::destroyWindow(void* window) {
    // Would destroy actual window
}

void PlatformAbstraction::showWindow(void* window) {
    // Would show window
}

void PlatformAbstraction::hideWindow(void* window) {
    // Would hide window
}

void PlatformAbstraction::setWindowTitle(void* window, const char* title) {
    // Would set window title
}

void PlatformAbstraction::setWindowSize(void* window, int width, int height) {
    // Would set window size
}

void PlatformAbstraction::setWindowPosition(void* window, int x, int y) {
    // Would set window position
}

void PlatformAbstraction::maximizeWindow(void* window) {
    // Would maximize window
}

void PlatformAbstraction::minimizeWindow(void* window) {
    // Would minimize window
}

void PlatformAbstraction::restoreWindow(void* window) {
    // Would restore window
}

void PlatformAbstraction::setWindowFullscreen(void* window, bool fullscreen) {
    // Would set fullscreen
}

bool PlatformAbstraction::isWindowFullscreen(void* window) {
    return false;
}

void* PlatformAbstraction::getNativeWindowHandle(void* window) {
    return window;
}

} // namespace NexusForge::Platform

#endif // NEXUS_PLATFORM_LINUX
