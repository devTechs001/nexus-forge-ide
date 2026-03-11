// core/engine/nexus_core.cpp
#include "nexus_core.hpp"
#include "memory_manager.hpp"
#include "thread_pool.hpp"
#include "event_system.hpp"
#include "plugin_loader.hpp"
#include "../platform/platform_abstraction.hpp"

#include <chrono>
#include <iostream>

namespace NexusForge::Core {

NexusEngine::NexusEngine() = default;
NexusEngine::~NexusEngine() {
    shutdown();
}

bool NexusEngine::initialize(const EngineConfig& config) {
    config_ = config;

    try {
        // Initialize Memory Manager
        memoryManager_ = std::make_unique<MemoryManager>();
        if (!memoryManager_->initialize(config.initialHeapSize)) {
            std::cerr << "Failed to initialize memory manager" << std::endl;
            return false;
        }

        // Initialize Thread Pool
        threadPool_ = std::make_unique<ThreadPool>();
        if (!threadPool_->initialize(config.maxThreads)) {
            std::cerr << "Failed to initialize thread pool" << std::endl;
            return false;
        }

        // Initialize Event System
        eventSystem_ = std::make_unique<EventSystem>();
        if (!eventSystem_->initialize()) {
            std::cerr << "Failed to initialize event system" << std::endl;
            return false;
        }

        // Initialize Extension Manager
        extensionManager_ = std::make_unique<ExtensionManager>(this);
        if (!extensionManager_->initialize(config.extensionsPath)) {
            std::cerr << "Failed to initialize extension manager" << std::endl;
            return false;
        }

        // Initialize Workspace Manager
        workspaceManager_ = std::make_unique<WorkspaceManager>(this);
        if (!workspaceManager_->initialize()) {
            std::cerr << "Failed to initialize workspace manager" << std::endl;
            return false;
        }

        // Initialize File Watcher
        fileWatcher_ = std::make_unique<FileSystemWatcher>();
        if (!fileWatcher_->initialize()) {
            std::cerr << "Failed to initialize file watcher" << std::endl;
            return false;
        }

        initialized_ = true;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Engine initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void NexusEngine::shutdown() {
    running_.store(false);

    if (fileWatcher_) fileWatcher_->shutdown();
    if (workspaceManager_) workspaceManager_->shutdown();
    if (extensionManager_) extensionManager_->shutdown();
    if (eventSystem_) eventSystem_->shutdown();
    if (threadPool_) threadPool_->shutdown();
    if (memoryManager_) memoryManager_->shutdown();

    fileWatcher_.reset();
    workspaceManager_.reset();
    extensionManager_.reset();
    eventSystem_.reset();
    threadPool_.reset();
    memoryManager_.reset();

    initialized_ = false;
}

int NexusEngine::runMainLoop() {
    running_.store(true);

    auto lastTime = std::chrono::high_resolution_clock::now();

    while (running_.load()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        deltaTime_ = std::chrono::duration<double>(currentTime - lastTime).count();
        lastTime = currentTime;

        // Update statistics
        stats_.frameTime = deltaTime_;
        stats_.fps = 1.0 / deltaTime_;
        stats_.frameCount++;
        stats_.memoryUsed = memoryManager_->getUsedMemory();
        stats_.activeThreads = threadPool_->getActiveThreadCount();

        // Process frame
        tick();
        render();

        // Fixed timestep updates
        accumulator_ += deltaTime_;
        while (accumulator_ >= FIXED_TIMESTEP) {
            // Fixed update logic here
            accumulator_ -= FIXED_TIMESTEP;
        }

        // Platform event processing
        Platform::PlatformAbstraction::processEvents();
    }

    return 0;
}

void NexusEngine::tick() {
    eventSystem_->processEvents();
    fileWatcher_->update();
    extensionManager_->update(deltaTime_);
}

void NexusEngine::render() {
    // Rendering handled by UI system
}

void NexusEngine::requestQuit() {
    running_.store(false);
}

MemoryManager& NexusEngine::getMemoryManager() { return *memoryManager_; }
ThreadPool& NexusEngine::getThreadPool() { return *threadPool_; }
EventSystem& NexusEngine::getEventSystem() { return *eventSystem_; }
ExtensionManager& NexusEngine::getExtensionManager() { return *extensionManager_; }
WorkspaceManager& NexusEngine::getWorkspaceManager() { return *workspaceManager_; }

} // namespace NexusForge::Core
