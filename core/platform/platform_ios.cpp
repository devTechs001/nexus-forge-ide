// core/platform/platform_ios.cpp
#include "platform_abstraction.hpp"

#ifdef NEXUS_PLATFORM_IOS

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <fstream>
#include <sstream>

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
    return PlatformType::iOS;
}

std::string PlatformAbstraction::getPlatformName() {
    return "iOS";
}

std::string PlatformAbstraction::getOSVersion() {
    return [[[UIDevice currentDevice] systemVersion] UTF8String];
}

std::string PlatformAbstraction::getCPUInfo() {
    return "ARM64";
}

size_t PlatformAbstraction::getTotalMemory() {
    return [[NSProcessInfo processInfo] physicalMemory];
}

size_t PlatformAbstraction::getAvailableMemory() {
    return [[NSProcessInfo processInfo] activeMemory];
}

std::string PlatformAbstraction::getCurrentDirectory() {
    return std::string([[NSFileManager defaultManager] currentDirectoryPath].UTF8String);
}

bool PlatformAbstraction::setCurrentDirectory(const std::string& path) {
    return [[NSFileManager defaultManager] changeCurrentDirectoryPath:[NSString stringWithUTF8String:path.c_str()]];
}

bool PlatformAbstraction::fileExists(const std::string& path) {
    return [[NSFileManager defaultManager] fileExistsAtPath:[NSString stringWithUTF8String:path.c_str()]];
}

bool PlatformAbstraction::directoryExists(const std::string& path) {
    NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
    BOOL isDirectory;
    return [[NSFileManager defaultManager] fileExistsAtPath:nsPath isDirectory:&isDirectory] && isDirectory;
}

bool PlatformAbstraction::createDirectory(const std::string& path) {
    NSError* error = nil;
    return [[NSFileManager defaultManager] createDirectoryAtPath:[NSString stringWithUTF8String:path.c_str()]
                                     withIntermediateDirectories:YES
                                                       attributes:nil
                                                            error:&error];
}

bool PlatformAbstraction::deleteFile(const std::string& path) {
    NSError* error = nil;
    return [[NSFileManager defaultManager] removeItemAtPath:[NSString stringWithUTF8String:path.c_str()] error:&error];
}

bool PlatformAbstraction::deleteDirectory(const std::string& path) {
    return deleteFile(path);
}

std::vector<std::string> PlatformAbstraction::listDirectory(const std::string& path) {
    std::vector<std::string> entries;
    NSError* error = nil;
    NSArray* contents = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:[NSString stringWithUTF8String:path.c_str()] error:&error];
    if (contents) {
        for (NSString* item in contents) {
            entries.push_back([item UTF8String]);
        }
    }
    return entries;
}

FileStats PlatformAbstraction::getFileStats(const std::string& path) {
    FileStats stats = {};
    NSError* error = nil;
    NSDictionary* attrs = [[NSFileManager defaultManager] attributesOfItemAtPath:[NSString stringWithUTF8String:path.c_str()] error:&error];
    if (attrs) {
        stats.size = [[attrs fileSize] longLongValue];
        stats.isDirectory = [[attrs fileType] isEqualToString:NSFileTypeDirectory];
        stats.isFile = [[attrs fileType] isEqualToString:NSFileTypeRegular];
    }
    return stats;
}

std::string PlatformAbstraction::getExecutablePath() {
    return std::string([[[NSBundle mainBundle] executablePath] UTF8String]);
}

std::string PlatformAbstraction::getUserDataPath() {
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    return std::string([[paths firstObject] UTF8String]);
}

std::string PlatformAbstraction::getTempPath() {
    return std::string([NSTemporaryDirectory() UTF8String]);
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

std::string PlatformAbstraction::getClipboardText() {
    return std::string([[UIPasteboard generalPasteboard].string UTF8String]);
}

void PlatformAbstraction::setClipboardText(const std::string& text) {
    [UIPasteboard generalPasteboard].string = [NSString stringWithUTF8String:text.c_str()];
}

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
    NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
    return [[UIApplication sharedApplication] canOpenURL:url];
}

bool PlatformAbstraction::openURL(const std::string& url) {
    return [[UIApplication sharedApplication] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:url.c_str()]]];
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

#endif // NEXUS_PLATFORM_IOS
