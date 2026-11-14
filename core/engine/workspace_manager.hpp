// core/engine/workspace_manager.hpp
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace NexusForge::Core {

class NexusEngine;

// Workspace information
struct WorkspaceInfo {
    std::string id;
    std::string name;
    std::string rootPath;
    std::vector<std::string> folders;
    std::vector<std::string> recentFiles;
    uint64_t lastOpened = 0;
};

// File system watcher
class FileSystemWatcher {
public:
    FileSystemWatcher();
    ~FileSystemWatcher();

    bool initialize();
    void shutdown();

    void addWatch(const std::string& path);
    void removeWatch(const std::string& path);
    void update();

    using Callback = std::function<void(const std::string& path, int changeType)>;
    void setCallback(Callback callback);

private:
    std::vector<std::string> watchedPaths_;
    Callback callback_;
};

// Workspace Manager
class WorkspaceManager {
public:
    explicit WorkspaceManager(NexusEngine* engine);
    ~WorkspaceManager();

    bool initialize();
    void shutdown();

    // Workspace management
    bool openWorkspace(const std::string& path);
    bool createWorkspace(const std::string& name, const std::string& path);
    bool closeWorkspace();
    bool saveWorkspace();

    // Access
    WorkspaceInfo* getCurrentWorkspace() { return currentWorkspace_.get(); }
    const WorkspaceInfo* getCurrentWorkspace() const { return currentWorkspace_.get(); }
    const std::string& getRootPath() const { return rootPath_; }

    // Recent files
    void addRecentFile(const std::string& path);
    std::vector<std::string> getRecentFiles() const { return recentFiles_; }

    // Session management
    bool restoreLastSession();
    bool saveCurrentSession();

private:
    NexusEngine* engine_;
    std::unique_ptr<WorkspaceInfo> currentWorkspace_;
    std::string rootPath_;
    std::vector<std::string> recentFiles_;
    std::vector<std::shared_ptr<WorkspaceInfo>> workspaces_;
};

} // namespace NexusForge::Core
