// core/engine/nexus_core.hpp
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace NexusForge::Core {

// Forward declarations
class MemoryManager;
class ThreadPool;
class EventSystem;
class PluginLoader;
class ExtensionManager;
class WorkspaceManager;
class FileSystemWatcher;

// Engine Configuration
struct EngineConfig {
    std::string appName;
    std::string appVersion;
    bool enableVulkan = true;
    bool enableOpenGL = true;
    bool enableMetal = true;
    bool debugMode = false;
    int maxThreads = 8;
    size_t initialHeapSize = 256 * 1024 * 1024;
    std::string dataPath;
    std::string cachePath;
    std::string extensionsPath;
};

// Engine Statistics
struct EngineStats {
    uint64_t frameCount = 0;
    double frameTime = 0.0;
    double fps = 0.0;
    size_t memoryUsed = 0;
    size_t memoryAllocated = 0;
    int activeThreads = 0;
    int loadedExtensions = 0;
    int openFiles = 0;
};

// Core Engine Class
class NexusEngine {
public:
    NexusEngine();
    ~NexusEngine();

    // Lifecycle
    bool initialize(const EngineConfig& config);
    void shutdown();
    int runMainLoop();
    void requestQuit();

    // Frame management
    void tick();
    void render();

    // Subsystem access
    MemoryManager& getMemoryManager();
    ThreadPool& getThreadPool();
    EventSystem& getEventSystem();
    ExtensionManager& getExtensionManager();
    WorkspaceManager& getWorkspaceManager();

    // Statistics
    const EngineStats& getStats() const { return stats_; }
    const EngineConfig& getConfig() const { return config_; }

    // State
    bool isRunning() const { return running_.load(); }
    bool isInitialized() const { return initialized_; }

private:
    EngineConfig config_;
    EngineStats stats_;

    std::unique_ptr<MemoryManager> memoryManager_;
    std::unique_ptr<ThreadPool> threadPool_;
    std::unique_ptr<EventSystem> eventSystem_;
    std::unique_ptr<ExtensionManager> extensionManager_;
    std::unique_ptr<WorkspaceManager> workspaceManager_;
    std::unique_ptr<FileSystemWatcher> fileWatcher_;

    std::atomic<bool> running_{false};
    bool initialized_ = false;

    // Timing
    double lastFrameTime_ = 0.0;
    double deltaTime_ = 0.0;
    double accumulator_ = 0.0;
    static constexpr double FIXED_TIMESTEP = 1.0 / 60.0;
};

} // namespace NexusForge::Core
