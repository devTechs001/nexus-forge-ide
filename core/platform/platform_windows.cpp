// core/platform/platform_windows.cpp
#include "platform_abstraction.hpp"

#ifdef NEXUS_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <commdlg.h>
#include <direct.h>
#include <io.h>
#include <process.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ole32.lib")

namespace NexusForge::Platform {

PlatformType getCurrentPlatform() {
    return PlatformType::Windows;
}

std::string getPlatformName() {
    return "Windows";
}

bool isPlatform(PlatformType platform) {
    return platform == PlatformType::Windows;
}

bool initialize() {
    return true;
}

void shutdown() {
}

void processEvents() {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void pumpEvents() {
    processEvents();
}

void* createMainWindow(int width, int height, void** nativeWindow) {
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "NexusForgeWindow";
    RegisterClassEx(&wc);

    HWND hwnd = CreateWindowEx(
        0, "NexusForgeWindow", "NexusForge IDE",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    if (nativeWindow) *nativeWindow = hwnd;
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return hwnd;
}

void* createSplashWindow(int width, int height, void** nativeWindow) {
    return createMainWindow(width, height, nativeWindow);
}

void destroyWindow(void* window) {
    if (window) {
        DestroyWindow(reinterpret_cast<HWND>(window));
    }
}

void showWindow(void* window) {
    if (window) ShowWindow(reinterpret_cast<HWND>(window), SW_SHOW);
}

void hideWindow(void* window) {
    if (window) ShowWindow(reinterpret_cast<HWND>(window), SW_HIDE);
}

void setWindowTitle(void* window, const char* title) {
    if (window) SetWindowTextA(reinterpret_cast<HWND>(window), title);
}

void setWindowSize(void* window, int width, int height) {
    if (window) SetWindowPos(reinterpret_cast<HWND>(window), NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
}

void setWindowPosition(void* window, int x, int y) {
    if (window) SetWindowPos(reinterpret_cast<HWND>(window), NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void maximizeWindow(void* window) {
    if (window) ShowWindow(reinterpret_cast<HWND>(window), SW_MAXIMIZE);
}

void minimizeWindow(void* window) {
    if (window) ShowWindow(reinterpret_cast<HWND>(window), SW_MINIMIZE);
}

void restoreWindow(void* window) {
    if (window) ShowWindow(reinterpret_cast<HWND>(window), SW_RESTORE);
}

void setWindowFullscreen(void* window, bool fullscreen) {
    // Fullscreen implementation
}

std::string getExecutablePath() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return path;
}

std::string getCurrentDirectory() {
    char path[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, path);
    return path;
}

bool setCurrentDirectory(const std::string& path) {
    return SetCurrentDirectoryA(path.c_str()) != FALSE;
}

bool fileExists(const std::string& path) {
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool directoryExists(const std::string& path) {
    DWORD attrs = GetFileAttributesA(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool createDirectory(const std::string& path) {
    return CreateDirectoryA(path.c_str(), NULL) != FALSE;
}

bool deleteFile(const std::string& path) {
    return DeleteFileA(path.c_str()) != FALSE;
}

bool deleteDirectory(const std::string& path, bool recursive) {
    if (recursive) {
        return RemoveDirectoryA(path.c_str()) != FALSE;
    }
    return RemoveDirectoryA(path.c_str()) != FALSE;
}

std::string readTextFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool writeTextFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << content;
    return true;
}

std::vector<uint8_t> readBinaryFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return {};
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

bool writeBinaryFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
}

std::string joinPath(const std::string& base, const std::string& path) {
    if (base.empty()) return path;
    if (path.empty()) return base;
    if (path[0] == '\\' || path[0] == '/') return path;
    if (base.back() == '\\' || base.back() == '/') return base + path;
    return base + "\\" + path;
}

std::string getDirectoryName(const std::string& path) {
    size_t pos = path.find_last_of("\\/");
    if (pos == std::string::npos) return "";
    return path.substr(0, pos);
}

std::string getFileName(const std::string& path) {
    size_t pos = path.find_last_of("\\/");
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

std::string getExtension(const std::string& path) {
    size_t pos = path.find_last_of('.');
    if (pos == std::string::npos) return "";
    return path.substr(pos + 1);
}

std::string normalizePath(const std::string& path) {
    std::string result = path;
    std::replace(result.begin(), result.end(), '/', '\\');
    return result;
}

std::string getAbsolutePath(const std::string& relativePath) {
    if (relativePath.empty() || relativePath[1] == ':') return relativePath;
    return joinPath(getCurrentDirectory(), relativePath);
}

std::string getRelativePath(const std::string& basePath, const std::string& path) {
    return path;
}

std::string getEnvironmentVariable(const std::string& name) {
    char buffer[32767];
    DWORD size = GetEnvironmentVariableA(name.c_str(), buffer, sizeof(buffer));
    if (size > 0 && size < sizeof(buffer)) return buffer;
    return "";
}

bool setEnvironmentVariable(const std::string& name, const std::string& value) {
    return SetEnvironmentVariableA(name.c_str(), value.c_str()) != FALSE;
}

std::string getHomeDirectory() {
    return getEnvironmentVariable("USERPROFILE");
}

std::string getDataDirectory() {
    return getEnvironmentVariable("APPDATA");
}

std::string getConfigDirectory() {
    return getEnvironmentVariable("APPDATA");
}

std::string getTempDirectory() {
    return getEnvironmentVariable("TEMP");
}

std::string getClipboardText() {
    if (!OpenClipboard(NULL)) return "";
    HANDLE hData = GetClipboardData(CF_TEXT);
    if (!hData) { CloseClipboard(); return ""; }
    char* pText = static_cast<char*>(GlobalLock(hData));
    std::string result = pText ? pText : "";
    GlobalUnlock(hData);
    CloseClipboard();
    return result;
}

void setClipboardText(const std::string& text) {
    if (!OpenClipboard(NULL)) return;
    EmptyClipboard();
    HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (hData) {
        char* pText = static_cast<char*>(GlobalLock(hData));
        strcpy_s(pText, text.size() + 1, text.c_str());
        GlobalUnlock(hData);
        SetClipboardData(CF_TEXT, hData);
    }
    CloseClipboard();
}

bool hasClipboardText() {
    return IsClipboardFormatAvailable(CF_TEXT) != FALSE;
}

void setCursor(CursorType cursor) {
    // Cursor implementation
}

void showCursor() { ShowCursor(TRUE); }
void hideCursor() { ShowCursor(FALSE); }
bool isCursorVisible() { return true; }

uint64_t getCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

double getCurrentTimeSeconds() {
    return std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

void sleep(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void* createThread(void (*func)(void*), void* arg) {
    return _beginthread(func, 0, arg);
}

void joinThread(void* thread) {
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
}

void detachThread(void* thread) {
    CloseHandle(thread);
}

void yieldThread() {
    std::this_thread::yield();
}

void* loadLibrary(const std::string& path) {
    return LoadLibraryA(path.c_str());
}

void unloadLibrary(void* handle) {
    if (handle) FreeLibrary(reinterpret_cast<HMODULE>(handle));
}

void* getProcAddress(void* handle, const std::string& name) {
    return GetProcAddress(reinterpret_cast<HMODULE>(handle), name.c_str());
}

int getCurrentProcessId() { return GetCurrentProcessId(); }
int getCurrentThreadId() { return GetCurrentThreadId(); }

bool openUrl(const std::string& url) {
    return ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOW) > (HINSTANCE)32;
}

bool openFile(const std::string& path) {
    return ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOW) > (HINSTANCE)32;
}

bool openFolder(const std::string& path) {
    return ShellExecuteA(NULL, "explore", path.c_str(), NULL, NULL, SW_SHOW) > (HINSTANCE)32;
}

SystemInfo getSystemInfo() {
    SystemInfo info;
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    info.cpuCount = si.dwNumberOfProcessors;
    
    MEMORYSTATUSEX ms = { sizeof(MEMORYSTATUSEX) };
    GlobalMemoryStatusEx(&ms);
    info.totalMemory = ms.ullTotalPhys;
    
    char hostname[256];
    DWORD size = sizeof(hostname);
    GetComputerNameA(hostname, &size);
    info.machineName = hostname;
    info.osVersion = "Windows";
    
    return info;
}

PowerStatus getPowerStatus() {
    SYSTEM_POWER_STATUS ps;
    if (GetSystemPowerStatus(&ps)) {
        if (ps.ACLineStatus == 1) return PowerStatus::AC;
        if (ps.BatteryFlag < 128) return PowerStatus::Battery;
    }
    return PowerStatus::Unknown;
}

int getBatteryLevel() {
    SYSTEM_POWER_STATUS ps;
    if (GetSystemPowerStatus(&ps) && ps.BatteryFlag < 128) {
        return ps.BatteryLifePercent;
    }
    return -1;
}

void showNotification(const std::string& title, const std::string& message) {
    // Windows notification
}

void showWarning(const std::string& title, const std::string& message) {
    showNotification(title, message);
}

void showError(const std::string& title, const std::string& message) {
    showNotification(title, message);
}

std::string showOpenFileDialog(const std::string& title,
                                const std::vector<std::pair<std::string, std::string>>& filters) {
    OPENFILENAMEA ofn = { sizeof(OPENFILENAMEA) };
    char szFile[MAX_PATH] = "";
    
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "All Files\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrTitle = title.c_str();
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    
    if (GetOpenFileNameA(&ofn)) {
        return szFile;
    }
    return "";
}

std::vector<std::string> showOpenMultipleFileDialog(const std::string& title,
                                                     const std::vector<std::pair<std::string, std::string>>& filters) {
    // Multiple file selection
    return {};
}

std::string showSaveFileDialog(const std::string& title,
                                const std::string& defaultName,
                                const std::vector<std::pair<std::string, std::string>>& filters) {
    OPENFILENAMEA ofn = { sizeof(OPENFILENAMEA) };
    char szFile[MAX_PATH] = "";
    if (!defaultName.empty()) {
        strncpy_s(szFile, defaultName.c_str(), sizeof(szFile) - 1);
    }
    
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "All Files\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrTitle = title.c_str();
    ofn.Flags = OFN_OVERWRITEPROMPT;
    
    if (GetSaveFileNameA(&ofn)) {
        return szFile;
    }
    return "";
}

std::string showFolderDialog(const std::string& title) {
    // Use IFileDialog or SHBrowseForFolder
    return "";
}

int showMessageBox(const std::string& title, const std::string& message,
                   MessageBoxType type, MessageBoxButtons buttons) {
    UINT uType = MB_OK;
    switch (type) {
        case MessageBoxType::Info: uType |= MB_ICONINFORMATION; break;
        case MessageBoxType::Warning: uType |= MB_ICONWARNING; break;
        case MessageBoxType::Error: uType |= MB_ICONERROR; break;
        case MessageBoxType::Question: uType |= MB_ICONQUESTION; break;
    }
    switch (buttons) {
        case MessageBoxButtons::OKCancel: uType |= MB_OKCANCEL; break;
        case MessageBoxButtons::YesNo: uType |= MB_YESNO; break;
        case MessageBoxButtons::YesNoCancel: uType |= MB_YESNOCANCEL; break;
    }
    
    int result = MessageBoxA(NULL, message.c_str(), title.c_str(), uType);
    switch (result) {
        case IDOK: return 0;
        case IDCANCEL: return 1;
        case IDYES: return 2;
        case IDNO: return 3;
        default: return 0;
    }
}

} // namespace NexusForge::Platform

#endif // NEXUS_PLATFORM_WINDOWS
