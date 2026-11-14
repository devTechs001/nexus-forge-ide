// main.cpp - NexusForge IDE Entry Point
// Cross-Platform Main Entry with Mobile-First Architecture

#include "core/engine/nexus_core.hpp"
#include "core/platform/platform_abstraction.hpp"
#include "core/engine/extension_manager.hpp"
#include "core/engine/workspace_manager.hpp"
#include "ui/screens/splash_screen.hpp"
#include "ui/screens/main_window.hpp"
#include "config/settings_manager.hpp"
#include "ai/models/model_manager.hpp"

#include <chrono>
#include <csignal>
#include <exception>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

// Platform-specific entry points
#if defined(_WIN32) || defined(_WIN64)
    #ifndef NEXUS_PLATFORM_WINDOWS
        #define NEXUS_PLATFORM_WINDOWS
    #endif
    #include <Windows.h>
#elif defined(__ANDROID__)
    #ifndef NEXUS_PLATFORM_ANDROID
        #define NEXUS_PLATFORM_ANDROID
    #endif
    #include <android/native_activity.h>
    #include <jni.h>
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        #ifndef NEXUS_PLATFORM_IOS
            #define NEXUS_PLATFORM_IOS
        #endif
    #else
        #ifndef NEXUS_PLATFORM_MACOS
            #define NEXUS_PLATFORM_MACOS
        #endif
    #endif
#elif defined(__linux__)
    #ifndef NEXUS_PLATFORM_LINUX
        #define NEXUS_PLATFORM_LINUX
    #endif
#elif defined(__EMSCRIPTEN__)
    #ifndef NEXUS_PLATFORM_WEB
        #define NEXUS_PLATFORM_WEB
    #endif
    #include <emscripten.h>
#endif

namespace NexusForge {

namespace {
volatile std::sig_atomic_t g_signalQuit = 0;

void handleTerminationSignal(int) {
    g_signalQuit = 1;
}
} // anonymous namespace

// Application Version Information
constexpr const char* APP_NAME = "NexusForge IDE";
constexpr const char* APP_VERSION = "1.0.0";
constexpr const char* APP_BUILD = "2024.12.001";
constexpr const char* APP_CODENAME = "Phoenix";

// Build Configuration
struct BuildConfig {
    static constexpr bool DEBUG_MODE =
    #ifdef NDEBUG
        false;
    #else
        true;
    #endif

    static constexpr bool ENABLE_VULKAN = true;
    static constexpr bool ENABLE_AI = true;
    static constexpr bool ENABLE_LIVE_SHARE = true;
    static constexpr bool ENABLE_TELEMETRY = false;
    static constexpr int MAX_THREADS = 16;
    static constexpr size_t INITIAL_HEAP_SIZE = 256 * 1024 * 1024; // 256MB
};

// Startup Phase Manager
enum class StartupPhase {
    PreInit,
    PlatformInit,
    CoreInit,
    UIInit,
    ExtensionsLoad,
    AIModelsLoad,
    WorkspaceRestore,
    Ready
};

class StartupManager {
public:
    using PhaseCallback = std::function<void(StartupPhase, float)>;

    void registerCallback(PhaseCallback callback) {
        callbacks_.push_back(std::move(callback));
    }

    void notifyProgress(StartupPhase phase, float progress) {
        currentPhase_ = phase;
        for (auto& cb : callbacks_) {
            cb(phase, progress);
        }
    }

    StartupPhase getCurrentPhase() const { return currentPhase_; }

private:
    StartupPhase currentPhase_ = StartupPhase::PreInit;
    std::vector<PhaseCallback> callbacks_;
};

// Global Application Instance
class NexusForgeApp {
public:
    static NexusForgeApp& getInstance() {
        static NexusForgeApp instance;
        return instance;
    }

    int run(int argc, char* argv[]);
    void shutdown();

    // Accessors
    Core::NexusEngine& getEngine() { return *engine_; }
    UI::MainWindow& getMainWindow() { return *mainWindow_; }
    Config::SettingsManager& getSettings() { return *settings_; }
    AI::ModelManager& getAIManager() { return *aiManager_; }
    StartupManager& getStartupManager() { return startupManager_; }

private:
    NexusForgeApp() = default;
    ~NexusForgeApp();

    bool initializePlatform();
    bool initializeCore();
    bool initializeUI();
    bool loadExtensions();
    bool loadAIModels();
    bool restoreWorkspace();

