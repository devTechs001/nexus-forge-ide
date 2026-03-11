// core/platform/platform_android.cpp
#include "platform_abstraction.hpp"

#ifdef NEXUS_PLATFORM_ANDROID

#include <android/native_activity.h>
#include <android/input.h>
#include <android/keycodes.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <sstream>

namespace NexusForge::Platform {

static ANativeActivity* g_activity = nullptr;
static EGLDisplay g_eglDisplay = EGL_NO_DISPLAY;
static EGLSurface g_eglSurface = EGL_NO_SURFACE;
static EGLContext g_eglContext = EGL_NO_CONTEXT;

PlatformType getCurrentPlatform() {
    return PlatformType::Android;
}

std::string getPlatformName() {
    return "Android";
}

bool isPlatform(PlatformType platform) {
    return platform == PlatformType::Android;
}

void setNativeActivity(ANativeActivity* activity) {
    g_activity = activity;
}

bool initialize() {
    if (!g_activity) return false;
    
    // Initialize EGL
    g_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_eglDisplay == EGL_NO_DISPLAY) return false;
    
    eglInitialize(g_eglDisplay, nullptr, nullptr);
    return true;
}

void shutdown() {
    if (g_eglSurface != EGL_NO_SURFACE) {
        eglDestroySurface(g_eglDisplay, g_eglSurface);
    }
    if (g_eglContext != EGL_NO_CONTEXT) {
        eglDestroyContext(g_eglDisplay, g_eglContext);
    }
    if (g_eglDisplay != EGL_NO_DISPLAY) {
        eglTerminate(g_eglDisplay);
    }
}

void processEvents() {
    // Android events handled through main loop
}

void* createMainWindow(int width, int height, void** nativeWindow) {
    if (!g_activity) return nullptr;
    
    ANativeWindow* window = g_activity->window;
    if (nativeWindow) *nativeWindow = window;
    
    // Create EGL surface
    EGLConfig config;
    EGLint numConfigs;
    EGLint attribList[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };
    
    eglChooseConfig(g_eglDisplay, attribList, &config, 1, &numConfigs);
    
    g_eglSurface = eglCreateWindowSurface(g_eglDisplay, config, window, nullptr);
    g_eglContext = eglCreateContext(g_eglDisplay, config, EGL_NO_CONTEXT, nullptr);
    
    eglMakeCurrent(g_eglDisplay, g_eglSurface, g_eglSurface, g_eglContext);
    
    return window;
}

void* createSplashWindow(int width, int height, void** nativeWindow) {
    return createMainWindow(width, height, nativeWindow);
}

void destroyWindow(void* window) {
    // Android window managed by system
}

void showWindow(void* window) {
    // Android window always visible
}

void hideWindow(void* window) {
    // Not applicable on Android
}

void setWindowTitle(void* window, const char* title) {
    if (g_activity) {
        ANativeActivity_setWindowFlags(g_activity, AWN_FLAG_FULLSCREEN, 0);
    }
}

void setWindowSize(void* window, int width, int height) {
    // Android handles window sizing
}

void setWindowPosition(void* window, int x, int y) {
    // Not applicable on Android
}

void maximizeWindow(void* window) {
    // Android handles maximization
}

void minimizeWindow(void* window) {
    // Android handles minimization
}

void restoreWindow(void* window) {
    // Android handles restoration
}

void setWindowFullscreen(void* window, bool fullscreen) {
    if (g_activity) {
        if (fullscreen) {
            ANativeActivity_setWindowFlags(g_activity, AWN_FLAG_FULLSCREEN, 0);
        } else {
            ANativeActivity_setWindowFlags(g_activity, 0, AWN_FLAG_FULLSCREEN);
        }
    }
}

std::string getExecutablePath() {
    return g_activity ? g_activity->internalDataPath : "";
}

std::string getCurrentDirectory() {
    return g_activity ? g_activity->internalDataPath : "";
}

bool setCurrentDirectory(const std::string& path) {
    return chdir(path.c_str()) == 0;
}

bool fileExists(const std::string& path) {
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0;
}

bool directoryExists(const std::string& path) {
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode);
}

bool createDirectory(const std::string& path) {
    return mkdir(path.c_str(), 0755) == 0;
}

bool deleteFile(const std::string& path) {
    return unlink(path.c_str()) == 0;
}

bool deleteDirectory(const std::string& path, bool recursive) {
    return rmdir(path.c_str()) == 0;
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
    if (path[0] == '/') return path;
    if (base.back() == '/') return base + path;
    return base + "/" + path;
}

