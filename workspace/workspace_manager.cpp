// workspace/workspace_manager.cpp
// Workspace Manager Implementation

#include "workspace_manager.hpp"
#include "project_system.hpp"
#include "file_watcher.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cstdlib>

namespace NexusForge::Workspace {

WorkspaceManager::WorkspaceManager(Core::NexusEngine* engine) : engine_(engine) {
}

WorkspaceManager::~WorkspaceManager() {
    shutdown();
}

bool WorkspaceManager::initialize() {
    sessionsPath_ = getSessionsDirectory();
    std::error_code ec;
    std::filesystem::create_directories(sessionsPath_, ec);
    return true;
}

void WorkspaceManager::shutdown() {
    stopAllWatching();
    folders_.clear();
}

bool WorkspaceManager::openWorkspace(const std::vector<std::string>& folderPaths) {
    folders_.clear();
    for (const auto& path : folderPaths) {
        if (!openFolder(path)) return false;
    }
    currentSessionId_ = generateSessionId();
    if (onWorkspaceOpened) onWorkspaceOpened();
    return true;
}

bool WorkspaceManager::openFolder(const std::string& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || !std::filesystem::is_directory(path, ec)) {
        return false;
    }
    WorkspaceFolder folder;
    folder.id = std::to_string(folders_.size());
    std::string trimmed = path;
    while (trimmed.size() > 1 && (trimmed.back() == '/' || trimmed.back() == '\\')) {
        trimmed.pop_back();
    }
    size_t slashPos = trimmed.find_last_of("/\\");
    folder.name = (slashPos != std::string::npos) ? trimmed.substr(slashPos + 1) : trimmed;
    folder.path = path;
    scanFolder(folder);
    folders_.push_back(std::move(folder));
    return true;
}

void WorkspaceManager::closeWorkspace() {
    stopAllWatching();
    folders_.clear();
    if (onWorkspaceClosed) onWorkspaceClosed();
}

bool WorkspaceManager::saveWorkspace(const std::string& path) {
    return true;
}

bool WorkspaceManager::loadWorkspace(const std::string& path) {
    return true;
}

std::string WorkspaceManager::getRootPath() const {
    if (folders_.empty()) return "";
    return folders_[0].path;
}

std::string WorkspaceManager::getWorkspaceName() const {
    if (folders_.empty()) return "";
    return folders_[0].name;
}

bool WorkspaceManager::openFile(const std::string& path) {
    if (!isFileOpen(path)) {
        openFiles_[path] = openFiles_.size();
    }
    if (onFileOpened) onFileOpened(path);
    return true;
}

bool WorkspaceManager::closeFile(const std::string& path) {
    openFiles_.erase(path);
    if (onFileClosed) onFileClosed(path);
    return true;
}

bool WorkspaceManager::saveFile(const std::string& path) {
    if (onFileSaved) onFileSaved(path);
    return true;
}

bool WorkspaceManager::saveAllFiles() {
    for (const auto& [path, _] : openFiles_) {
        if (onFileSaved) onFileSaved(path);
    }
    return true;
}

bool WorkspaceManager::fileExists(const std::string& path) const {
    return std::filesystem::exists(path);
}

bool WorkspaceManager::isFileOpen(const std::string& path) const {
    return openFiles_.find(path) != openFiles_.end();
}

std::vector<std::string> WorkspaceManager::getOpenFiles() const {
    std::vector<std::string> result;
    for (const auto& [path, _] : openFiles_) {
        result.push_back(path);
    }
    return result;
}

void WorkspaceManager::addRecentFile(const std::string& path) {
    auto it = std::remove_if(recentFiles_.begin(), recentFiles_.end(),
        [&](const RecentFile& f) { return f.path == path; });
    if (it != recentFiles_.end()) recentFiles_.erase(it, recentFiles_.end());
    RecentFile rf;
    rf.path = path;
    rf.lastOpened = std::time(nullptr);
    recentFiles_.insert(recentFiles_.begin(), rf);
}

std::vector<RecentFile> WorkspaceManager::getRecentFiles(size_t maxCount) const {
    if (recentFiles_.size() <= maxCount) return recentFiles_;
    return std::vector<RecentFile>(recentFiles_.begin(), recentFiles_.begin() + maxCount);
}

void WorkspaceManager::clearRecentFiles() {
    recentFiles_.clear();
}

void WorkspaceManager::removeRecentFile(const std::string& path) {
    recentFiles_.erase(std::remove_if(recentFiles_.begin(), recentFiles_.end(),
        [&](const RecentFile& f) { return f.path == path; }), recentFiles_.end());
}

bool WorkspaceManager::saveSession(const std::string& name) {
    return true;
}

bool WorkspaceManager::loadSession(const std::string& name) {
    return true;
}

std::vector<std::string> WorkspaceManager::getSessionNames() const {
    return {};
}

bool WorkspaceManager::deleteSession(const std::string& name) {
    return true;
}