    std::unique_ptr<Core::NexusEngine> engine_;
    std::unique_ptr<UI::SplashScreen> splashScreen_;
    std::unique_ptr<UI::MainWindow> mainWindow_;
    std::unique_ptr<Config::SettingsManager> settings_;
    std::unique_ptr<AI::ModelManager> aiManager_;
    StartupManager startupManager_;

    bool running_ = false;
    std::vector<std::string> commandLineArgs_;
};

NexusForgeApp::~NexusForgeApp() {
    shutdown();
}

int NexusForgeApp::run(int argc, char* argv[]) {
    g_signalQuit = 0;
    std::signal(SIGINT, handleTerminationSignal);
    std::signal(SIGTERM, handleTerminationSignal);

    // Parse command line arguments
    commandLineArgs_.clear();
    bool skipSplash = false;
    std::vector<std::string> filesToOpen;

    for (int i = 0; i < argc; ++i) {
        std::string arg = argv[i] ? argv[i] : "";
        commandLineArgs_.push_back(arg);
        if (i == 0) continue;

        if (arg == "--version" || arg == "-v") {
            std::cout << APP_NAME << " " << APP_VERSION
                      << " (" << APP_CODENAME << ", build " << APP_BUILD << ")" << std::endl;
            return 0;
        }
        if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: NexusForge [options] [files...]\n"
                      << "Options:\n"
                      << "  -h, --help       Show this help message and exit\n"
                      << "  -v, --version    Show version information and exit\n"
                      << "      --no-splash  Skip the startup splash screen\n";
            return 0;
        }
        if (arg == "--no-splash") {
            skipSplash = true;
            continue;
        }
        if (!arg.empty() && arg[0] != '-') {
            filesToOpen.push_back(arg);
        }
    }

    try {
        // Phase 1: Platform Initialization
        startupManager_.notifyProgress(StartupPhase::PlatformInit, 0.0f);
        if (!initializePlatform()) {
            std::cerr << "Failed to initialize platform" << std::endl;
            return -1;
        }
        startupManager_.notifyProgress(StartupPhase::PlatformInit, 1.0f);

        const bool hasGui = Platform::PlatformAbstraction::hasGraphicalDisplay();

        // Print concise startup banner to console as well
        std::cout << "\n=============================================\n"
                  << "  " << APP_NAME << " " << APP_VERSION << " (" << APP_CODENAME << ")\n"
                  << "  Code Without Boundaries\n"
                  << "=============================================\n";
        if (hasGui) {
            std::cout << "[splash] Opening splash screen..." << std::endl;
        } else {
            std::cout << "[info]   No graphical X11 display detected (DISPLAY unset); running in console mode." << std::endl;
        }

        // Launch Splash Screen immediately before heavier subsystems initialize
        auto lastSplashTick = std::chrono::steady_clock::now();
        auto pumpSplash = [&](int durationMs) {
            if (!splashScreen_ || !splashScreen_->isVisible()) return;
            auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(durationMs);
            do {
                if (g_signalQuit || splashScreen_->isCloseRequested()) break;
                auto now = std::chrono::steady_clock::now();
                double dt = std::chrono::duration<double>(now - lastSplashTick).count();
                lastSplashTick = now;
                Platform::PlatformAbstraction::processEvents();
                splashScreen_->update(dt);
                splashScreen_->render();
                if (durationMs > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(16));
                }
            } while (std::chrono::steady_clock::now() < deadline);
        };

        auto setSplashStage = [&](float progress, const std::string& phase, const std::string& msg) {
            std::cout << "[boot " << static_cast<int>(progress * 100.0f) << "%] "
                      << phase << " - " << msg << std::endl;
            if (splashScreen_) {
                splashScreen_->setPhase(phase);
                splashScreen_->setProgress(progress, msg);
                pumpSplash(hasGui ? 360 : 0);
            }
        };

        if (!skipSplash) {
            splashScreen_ = std::make_unique<UI::SplashScreen>();
            if (splashScreen_->initialize(nullptr)) {
                splashScreen_->setMinimumDisplayTime(hasGui ? 2.4f : 0.1f);
                splashScreen_->show();
                setSplashStage(0.12f, "Initializing Platform",
                               hasGui ? "Connected to native display server" : "Console runtime initialized");
            } else {
                splashScreen_.reset();
            }
        }

