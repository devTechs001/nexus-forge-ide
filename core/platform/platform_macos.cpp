// core/platform/platform_macos.cpp
#include "platform_abstraction.hpp"

#ifdef NEXUS_PLATFORM_MACOS

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <pwd.h>
#include <dlfcn.h>

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
    return PlatformType::MacOS;
}

std::string PlatformAbstraction::getPlatformName() {
    return "macOS";
}

std::string PlatformAbstraction::getOSVersion() {
    struct utsname buffer;
    uname(&buffer);
    return std::string(buffer.release);
}

std::string PlatformAbstraction::getCPUInfo() {
    char buffer[256];
    size_t size = sizeof(buffer);
    if (sysctlbyname("machdep.cpu.brand_string", &buffer, &size, nullptr, 0) == 0) {
        return std::string(buffer);
    }
    return "Unknown";
}

size_t PlatformAbstraction::getTotalMemory() {
    int mib[2] = { CTL_HW, HW_MEMSIZE };
    uint64_t memory = 0;
    size_t size = sizeof(memory);
    sysctl(mib, 2, &memory, &size, nullptr, 0);
    return memory;
}

size_t PlatformAbstraction::getAvailableMemory() {
    // Placeholder
    return getTotalMemory() / 2;
}

std::string PlatformAbstraction::getCurrentDirectory() {
    char path[PATH_MAX];
    getcwd(path, sizeof(path));
    return std::string(path);
}

bool PlatformAbstraction::setCurrentDirectory(const std::string& path) {
    return chdir(path.c_str()) == 0;
}

bool PlatformAbstraction::fileExists(const std::string& path) {
    return access(path.c_str(), F_OK) == 0;
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
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        return std::string(path);
    }
    return "";
}

std::string PlatformAbstraction::getUserDataPath() {
    const char* home = getenv("HOME");
    if (!home) {
        home = getpwuid(getuid())->pw_dir;
    }
    return std::string(home) + "/Library/Application Support/NexusForge";
}

std::string PlatformAbstraction::getTempPath() {
    return std::string(NSTemporaryDirectory());
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

std::vector<uint8_t> PlatformAbstraction::readBinaryFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return {};
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

bool PlatformAbstraction::writeBinaryFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
}

WindowHandle PlatformAbstraction::createWindow(int width, int height, const std::string& title) {
    return nullptr;
}

bool PlatformAbstraction::createSplashWindow(int width, int height, WindowHandle* handle) {
    *handle = nullptr;
    return true;
}

void PlatformAbstraction::destroyWindow(WindowHandle window) {}
void PlatformAbstraction::setWindowTitle(WindowHandle window, const std::string& title) {}
void PlatformAbstraction::setWindowSize(WindowHandle window, int width, int height) {}
void PlatformAbstraction::setWindowPosition(WindowHandle window, int x, int y) {}
void PlatformAbstraction::showWindow(WindowHandle window) {}
void PlatformAbstraction::hideWindow(WindowHandle window) {}
void PlatformAbstraction::maximizeWindow(WindowHandle window) {}
void PlatformAbstraction::minimizeWindow(WindowHandle window) {}
bool PlatformAbstraction::isWindowMaximized(WindowHandle window) { return false; }
bool PlatformAbstraction::isWindowMinimized(WindowHandle window) { return false; }
void* PlatformAbstraction::getNativeWindowHandle(WindowHandle window) { return window; }

void PlatformAbstraction::processEvents() {}
void PlatformAbstraction::pollEvents() {}
void PlatformAbstraction::waitEvents() {}

std::string PlatformAbstraction::getClipboardText() { return ""; }
void PlatformAbstraction::setClipboardText(const std::string& text) {}

void* PlatformAbstraction::loadLibrary(const std::string& path) {
    return dlopen(path.c_str(), RTLD_LAZY);
}

void PlatformAbstraction::unloadLibrary(void* handle) {
    if (handle) dlclose(handle);
}

void* PlatformAbstraction::getProcAddress(void* handle, const std::string& name) {
    if (handle) return dlsym(handle, name.c_str());
    return nullptr;
}

int PlatformAbstraction::executeCommand(const std::string& command, std::string& output) { return 0; }

bool PlatformAbstraction::openFile(const std::string& path) {
    NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
    [[NSWorkspace sharedWorkspace] openFile:nsPath];
    return true;
}

bool PlatformAbstraction::openURL(const std::string& url) {
    NSString* nsUrl = [NSString stringWithUTF8String:url.c_str()];
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:nsUrl]];
    return true;
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

void PlatformAbstraction::yieldThread() { std::this_thread::yield(); }

void PlatformAbstraction::log(const std::string& message) {
    NSLog(@"[INFO] %s", message.c_str());
}

void PlatformAbstraction::logError(const std::string& message) {
    NSLog(@"[ERROR] %s", message.c_str());
}

void PlatformAbstraction::logWarning(const std::string& message) {
    NSLog(@"[WARNING] %s", message.c_str());
}

} // namespace NexusForge::Platform

#endif // NEXUS_PLATFORM_MACOS
