// core/engine/workspace_manager.cpp
#include "workspace_manager.hpp"
#include "nexus_core.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <chrono>

namespace NexusForge::Core {

// WorkspaceManager implementation
WorkspaceManager::WorkspaceManager(NexusEngine* engine) : engine_(engine) {}

WorkspaceManager::~WorkspaceManager() {
    shutdown();
}

bool WorkspaceManager::initialize() {
    std::cout << "WorkspaceManager initialized" << std::endl;
    return true;
}

void WorkspaceManager::shutdown() {
    saveCurrentSession();
    currentWorkspace_.reset();
}

bool WorkspaceManager::openWorkspace(const std::string& path) {
    rootPath_ = path;
    currentWorkspace_ = std::make_unique<WorkspaceInfo>();
    currentWorkspace_->rootPath = path;
    size_t slashPos = path.find_last_of("/\\");
    currentWorkspace_->name = (slashPos != std::string::npos) ? path.substr(slashPos + 1) : path;
    currentWorkspace_->lastOpened = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    std::cout << "Opened workspace: " << currentWorkspace_->name << std::endl;
    return true;
}

bool WorkspaceManager::createWorkspace(const std::string& name, const std::string& path) {
    auto workspace = std::make_shared<WorkspaceInfo>();
    workspace->id = name;
    workspace->name = name;
    workspace->rootPath = path;
    workspaces_.push_back(workspace);
    return true;
}

bool WorkspaceManager::closeWorkspace() {
    saveCurrentSession();
    currentWorkspace_.reset();
    rootPath_.clear();
    return true;
}

bool WorkspaceManager::saveWorkspace() {
    if (!currentWorkspace_) return false;
    // Would save workspace configuration
    return true;
}

void WorkspaceManager::addRecentFile(const std::string& path) {
    // Remove if already exists
    recentFiles_.erase(
        std::remove(recentFiles_.begin(), recentFiles_.end(), path),
        recentFiles_.end()
    );
    // Add to front
    recentFiles_.insert(recentFiles_.begin(), path);
    // Limit to 20 files
    if (recentFiles_.size() > 20) {
        recentFiles_.pop_back();
    }
}

bool WorkspaceManager::restoreLastSession() {
    std::cout << "Restoring last session..." << std::endl;
    // Would restore open files, layout, etc.
    return true;
}

bool WorkspaceManager::saveCurrentSession() {
    std::cout << "Saving current session..." << std::endl;
    // Would save open files, layout, etc.
    return true;
}

} // namespace NexusForge::Core
