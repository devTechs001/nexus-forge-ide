// core/platform/platform_web.cpp
#include "platform_abstraction.hpp"

#ifdef NEXUS_PLATFORM_WEB

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <fstream>
#include <sstream>
#include <iostream>

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
    return PlatformType::Web;
}

std::string PlatformAbstraction::getPlatformName() {
    return "Web (Emscripten)";
}

std::string PlatformAbstraction::getOSVersion() {
    return "WebAssembly";
}

std::string PlatformAbstraction::getCPUInfo() {
    return "WebAssembly";
}

size_t PlatformAbstraction::getTotalMemory() {
    return emscripten_get_heap_max();
}

size_t PlatformAbstraction::getAvailableMemory() {
    return emscripten_get_heap_size() - emscripten_get_heap_max() / 2;
}

std::string PlatformAbstraction::getCurrentDirectory() {
    return "/";
}

bool PlatformAbstraction::setCurrentDirectory(const std::string& path) {
    return true;
}

bool PlatformAbstraction::fileExists(const std::string& path) {
    EM_ASM({
        return FS.analyzePath(UTF8ToString($0)).exists ? 1 : 0;
    }, path.c_str());
    return false;
}

bool PlatformAbstraction::directoryExists(const std::string& path) {
    return EM_ASM_INT({
        try {
            return FS.analyzePath(UTF8ToString($0)).object.node.isFolder ? 1 : 0;
        } catch(e) {
            return 0;
        }
    }, path.c_str());
}

bool PlatformAbstraction::createDirectory(const std::string& path) {
    EM_ASM({
        try {
            FS.mkdir(UTF8ToString($0));
        } catch(e) {}
    }, path.c_str());
    return true;
}

bool PlatformAbstraction::deleteFile(const std::string& path) {
    EM_ASM({
        try {
            FS.unlink(UTF8ToString($0));
        } catch(e) {}
    }, path.c_str());
    return true;
}

bool PlatformAbstraction::deleteDirectory(const std::string& path) {
    EM_ASM({
        try {
            FS.rmdir(UTF8ToString($0));
        } catch(e) {}
    }, path.c_str());
    return true;
}

std::vector<std::string> PlatformAbstraction::listDirectory(const std::string& path) {
    std::vector<std::string> entries;
    // Web implementation would use Emscripten FS APIs
    return entries;
}

FileStats PlatformAbstraction::getFileStats(const std::string& path) {
    FileStats stats = {};
    // Web implementation would use Emscripten FS APIs
    return stats;
}

std::string PlatformAbstraction::getExecutablePath() {
    return "/nexusforge.wasm";
}

std::string PlatformAbstraction::getUserDataPath() {
    return "/home/web_user/nexusforge";
}

std::string PlatformAbstraction::getTempPath() {
    return "/tmp";
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
void PlatformAbstraction::setWindowTitle(WindowHandle window, const std::string& title) {
    EM_ASM({
        document.title = UTF8ToString($0);
    }, title.c_str());
}

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
    // Web clipboard API would be used here
    return "";
}

void PlatformAbstraction::setClipboardText(const std::string& text) {
    EM_ASM({
        navigator.clipboard.writeText(UTF8ToString($0));
    }, text.c_str());
}

void* PlatformAbstraction::loadLibrary(const std::string& path) {
    // WebAssembly doesn't support dynamic loading in the traditional sense
    return nullptr;
}

void PlatformAbstraction::unloadLibrary(void* handle) {}

void* PlatformAbstraction::getProcAddress(void* handle, const std::string& name) {
    return nullptr;
}

int PlatformAbstraction::executeCommand(const std::string& command, std::string& output) {
    return 0;
}

bool PlatformAbstraction::openFile(const std::string& path) {
    return false;
}

bool PlatformAbstraction::openURL(const std::string& url) {
    EM_ASM({
        window.open(UTF8ToString($0), '_blank');
    }, url.c_str());
    return true;
}

std::string PlatformAbstraction::getEnvironmentVariable(const std::string& name) {
    // Web environment variables are limited
    return "";
}

bool PlatformAbstraction::setEnvironmentVariable(const std::string& name, const std::string& value) {
    return false;
}

uint64_t PlatformAbstraction::getCurrentTimeMs() {
    return emscripten_get_now();
}

uint64_t PlatformAbstraction::getHighResolutionTime() {
    return emscripten_get_now() * 1000000;
}

void PlatformAbstraction::sleep(int milliseconds) {
    emscripten_sleep(milliseconds);
}

void PlatformAbstraction::yieldThread() {}

void PlatformAbstraction::log(const std::string& message) {
    EM_ASM({
        console.log("[INFO] " + UTF8ToString($0));
    }, message.c_str());
}

void PlatformAbstraction::logError(const std::string& message) {
    EM_ASM({
        console.error("[ERROR] " + UTF8ToString($0));
    }, message.c_str());
}

void PlatformAbstraction::logWarning(const std::string& message) {
    EM_ASM({
        console.warn("[WARNING] " + UTF8ToString($0));
    }, message.c_str());
}

} // namespace NexusForge::Platform

#endif // NEXUS_PLATFORM_WEB
