// main.cpp - NexusForge IDE Entry Point
// Cross-Platform Main Entry with Mobile-First Architecture

#include "core/engine/nexus_core.hpp"
#include "core/platform/platform_abstraction.hpp"
#include "ui/screens/splash_screen.hpp"
#include "ui/screens/main_window.hpp"
#include "config/settings_manager.hpp"
#include "ai/models/model_manager.hpp"

#include <iostream>
#include <memory>
#include <thread>
#include <future>

// Platform-specific entry points
#if defined(_WIN32) || defined(_WIN64)
    #define NEXUS_PLATFORM_WINDOWS
    #include <Windows.h>
#elif defined(__ANDROID__)
    #define NEXUS_PLATFORM_ANDROID
    #include <android/native_activity.h>
    #include <jni.h>
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        #define NEXUS_PLATFORM_IOS
    #else
        #define NEXUS_PLATFORM_MACOS
    #endif
#elif defined(__linux__)
    #define NEXUS_PLATFORM_LINUX
#elif defined(__EMSCRIPTEN__)
    #define NEXUS_PLATFORM_WEB
    #include <emscripten.h>
#endif

namespace NexusForge {

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
    // Parse command line arguments
    for (int i = 0; i < argc; ++i) {
        commandLineArgs_.push_back(argv[i]);
    }

    try {
        // Phase 1: Platform Initialization
        startupManager_.notifyProgress(StartupPhase::PlatformInit, 0.0f);
        if (!initializePlatform()) {
            std::cerr << "Failed to initialize platform" << std::endl;
            return -1;
        }
        startupManager_.notifyProgress(StartupPhase::PlatformInit, 1.0f);

        // Phase 2: Core Engine Initialization
        startupManager_.notifyProgress(StartupPhase::CoreInit, 0.0f);
        if (!initializeCore()) {
            std::cerr << "Failed to initialize core engine" << std::endl;
            return -2;
        }
        startupManager_.notifyProgress(StartupPhase::CoreInit, 1.0f);

        // Phase 3: UI Initialization (Show Splash Screen)
        startupManager_.notifyProgress(StartupPhase::UIInit, 0.0f);
        if (!initializeUI()) {
            std::cerr << "Failed to initialize UI" << std::endl;
            return -3;
        }
        startupManager_.notifyProgress(StartupPhase::UIInit, 1.0f);

        // Phase 4: Load Extensions (Async)
        startupManager_.notifyProgress(StartupPhase::ExtensionsLoad, 0.0f);
        auto extensionsFuture = std::async(std::launch::async, [this]() {
            return loadExtensions();
        });

        // Phase 5: Load AI Models (Async)
        startupManager_.notifyProgress(StartupPhase::AIModelsLoad, 0.0f);
        auto aiModelsFuture = std::async(std::launch::async, [this]() {
            return loadAIModels();
        });

        // Wait for async operations
        extensionsFuture.wait();
        startupManager_.notifyProgress(StartupPhase::ExtensionsLoad, 1.0f);

        aiModelsFuture.wait();
        startupManager_.notifyProgress(StartupPhase::AIModelsLoad, 1.0f);

        // Phase 6: Restore Workspace
        startupManager_.notifyProgress(StartupPhase::WorkspaceRestore, 0.0f);
        restoreWorkspace();
        startupManager_.notifyProgress(StartupPhase::WorkspaceRestore, 1.0f);

        // Ready
        startupManager_.notifyProgress(StartupPhase::Ready, 1.0f);

        // Main Loop
        running_ = true;
        return engine_->runMainLoop();

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -999;
    }
}

void NexusForgeApp::shutdown() {
    running_ = false;

    if (aiManager_) aiManager_->shutdown();
    if (mainWindow_) mainWindow_->close();
    if (engine_) engine_->shutdown();

    aiManager_.reset();
    mainWindow_.reset();
    settings_.reset();
    engine_.reset();
}

bool NexusForgeApp::initializePlatform() {
    return Platform::PlatformAbstraction::initialize();
}

bool NexusForgeApp::initializeCore() {
    engine_ = std::make_unique<Core::NexusEngine>();
    settings_ = std::make_unique<Config::SettingsManager>();

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
    return mainWindow_->initialize(engine_.get(), settings_.get());
}

bool NexusForgeApp::loadExtensions() {
    return engine_->getExtensionManager().loadAllExtensions();
}

bool NexusForgeApp::loadAIModels() {
    if (!BuildConfig::ENABLE_AI) return true;

    aiManager_ = std::make_unique<AI::ModelManager>();
    return aiManager_->initialize(settings_->getAISettings());
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

#ifdef NEXUS_PLATFORM_ANDROID
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