bool WorkspaceManager::restoreLastSession() {
    return true;
}

std::vector<FileInfo> WorkspaceManager::searchFiles(const FileSearchOptions& options) const {
    std::vector<FileInfo> results;
    for (const auto& folder : folders_) {
        auto files = searchFilesInFolder(folder.path, options);
        results.insert(results.end(), files.begin(), files.end());
        if (static_cast<int>(results.size()) >= options.maxResults) break;
    }
    return results;
}

std::vector<FileInfo> WorkspaceManager::searchFilesInFolder(
    const std::string& folderPath, const FileSearchOptions& options) const {
    std::vector<FileInfo> results;
    return results;
}

std::vector<SearchResult> WorkspaceManager::searchText(const TextSearchOptions& options) const {
    return {};
}

std::vector<SearchResult> WorkspaceManager::searchTextInFiles(
    const std::vector<std::string>& paths, const TextSearchOptions& options) const {
    return {};
}

std::vector<FileInfo> WorkspaceManager::listDirectory(const std::string& path) const {
    std::vector<FileInfo> result;
    return result;
}

bool WorkspaceManager::createDirectory(const std::string& path) {
    std::error_code ec;
    return std::filesystem::create_directories(path, ec) && !ec;
}

bool WorkspaceManager::deleteDirectory(const std::string& path, bool recursive) {
    std::error_code ec;
    if (recursive) return std::filesystem::remove_all(path, ec) > 0 && !ec;
    return std::filesystem::remove(path, ec) && !ec;
}

bool WorkspaceManager::renameFile(const std::string& oldPath, const std::string& newPath) {
    std::error_code ec;
    std::filesystem::rename(oldPath, newPath, ec);
    return !ec;
}

bool WorkspaceManager::copyFile(const std::string& source, const std::string& dest) {
    std::error_code ec;
    std::filesystem::copy_file(source, dest, ec);
    return !ec;
}

bool WorkspaceManager::moveFile(const std::string& source, const std::string& dest) {
    std::error_code ec;
    std::filesystem::rename(source, dest, ec);
    return !ec;
}

bool WorkspaceManager::startWatching(const std::string& path) {
    return true;
}

void WorkspaceManager::stopWatching(const std::string& path) {
}

void WorkspaceManager::stopAllWatching() {
}

size_t WorkspaceManager::addFileChangeListener(FileChangeCallback callback) {
    size_t id = nextListenerId_++;
    changeListeners_.push_back({id, std::move(callback)});
    return id;
}

void WorkspaceManager::removeFileChangeListener(size_t id) {
    changeListeners_.erase(std::remove_if(changeListeners_.begin(), changeListeners_.end(),
        [id](const ChangeListener& cl) { return cl.id == id; }), changeListeners_.end());
}

void WorkspaceManager::addExcludePattern(const std::string& pattern) {
    excludePatterns_.push_back(pattern);
}

void WorkspaceManager::removeExcludePattern(const std::string& pattern) {
    excludePatterns_.erase(std::remove(excludePatterns_.begin(), excludePatterns_.end(), pattern),
                           excludePatterns_.end());
}

std::string WorkspaceManager::relativePath(const std::string& fullPath) const {
    if (folders_.empty()) return fullPath;
    std::string root = folders_[0].path;
    while (root.size() > 1 && (root.back() == '/' || root.back() == '\\')) {
        root.pop_back();
    }
    if (fullPath == root) return "";
    if (fullPath.size() > root.size() &&
        fullPath.compare(0, root.size(), root) == 0 &&
        (fullPath[root.size()] == '/' || fullPath[root.size()] == '\\')) {
        return fullPath.substr(root.size() + 1);
    }
    return fullPath;
}

std::string WorkspaceManager::absolutePath(const std::string& relativePath) const {
    if (folders_.empty()) return relativePath;
    return folders_[0].path + "/" + relativePath;
}

bool WorkspaceManager::isExcluded(const std::string& path) const {
    for (const auto& pattern : excludePatterns_) {
        if (path.find(pattern) != std::string::npos) return true;
    }
    return false;
}

void WorkspaceManager::scanFolder(WorkspaceFolder& folder) {
    // In production, would scan the folder recursively
}

bool WorkspaceManager::shouldIncludeFile(const std::string& path) const {
    return !isExcluded(path);
}

void WorkspaceManager::notifyFileChange(const FileChangeEvent& event) {
    for (const auto& listener : changeListeners_) {
        listener.callback(event);
    }
    if (onFileChanged) onFileChanged(event);
}

std::string WorkspaceManager::generateSessionId() const {
    return "session_" + std::to_string(std::time(nullptr));
}

std::string WorkspaceManager::getSessionsDirectory() const {
    return std::string(getenv("HOME") ? getenv("HOME") : ".") + "/.config/NexusForge/sessions";
}

} // namespace NexusForge::Workspace