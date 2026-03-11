// core/platform/platform_android.cpp
#include "platform_abstraction.hpp"

#ifdef NEXUS_PLATFORM_ANDROID

#include <android/native_activity.h>
#include <android/log.h>
#include <jni.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>

namespace NexusForge::Platform {

static ANativeActivity* g_activity = nullptr;
static JavaVM* g_vm = nullptr;
static bool g_initialized = false;

bool PlatformAbstraction::initialize() {
    g_initialized = true;
    return true;
}

void PlatformAbstraction::shutdown() {
    g_initialized = false;
}

PlatformType PlatformAbstraction::getPlatformType() {
    return PlatformType::Android;
}

std::string PlatformAbstraction::getPlatformName() {
    return "Android";
}

std::string PlatformAbstraction::getOSVersion() {
    return "Android API " + std::to_string(__ANDROID_API__);
}

std::string PlatformAbstraction::getCPUInfo() {
    return "ARM/ARM64";
}

size_t PlatformAbstraction::getTotalMemory() {
    // Placeholder
    return 2 * 1024 * 1024 * 1024;
}

size_t PlatformAbstraction::getAvailableMemory() {
    return getTotalMemory() / 2;
}

std::string PlatformAbstraction::getCurrentDirectory() {
    return ".";
}

bool PlatformAbstraction::setCurrentDirectory(const std::string& path) {
    return chdir(path.c_str()) == 0;
}

bool PlatformAbstraction::fileExists(const std::string& path) {
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0;
}

bool PlatformAbstraction::directoryExists(const std::string& path) {
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode);
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
    }
    return stats;
}

std::string PlatformAbstraction::getExecutablePath() {
    return "/data/data/com.nexusforge.ide";
}

std::string PlatformAbstraction::getUserDataPath() {
    return "/data/data/com.nexusforge.ide/files";
}

std::string PlatformAbstraction::getTempPath() {
    return "/data/data/com.nexusforge.ide/cache";
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
bool PlatformAbstraction::openFile(const std::string& path) { return false; }
bool PlatformAbstraction::openURL(const std::string& url) { return false; }

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
    __android_log_print(ANDROID_LOG_INFO, "NexusForge", "%s", message.c_str());
}

void PlatformAbstraction::logError(const std::string& message) {
    __android_log_print(ANDROID_LOG_ERROR, "NexusForge", "%s", message.c_str());
}

void PlatformAbstraction::logWarning(const std::string& message) {
    __android_log_print(ANDROID_LOG_WARN, "NexusForge", "%s", message.c_str());
}

} // namespace NexusForge::Platform

#endif // NEXUS_PLATFORM_ANDROID