std::string getDirectoryName(const std::string& path) {
    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos) return "";
    return path.substr(0, pos);
}

std::string getFileName(const std::string& path) {
    size_t pos = path.find_last_of('/');
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
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

std::string getAbsolutePath(const std::string& relativePath) {
    if (relativePath.empty() || relativePath[0] == '/') return relativePath;
    return joinPath(getCurrentDirectory(), relativePath);
}

std::string getRelativePath(const std::string& basePath, const std::string& path) {
    return path;
}

std::string getEnvironmentVariable(const std::string& name) {
    return "";
}

bool setEnvironmentVariable(const std::string& name, const std::string& value) {
    return false;
}

std::string getHomeDirectory() {
    return g_activity ? g_activity->internalDataPath : "/data/data";
}

std::string getDataDirectory() {
    return g_activity ? g_activity->internalDataPath : "";
}

std::string getConfigDirectory() {
    return g_activity ? g_activity->internalDataPath : "";
}

std::string getTempDirectory() {
    return g_activity ? g_activity->cachePath : "/cache";
}

std::string getClipboardText() {
    // Android clipboard via JNI
    return "";
}

void setClipboardText(const std::string& text) {
    // Android clipboard via JNI
}

bool hasClipboardText() {
    return false;
}

void setCursor(CursorType cursor) {
    // Not applicable on touch devices
}

void showCursor() {}
void hideCursor() {}
bool isCursorVisible() { return false; }

uint64_t getCurrentTimeMs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL;
}

double getCurrentTimeSeconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

void sleep(int milliseconds) {
    usleep(milliseconds * 1000);
}

void* createThread(void (*func)(void*), void* arg) {
    pthread_t* thread = new pthread_t;
    pthread_create(thread, nullptr, (void*(*)(void*))func, arg);
    return thread;
}

void joinThread(void* thread) {
    if (thread) {
        pthread_join(*reinterpret_cast<pthread_t*>(thread), nullptr);
        delete reinterpret_cast<pthread_t*>(thread);
    }
}

void detachThread(void* thread) {
    if (thread) {
        pthread_detach(*reinterpret_cast<pthread_t*>(thread));
        delete reinterpret_cast<pthread_t*>(thread);
    }
}

void yieldThread() {
    sched_yield();
}

void* loadLibrary(const std::string& path) {
    return dlopen(path.c_str(), RTLD_LAZY);
}

void unloadLibrary(void* handle) {
    if (handle) dlclose(handle);
}

void* getProcAddress(void* handle, const std::string& name) {
    return dlsym(handle, name.c_str());
}

int getCurrentProcessId() {
    return getpid();
}

int getCurrentThreadId() {
    return gettid();
}

bool openUrl(const std::string& url) {
    // Android Intent via JNI
    return false;
}

bool openFile(const std::string& path) {
    // Android Intent via JNI
    return false;
}

bool openFolder(const std::string& path) {
    // Android Intent via JNI
    return false;
}

SystemInfo getSystemInfo() {
    SystemInfo info;
    info.cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
    info.osVersion = "Android";
    info.machineName = "Android Device";
    return info;
}

PowerStatus getPowerStatus() {
    // Android battery status via JNI
    return PowerStatus::Unknown;
}

int getBatteryLevel() {
    // Android battery level via JNI
    return -1;
}

void showNotification(const std::string& title, const std::string& message) {
    // Android notification via JNI
}

void showWarning(const std::string& title, const std::string& message) {
    showNotification(title, message);
}

void showError(const std::string& title, const std::string& message) {
    showNotification(title, message);
}

std::string showOpenFileDialog(const std::string& title,
                                const std::vector<std::pair<std::string, std::string>>& filters) {
    // Android file picker via JNI
    return "";
}

std::vector<std::string> showOpenMultipleFileDialog(const std::string& title,
                                                     const std::vector<std::pair<std::string, std::string>>& filters) {
    return {};
}

std::string showSaveFileDialog(const std::string& title,
                                const std::string& defaultName,
                                const std::vector<std::pair<std::string, std::string>>& filters) {
    return "";
}

std::string showFolderDialog(const std::string& title) {
    return "";
}

int showMessageBox(const std::string& title, const std::string& message,
                   MessageBoxType type, MessageBoxButtons buttons) {
    // Android dialog via JNI
    return 0;
}

void swapBuffers() {
    if (g_eglDisplay != EGL_NO_DISPLAY && g_eglSurface != EGL_NO_SURFACE) {
        eglSwapBuffers(g_eglDisplay, g_eglSurface);
    }
}

} // namespace NexusForge::Platform

#endif // NEXUS_PLATFORM_ANDROID
