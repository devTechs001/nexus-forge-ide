// core/platform/platform_windows.cpp
#include "platform_abstraction.hpp"

#ifdef NEXUS_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <direct.h>
#include <io.h>
#include <psapi.h>
#include <commdlg.h>

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
    return PlatformType::Windows;
}

std::string PlatformAbstraction::getPlatformName() {
    return "Windows";
}

std::string PlatformAbstraction::getOSVersion() {
    OSVERSIONINFOEX osvi = { sizeof(OSVERSIONINFOEX) };
    GetVersionEx((OSVERSIONINFO*)&osvi);
    return std::to_string(osvi.dwMajorVersion) + "." + 
           std::to_string(osvi.dwMinorVersion) + " Build " + 
           std::to_string(osvi.dwBuildNumber);
}

std::string PlatformAbstraction::getCPUInfo() {
    char brand[0x40] = { 0 };
    int cpuInfo[4];
    __cpuid(cpuInfo, 0x80000002);
    memcpy(brand, cpuInfo, sizeof(cpuInfo));
    __cpuid(cpuInfo, 0x80000003);
    memcpy(brand + 16, cpuInfo, sizeof(cpuInfo));
    __cpuid(cpuInfo, 0x80000004);
    memcpy(brand + 32, cpuInfo, sizeof(cpuInfo));
    return std::string(brand);
}

size_t PlatformAbstraction::getTotalMemory() {
    MEMORYSTATUSEX status = { sizeof(MEMORYSTATUSEX) };
    GlobalMemoryStatusEx(&status);
    return status.ullTotalPhys;
}

size_t PlatformAbstraction::getAvailableMemory() {
    MEMORYSTATUSEX status = { sizeof(MEMORYSTATUSEX) };
    GlobalMemoryStatusEx(&status);
    return status.ullAvailPhys;
}

std::string PlatformAbstraction::getCurrentDirectory() {
    char path[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, path);
    return std::string(path);
}

bool PlatformAbstraction::setCurrentDirectory(const std::string& path) {
    return SetCurrentDirectory(path.c_str()) != FALSE;
}

bool PlatformAbstraction::fileExists(const std::string& path) {
    return GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool PlatformAbstraction::directoryExists(const std::string& path) {
    DWORD attrs = GetFileAttributes(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool PlatformAbstraction::createDirectory(const std::string& path) {
    return CreateDirectory(path.c_str(), nullptr) != FALSE || directoryExists(path);
}

bool PlatformAbstraction::deleteFile(const std::string& path) {
    return DeleteFile(path.c_str()) != FALSE;
}

bool PlatformAbstraction::deleteDirectory(const std::string& path) {
    return RemoveDirectory(path.c_str()) != FALSE;
}

std::vector<std::string> PlatformAbstraction::listDirectory(const std::string& path) {
    std::vector<std::string> entries;
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile((path + "/*").c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
                entries.push_back(findData.cFileName);
            }
        } while (FindNextFile(hFind, &findData));
        FindClose(hFind);
    }
    return entries;
}

FileStats PlatformAbstraction::getFileStats(const std::string& path) {
    FileStats stats = {};
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesEx(path.c_str(), GetFileExInfoStandard, &data)) {
        stats.size = ((size_t)data.nFileSizeHigh << 32) | data.nFileSizeLow;
        stats.isDirectory = data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        stats.isFile = !(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        // Time conversion would go here
    }
    return stats;
}

std::string PlatformAbstraction::getExecutablePath() {
    char path[MAX_PATH];
    GetModuleFileName(nullptr, path, MAX_PATH);
    return std::string(path);
}

std::string PlatformAbstraction::getUserDataPath() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        return std::string(path) + "\\NexusForge";
    }
    return "";
}

std::string PlatformAbstraction::getTempPath() {
    char path[MAX_PATH];
    GetTempPath(MAX_PATH, path);
    return std::string(path);
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
    // Placeholder - actual implementation would use Win32 API
    return nullptr;
}

bool PlatformAbstraction::createSplashWindow(int width, int height, WindowHandle* handle) {
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
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void PlatformAbstraction::pollEvents() {
    // Placeholder
}

void PlatformAbstraction::waitEvents() {
    WaitMessage();
}

std::string PlatformAbstraction::getClipboardText() {
    if (!OpenClipboard(nullptr)) return "";
    HANDLE hData = GetClipboardData(CF_TEXT);
    if (!hData) { CloseClipboard(); return ""; }
    char* data = (char*)GlobalLock(hData);
    std::string result(data ? data : "");
    GlobalUnlock(hData);
    CloseClipboard();
    return result;
}

void PlatformAbstraction::setClipboardText(const std::string& text) {
    if (!OpenClipboard(nullptr)) return;
    EmptyClipboard();
    HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (hData) {
        char* data = (char*)GlobalLock(hData);
        strcpy(data, text.c_str());
        GlobalUnlock(hData);
        SetClipboardData(CF_TEXT, hData);
    }
    CloseClipboard();
}

void* PlatformAbstraction::loadLibrary(const std::string& path) {
    return LoadLibrary(path.c_str());
}

void PlatformAbstraction::unloadLibrary(void* handle) {
    if (handle) FreeLibrary((HMODULE)handle);
}

void* PlatformAbstraction::getProcAddress(void* handle, const std::string& name) {
    if (handle) return GetProcAddress((HMODULE)handle, name.c_str());
    return nullptr;
}

int PlatformAbstraction::executeCommand(const std::string& command, std::string& output) {
    // Placeholder - would use _popen
    return 0;
}

bool PlatformAbstraction::openFile(const std::string& path) {
    return ShellExecute(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOW) > (HINSTANCE)32;
}

bool PlatformAbstraction::openURL(const std::string& url) {
    return ShellExecute(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOW) > (HINSTANCE)32;
}

std::string PlatformAbstraction::getEnvironmentVariable(const std::string& name) {
    char buffer[32767];
    DWORD size = GetEnvironmentVariable(name.c_str(), buffer, sizeof(buffer));
    return size > 0 ? std::string(buffer) : "";
}

bool PlatformAbstraction::setEnvironmentVariable(const std::string& name, const std::string& value) {
    return SetEnvironmentVariable(name.c_str(), value.c_str()) != FALSE;
}

uint64_t PlatformAbstraction::getCurrentTimeMs() {
    return GetTickCount64();
}

uint64_t PlatformAbstraction::getHighResolutionTime() {
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (count.QuadPart * 1000000000ULL) / freq.QuadPart;
}

void PlatformAbstraction::sleep(int milliseconds) {
    Sleep(milliseconds);
}

void PlatformAbstraction::yieldThread() {
    Sleep(0);
}

void PlatformAbstraction::log(const std::string& message) {
    OutputDebugString(("NexusForge: " + message + "\n").c_str());
}

void PlatformAbstraction::logError(const std::string& message) {
    OutputDebugString(("NexusForge ERROR: " + message + "\n").c_str());
}

void PlatformAbstraction::logWarning(const std::string& message) {
    OutputDebugString(("NexusForge WARNING: " + message + "\n").c_str());
}

} // namespace NexusForge::Platform

#endif // NEXUS_PLATFORM_WINDOWS
