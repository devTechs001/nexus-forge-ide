// workspace/file_watcher.hpp
// File Watcher Header

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <set>
#include <ctime>

namespace NexusForge::Core {

enum class FileChangeType {
    Created,
    Modified,
    Deleted,
    Renamed
};

class FileSystemWatcher {
public:
    using WatchCallback = std::function<void(const std::string&, FileChangeType)>;
    using DirectoryWatchCallback = std::function<void(const std::string&, FileChangeType)>;
    using Callback = std::function<void(const std::string& path, int changeType)>;

    FileSystemWatcher();
    ~FileSystemWatcher();

    bool initialize();
    void shutdown();

    void addWatch(const std::string& path);
    void addWatch(const std::string& path, WatchCallback callback);
    void addDirectoryWatch(const std::string& path, DirectoryWatchCallback callback, bool recursive = true);
    void removeWatch(const std::string& path);
    void setCallback(Callback callback);

    void pause();
    void resume();
    bool isPaused() const;

    void update();

    size_t getWatchCount() const;

private:
    struct FileState {
        bool exists = false;
        std::time_t lastModified = 0;
        size_t size = 0;
    };

    struct DirectoryWatch {
        std::string path;
        bool recursive = true;
        DirectoryWatchCallback callback;
        std::set<std::string> knownFiles;
    };

    std::unordered_map<std::string, FileState> watchedPaths_;
    std::unordered_map<std::string, std::vector<WatchCallback>> fileCallbacks_;
    std::vector<DirectoryWatch> directoryWatches_;
    Callback globalCallback_;

    std::thread watchThread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};

    void watchLoop();
    void checkForChanges();
    void checkDirectoryChanges(DirectoryWatch& watch);
    void notifyFileChanged(const std::string& path, FileChangeType changeType);

    bool pathExists(const std::string& path) const;
    std::time_t getFileModifiedTime(const std::string& path) const;
    size_t getFileSize(const std::string& path) const;
};

} // namespace NexusForge::Core