        if (g_signalQuit) {
            shutdown();
            return 0;
        }

        // Phase 2: Core Engine Initialization
        startupManager_.notifyProgress(StartupPhase::CoreInit, 0.0f);
        setSplashStage(0.28f, "Starting Core Engine",
                       "Initializing memory manager, thread pool & event bus");
        if (!initializeCore()) {
            std::cerr << "Failed to initialize core engine" << std::endl;
            return -2;
        }
        startupManager_.notifyProgress(StartupPhase::CoreInit, 1.0f);

        // Phase 3: Load Extensions & AI Models asynchronously while animating Splash Screen
        startupManager_.notifyProgress(StartupPhase::ExtensionsLoad, 0.0f);
        if (splashScreen_) {
            splashScreen_->setPhase("Loading Extensions & AI Assistant");
            splashScreen_->setProgress(0.54f, "Scanning plugins & initializing Phoenix AI models");
        }
        std::cout << "[boot 54%] Loading Extensions & Phoenix AI..." << std::endl;

        auto extensionsFuture = std::async(std::launch::async, [this]() {
            return loadExtensions();
        });

        startupManager_.notifyProgress(StartupPhase::AIModelsLoad, 0.0f);
        auto aiModelsFuture = std::async(std::launch::async, [this]() {
            return loadAIModels();
        });

        while ((extensionsFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready ||
                aiModelsFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) &&
               !g_signalQuit) {
            pumpSplash(16);
            if (!splashScreen_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        extensionsFuture.wait();
        aiModelsFuture.wait();
        startupManager_.notifyProgress(StartupPhase::ExtensionsLoad, 1.0f);
        startupManager_.notifyProgress(StartupPhase::AIModelsLoad, 1.0f);

        setSplashStage(0.76f, "Initializing Workbench UI",
                       "Preparing editor, explorer sidebar & integrated terminal");

        // Phase 4: Initialize Main IDE Window (kept hidden until splash finishes)
        startupManager_.notifyProgress(StartupPhase::UIInit, 0.0f);
        if (!initializeUI()) {
            std::cerr << "Failed to initialize UI" << std::endl;
            return -3;
        }
        startupManager_.notifyProgress(StartupPhase::UIInit, 1.0f);

        // Phase 5: Restore Workspace
        startupManager_.notifyProgress(StartupPhase::WorkspaceRestore, 0.0f);
        setSplashStage(0.90f, "Restoring Workspace",
                       "Indexing project tree, git branch & recent session");
        restoreWorkspace();
        startupManager_.notifyProgress(StartupPhase::WorkspaceRestore, 1.0f);

        // Phase 6: Finalize splash animation smoothly to 100% before opening IDE window
        startupManager_.notifyProgress(StartupPhase::Ready, 1.0f);
        if (splashScreen_ && splashScreen_->isVisible()) {
            splashScreen_->setPhase("Launching NexusForge IDE");
            splashScreen_->setProgress(1.0f, "Ready - opening workbench");
            while (!g_signalQuit && !splashScreen_->isCloseRequested() &&
                   (!splashScreen_->isMinimumTimeElapsed() ||
                    splashScreen_->getDisplayedProgress() < 0.99f)) {
                pumpSplash(16);
            }
            if (hasGui && !g_signalQuit && !splashScreen_->isCloseRequested()) {
                pumpSplash(180); // Brief pause at 100% so user sees completion
            }
            splashScreen_->close();
            splashScreen_.reset();
        }

        if (g_signalQuit) {
            shutdown();
            return 0;
        }

        // Open any files passed on the command line
        if (mainWindow_ && !filesToOpen.empty()) {
            mainWindow_->openFiles(filesToOpen);
        }

        // Wire platform window events and 60 FPS update/render loop to MainWindow
        Platform::PlatformAbstraction::setWindowEventCallback(
            [this](const Platform::PlatformEvent& ev) {
                if (mainWindow_) {
                    mainWindow_->handlePlatformEvent(ev);
                }
            });

        engine_->setUpdateCallback([this](double dt) {
            if (g_signalQuit) {
                engine_->requestQuit();
                return;
            }
            if (mainWindow_) {
                mainWindow_->update(dt);
            }
        });

        engine_->setRenderCallback([this]() {
            if (mainWindow_) {
                mainWindow_->render();
            }
        });

        if (mainWindow_) {
            mainWindow_->addOutputLog("[ready] Startup complete - NexusForge IDE is ready");
            mainWindow_->show();
        }

        std::cout << APP_NAME << " " << APP_VERSION << " (" << APP_CODENAME
                  << ") ready." << (hasGui ? " Close window or press Ctrl+C to exit."
                                           : " Press Ctrl+C to exit.")
                  << std::endl;

        // Main Loop
        running_ = true;
        int rc = engine_->runMainLoop();
        shutdown();
        return rc;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -999;
    }
}

void NexusForgeApp::shutdown() {
    running_ = false;
    Platform::PlatformAbstraction::setWindowEventCallback(nullptr);

    if (splashScreen_) {
        splashScreen_->close();
        splashScreen_.reset();
    }
    if (aiManager_) aiManager_->shutdown();
    if (mainWindow_) mainWindow_->close();
    if (engine_) engine_->shutdown();

    aiManager_.reset();
    mainWindow_.reset();
    settings_.reset();
    engine_.reset();

    Platform::PlatformAbstraction::shutdown();
}

bool NexusForgeApp::initializePlatform() {
    return Platform::PlatformAbstraction::initialize();
}

bool NexusForgeApp::initializeCore() {
    engine_ = std::make_unique<Core::NexusEngine>();
    settings_ = std::make_unique<Config::SettingsManager>();
    auto paths = Platform::PlatformAbstraction::getDirectoryPaths();
    settings_->initialize(paths.appDataDir);

    Core::EngineConfig config;
    config.appName = APP_NAME;
    config.appVersion = APP_VERSION;
    config.enableVulkan = BuildConfig::ENABLE_VULKAN;
    config.maxThreads = BuildConfig::MAX_THREADS;
    config.initialHeapSize = BuildConfig::INITIAL_HEAP_SIZE;

    return engine_->initialize(config);
}

bool NexusForgeApp::initializeUI() {
    mainWindow_ = std::make_unique<UI::MainWindow>();
    if (!mainWindow_->initialize(engine_.get(), settings_.get())) {
        return false;
    }
    mainWindow_->onClose = [this]() {
        if (engine_) engine_->requestQuit();
    };
    return true;
}

bool NexusForgeApp::loadExtensions() {
    return engine_->getExtensionManager().loadAllExtensions();
}

bool NexusForgeApp::loadAIModels() {
    if (!BuildConfig::ENABLE_AI) return true;

    aiManager_ = std::make_unique<AI::ModelManager>();
    AI::AISettings aiSettings;
    aiSettings.enabled = true;
    return aiManager_->initialize(aiSettings);
}

bool NexusForgeApp::restoreWorkspace() {
    return engine_->getWorkspaceManager().restoreLastSession();
}

} // namespace NexusForge

