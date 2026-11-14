// workspace/file_watcher.cpp
// File Watcher Implementation

#include "file_watcher.hpp"
#include <algorithm>
#include <filesystem>
#include <thread>
#include <chrono>

namespace NexusForge::Core {

FileSystemWatcher::FileSystemWatcher() : running_(false), paused_(false) {
}

FileSystemWatcher::~FileSystemWatcher() {
    shutdown();
}

bool FileSystemWatcher::initialize() {
    if (running_.exchange(true)) {
        return true;
    }

    // Start watch thread
    watchThread_ = std::thread(&FileSystemWatcher::watchLoop, this);

    return true;
}

void FileSystemWatcher::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_.store(false);
    }
    cv_.notify_all();

    if (watchThread_.joinable()) {
        watchThread_.join();
    }

    std::lock_guard<std::mutex> lock(mutex_);
    watchedPaths_.clear();
    fileCallbacks_.clear();
    directoryWatches_.clear();
}

void FileSystemWatcher::addWatch(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (watchedPaths_.find(path) == watchedPaths_.end()) {
        FileState state;
        state.exists = pathExists(path);
        if (state.exists) {
            state.lastModified = getFileModifiedTime(path);
            state.size = getFileSize(path);
        }
        watchedPaths_[path] = state;
    }
}

void FileSystemWatcher::addWatch(const std::string& path, WatchCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (watchedPaths_.find(path) == watchedPaths_.end()) {
        FileState state;
        state.exists = pathExists(path);
        if (state.exists) {
            state.lastModified = getFileModifiedTime(path);
            state.size = getFileSize(path);
        }
        watchedPaths_[path] = state;
    }

    if (callback) {
        fileCallbacks_[path].push_back(std::move(callback));
    }
}

void FileSystemWatcher::setCallback(Callback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    globalCallback_ = std::move(callback);
}

void FileSystemWatcher::update() {
    if (!paused_.load()) {
        checkForChanges();
    }
}

void FileSystemWatcher::addDirectoryWatch(const std::string& path, DirectoryWatchCallback callback,
                                     bool recursive) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    DirectoryWatch watch;
    watch.path = path;
    watch.recursive = recursive;
    watch.callback = std::move(callback);
    
    directoryWatches_.push_back(watch);
    
    // Also add to watched paths
    if (watchedPaths_.find(path) == watchedPaths_.end()) {
        watchedPaths_[path] = {};
    }
}

void FileSystemWatcher::removeWatch(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    watchedPaths_.erase(path);
    fileCallbacks_.erase(path);
    
    directoryWatches_.erase(
        std::remove_if(directoryWatches_.begin(), directoryWatches_.end(),
            [&path](const DirectoryWatch& w) { return w.path == path; }),
        directoryWatches_.end()
    );
}

void FileSystemWatcher::watchLoop() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (running_.load()) {
        cv_.wait_for(lock, std::chrono::milliseconds(200), [this]() {
            return !running_.load();
        });
        if (!running_.load()) break;
        if (paused_.load()) continue;

        lock.unlock();
        checkForChanges();
        lock.lock();
    }
}

void FileSystemWatcher::checkForChanges() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& [path, state] : watchedPaths_) {
        if (!pathExists(path)) {
            // File was deleted
            if (state.exists) {
                state.exists = false;
                notifyFileChanged(path, FileChangeType::Deleted);
            }
            continue;
        }
        
        if (!state.exists) {
            // File was created
            state.exists = true;
            state.lastModified = getFileModifiedTime(path);
            state.size = getFileSize(path);
            notifyFileChanged(path, FileChangeType::Created);
            continue;
        }
        
        // Check for modifications
        auto modifiedTime = getFileModifiedTime(path);
        auto size = getFileSize(path);
        
        if (modifiedTime != state.lastModified || size != state.size) {
            state.lastModified = modifiedTime;
            state.size = size;
            notifyFileChanged(path, FileChangeType::Modified);
        }
    }
    
    // Check directory watches
    for (auto& watch : directoryWatches_) {
        checkDirectoryChanges(watch);
    }
}

void FileSystemWatcher::checkDirectoryChanges(DirectoryWatch& watch) {
    try {
        std::set<std::string> currentFiles;
        if (watch.recursive) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(watch.path)) {
                std::string entryPath = entry.path().string();
                currentFiles.insert(entryPath);
                if (watch.knownFiles.find(entryPath) == watch.knownFiles.end()) {
                    watch.knownFiles.insert(entryPath);
                    if (watch.callback) watch.callback(entryPath, FileChangeType::Created);
                }
            }
        } else {
            for (const auto& entry : std::filesystem::directory_iterator(watch.path)) {
                std::string entryPath = entry.path().string();
                currentFiles.insert(entryPath);
                if (watch.knownFiles.find(entryPath) == watch.knownFiles.end()) {
                    watch.knownFiles.insert(entryPath);
                    if (watch.callback) watch.callback(entryPath, FileChangeType::Created);
                }
            }
        }
        
        for (auto it = watch.knownFiles.begin(); it != watch.knownFiles.end();) {
            if (currentFiles.find(*it) == currentFiles.end()) {
                // File was deleted
                if (watch.callback) {
                    watch.callback(*it, FileChangeType::Deleted);
                }
                it = watch.knownFiles.erase(it);
            } else {
                ++it;
            }
        }
    } catch (const std::exception&) {
        // Handle filesystem errors
    }
}

void FileSystemWatcher::notifyFileChanged(const std::string& path, FileChangeType changeType) {
    auto it = fileCallbacks_.find(path);
    if (it != fileCallbacks_.end()) {
        for (auto& callback : it->second) {
            if (callback) {
                callback(path, changeType);
            }
        }
    }
    if (globalCallback_) {
        globalCallback_(path, static_cast<int>(changeType));
    }
}

bool FileSystemWatcher::pathExists(const std::string& path) const {
    try {
        return std::filesystem::exists(path);
    } catch (...) {
        return false;
    }
}

std::time_t FileSystemWatcher::getFileModifiedTime(const std::string& path) const {
    try {
        return std::filesystem::last_write_time(path).time_since_epoch().count();
    } catch (...) {
        return 0;
    }
}

size_t FileSystemWatcher::getFileSize(const std::string& path) const {
    try {
        return std::filesystem::file_size(path);
    } catch (...) {
        return 0;
    }
}

void FileSystemWatcher::pause() {
    paused_.store(true);
}

void FileSystemWatcher::resume() {
    paused_.store(false);
    cv_.notify_all();
}

bool FileSystemWatcher::isPaused() const {
    return paused_.load();
}

size_t FileSystemWatcher::getWatchCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return watchedPaths_.size();
}

} // namespace NexusForge::Core
