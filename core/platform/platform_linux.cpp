// core/platform/platform_linux.cpp
#include "platform_abstraction.hpp"

#ifdef NEXUS_PLATFORM_LINUX

#include <unistd.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <dirent.h>
#include <dlfcn.h>
#include <pwd.h>
#include <libgen.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <climits>

namespace NexusForge::Platform {

static bool g_initialized = false;

bool PlatformAbstraction::initialize() {
    g_initialized = true;
    return true;
}

void PlatformAbstraction::shutdown() {
    g_initialized = false;
}

PlatformType PlatformAbstraction::getPlatformType() {
    return PlatformType::Linux;
}

std::string PlatformAbstraction::getPlatformName() {
    return "Linux";
}

std::string PlatformAbstraction::getOSVersion() {
    struct utsname buffer;
    if (uname(&buffer) == 0) {
        return std::string(buffer.release) + " " + std::string(buffer.version);
    }
    return "Unknown";
}

std::string PlatformAbstraction::getCPUInfo() {
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                return line.substr(pos + 2);
            }
        }
    }
    return "Unknown";
}

size_t PlatformAbstraction::getTotalMemory() {
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return info.totalram * info.mem_unit;
    }
    return 0;
}

size_t PlatformAbstraction::getAvailableMemory() {
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return info.freeram * info.mem_unit;
    }
    return 0;
}

std::string PlatformAbstraction::getCurrentDirectory() {
    char path[PATH_MAX];
    if (getcwd(path, sizeof(path))) {
        return std::string(path);
    }
    return ".";
}

bool PlatformAbstraction::setCurrentDirectory(const std::string& path) {
    return chdir(path.c_str()) == 0;
}

bool PlatformAbstraction::fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
}

bool PlatformAbstraction::directoryExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
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

std::vector<std::string> PlatformAbstraction::listDirectory(const std::string& path) {
    std::vector<std::string> entries;
    DIR* dir = opendir(path.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                entries.push_back(entry->d_name);
            }
        }
        closedir(dir);
    }
    return entries;
}

FileStats PlatformAbstraction::getFileStats(const std::string& path) {
    FileStats stats = {};
    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0) {
        stats.size = buffer.st_size;
        stats.createdTime = buffer.st_ctime;
        stats.modifiedTime = buffer.st_mtime;
        stats.accessedTime = buffer.st_atime;
        stats.isDirectory = S_ISDIR(buffer.st_mode);
        stats.isFile = S_ISREG(buffer.st_mode);
        stats.isSymlink = S_ISLNK(buffer.st_mode);
    }
    return stats;
}

std::string PlatformAbstraction::getExecutablePath() {
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count != -1) {
        path[count] = '\0';
        return std::string(path);
    }
    return "";
}

std::string PlatformAbstraction::getUserDataPath() {
    const char* home = getenv("HOME");
    if (!home) {
        home = getpwuid(getuid())->pw_dir;
    }
    return std::string(home) + "/.config/NexusForge";
}

std::string PlatformAbstraction::getTempPath() {
    const char* temp = getenv("TMPDIR");
    if (!temp) temp = "/tmp";
    return std::string(temp);
}

std::string PlatformAbstraction::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool PlatformAbstraction::writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    file << content;
    return true;
}

std::vector<uint8_t> PlatformAbstraction::readBinaryFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return {};
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

bool PlatformAbstraction::writeBinaryFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
}

WindowHandle PlatformAbstraction::createWindow(int width, int height, const std::string& title) {
    // Placeholder - actual implementation would use X11/Wayland
    return nullptr;
}

bool PlatformAbstraction::createSplashWindow(int width, int height, WindowHandle* handle) {
    // Placeholder
    *handle = nullptr;
    return true;
}

void PlatformAbstraction::destroyWindow(WindowHandle window) {
    // Placeholder
}

void PlatformAbstraction::setWindowTitle(WindowHandle window, const std::string& title) {
    // Placeholder
}

void PlatformAbstraction::setWindowSize(WindowHandle window, int width, int height) {
    // Placeholder
}

void PlatformAbstraction::setWindowPosition(WindowHandle window, int x, int y) {
    // Placeholder
}

void PlatformAbstraction::showWindow(WindowHandle window) {
    // Placeholder
}

void PlatformAbstraction::hideWindow(WindowHandle window) {
    // Placeholder
}

void PlatformAbstraction::maximizeWindow(WindowHandle window) {
    // Placeholder
}

void PlatformAbstraction::minimizeWindow(WindowHandle window) {
    // Placeholder
}

bool PlatformAbstraction::isWindowMaximized(WindowHandle window) {
    return false;
}

bool PlatformAbstraction::isWindowMinimized(WindowHandle window) {
    return false;
}

void* PlatformAbstraction::getNativeWindowHandle(WindowHandle window) {
    return window;
}

void PlatformAbstraction::processEvents() {
    // Placeholder - would use X11/Wayland event loop
}

void PlatformAbstraction::pollEvents() {
    // Placeholder
}

void PlatformAbstraction::waitEvents() {
    // Placeholder
}

std::string PlatformAbstraction::getClipboardText() {
    // Placeholder - would use X11 clipboard
    return "";
}

void PlatformAbstraction::setClipboardText(const std::string& text) {
    // Placeholder - would use X11 clipboard
}

void* PlatformAbstraction::loadLibrary(const std::string& path) {
    return dlopen(path.c_str(), RTLD_LAZY);
}

void PlatformAbstraction::unloadLibrary(void* handle) {
    if (handle) {
        dlclose(handle);
    }
}

void* PlatformAbstraction::getProcAddress(void* handle, const std::string& name) {
    if (handle) {
        return dlsym(handle, name.c_str());
    }
    return nullptr;
}

int PlatformAbstraction::executeCommand(const std::string& command, std::string& output) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        return -1;
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    output = result;
    return 0;
}

bool PlatformAbstraction::openFile(const std::string& path) {
    std::string cmd = "xdg-open \"" + path + "\"";
    return system(cmd.c_str()) == 0;
}

bool PlatformAbstraction::openURL(const std::string& url) {
    std::string cmd = "xdg-open \"" + url + "\"";
    return system(cmd.c_str()) == 0;
}

std::string PlatformAbstraction::getEnvironmentVariable(const std::string& name) {
    const char* value = getenv(name.c_str());
    return value ? std::string(value) : "";
}

bool PlatformAbstraction::setEnvironmentVariable(const std::string& name, const std::string& value) {
    return setenv(name.c_str(), value.c_str(), 1) == 0;
}

uint64_t PlatformAbstraction::getCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

uint64_t PlatformAbstraction::getHighResolutionTime() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

void PlatformAbstraction::sleep(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void PlatformAbstraction::yieldThread() {
    std::this_thread::yield();
}

void PlatformAbstraction::log(const std::string& message) {
    std::cout << "[INFO] " << message << std::endl;
}

void PlatformAbstraction::logError(const std::string& message) {
    std::cerr << "[ERROR] " << message << std::endl;
}

void PlatformAbstraction::logWarning(const std::string& message) {
    std::cerr << "[WARNING] " << message << std::endl;
}

} // namespace NexusForge::Platform

#endif // NEXUS_PLATFORM_LINUX