// Platform-Specific Entry Points

#ifdef NEXUS_PLATFORM_WINDOWS
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    return NexusForge::NexusForgeApp::getInstance().run(__argc, __argv);
}
#endif

#if defined(NEXUS_PLATFORM_ANDROID) && !defined(NEXUS_HAS_JNI_BRIDGE)
extern "C" {
    JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
        return JNI_VERSION_1_6;
    }

    JNIEXPORT void JNICALL
    Java_com_nexusforge_ide_NexusActivity_nativeInit(JNIEnv* env, jobject obj) {
        // Android-specific initialization
    }

    JNIEXPORT void JNICALL
    Java_com_nexusforge_ide_NexusActivity_nativeRun(JNIEnv* env, jobject obj) {
        char* argv[] = { (char*)"NexusForge" };
        NexusForge::NexusForgeApp::getInstance().run(1, argv);
    }
}
#endif

#ifdef NEXUS_PLATFORM_WEB
extern "C" {
    void emscripten_main_loop() {
        NexusForge::NexusForgeApp::getInstance().getEngine().tick();
    }
}
#endif

// Standard entry point for desktop platforms
#if defined(NEXUS_PLATFORM_LINUX) || defined(NEXUS_PLATFORM_MACOS)
int main(int argc, char* argv[]) {
    return NexusForge::NexusForgeApp::getInstance().run(argc, argv);
}
#endif

#ifdef NEXUS_PLATFORM_WEB
int main(int argc, char* argv[]) {
    NexusForge::NexusForgeApp::getInstance().run(argc, argv);
    emscripten_set_main_loop(emscripten_main_loop, 0, 1);
    return 0;
}
#endif
