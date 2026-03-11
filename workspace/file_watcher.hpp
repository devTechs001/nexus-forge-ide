// workspace/file_watcher.hpp
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>

namespace NexusForge::Workspace {

// File change event
enum class FileChangeType {
    Created,
    Modified,
    Deleted,
    Renamed
};

struct FileEvent {
    std::string path;
    std::string oldPath;  // For rename
    FileChangeType type;
    uint64_t timestamp;
};

// Watch options
struct WatchOptions {
    bool recursive = true;
    bool ignoreSymlinks = false;
    std::vector<std::string> excludePatterns;
    std::vector<std::string> includePatterns;
};

// File watcher interface
class FileWatcher {
public:
    FileWatcher();
    ~FileWatcher();

    // Watch management
    int watch(const std::string& path, const WatchOptions& options = {});
    void unwatch(int watchId);
    void unwatchAll();

    // Event handling
    using FileEventHandler = std::function<void(const FileEvent&)>;
    void setEventHandler(FileEventHandler handler);

    // Start/stop
    void start();
    void stop();
    bool isRunning() const { return running_; }

    // Ignore patterns
    void addIgnorePattern(const std::string& pattern);
    void removeIgnorePattern(const std::string& pattern);
    bool isIgnored(const std::string& path) const;

private:
    bool running_ = false;
    int nextWatchId_ = 1;
    FileEventHandler eventHandler_;
    std::vector<std::string> ignorePatterns_;

    struct WatchEntry {
        int id;
        std::string path;
        WatchOptions options;
        void* platformData = nullptr;
    };
    std::vector<WatchEntry> watches_;

    void processEvents();
    bool matchesPattern(const std::string& path, const std::vector<std::string>& patterns) const;
};

// Platform-specific implementations
class FileWatcherImpl {
public:
    virtual ~FileWatcherImpl() = default;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual int watch(const std::string& path, bool recursive) = 0;
    virtual void unwatch(int watchId) = 0;
    virtual void setCallback(std::function<void(const FileEvent&)> callback) = 0;
};

std::unique_ptr<FileWatcherImpl> createPlatformFileWatcher();

} // namespace NexusForge::Workspace
