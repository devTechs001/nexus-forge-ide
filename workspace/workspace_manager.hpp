// workspace/workspace_manager.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <optional>

namespace NexusForge::Core {
    class NexusEngine;
}

namespace NexusForge::Workspace {

// File information
struct FileInfo {
    std::string path;
    std::string name;
    std::string extension;
    size_t size = 0;
    bool isDirectory = false;
    bool isSymlink = false;
    int64_t createdTime = 0;
    int64_t modifiedTime = 0;
    int64_t accessedTime = 0;
};

// Workspace folder
struct WorkspaceFolder {
    std::string id;
    std::string name;
    std::string path;
    std::vector<FileInfo> files;
    std::vector<std::string> excludePatterns;
    bool includeHidden = false;
    size_t maxFileSize = 10 * 1024 * 1024;  // 10MB
};

// Recent file entry
struct RecentFile {
    std::string path;
    std::string workspaceId;
    int64_t lastOpened;
    size_t cursorPosition = 0;
    size_t cursorLine = 0;
    size_t cursorColumn = 0;
    std::vector<size_t> bookmarks;
};

// Workspace session
struct WorkspaceSession {
    std::string id;
    std::string name;
    std::vector<std::string> folderPaths;
    std::vector<RecentFile> openFiles;
    std::string activeFile;
    std::vector<std::string> searchHistory;
    std::vector<std::string> replaceHistory;
    std::unordered_map<std::string, std::string> fileStates;  // File path -> state
    int64_t lastUsed = 0;
};

// File search options
struct FileSearchOptions {
    std::string pattern;
    std::vector<std::string> excludePatterns;
    bool caseSensitive = false;
    bool matchFullPath = false;
    bool includeHidden = false;
    int maxResults = 200;
};

// Text search options
struct TextSearchOptions {
    std::string pattern;
    bool caseSensitive = false;
    bool wholeWord = false;
    bool useRegex = false;
    std::vector<std::string> includePatterns;
    std::vector<std::string> excludePatterns;
    int maxResults = 20000;
    int contextLines = 0;
};

// Search result
struct SearchResult {
    std::string filePath;
    size_t lineNumber;
    size_t column;
    std::string lineText;
    std::string matchText;
    size_t matchStart;
    size_t matchLength;
    std::vector<std::string> contextBefore;
    std::vector<std::string> contextAfter;
};

// File watcher events
enum class FileChangeType {
    Created,
    Changed,
    Deleted,
    Renamed
};

struct FileChangeEvent {
    std::string path;
    std::string oldPath;  // For rename
    FileChangeType type;
    int64_t timestamp;
};

// Workspace Manager
class WorkspaceManager {
public:
    explicit WorkspaceManager(Core::NexusEngine* engine);
    ~WorkspaceManager();

    bool initialize();
    void shutdown();

    // Workspace management
    bool openWorkspace(const std::vector<std::string>& folderPaths);
    bool openFolder(const std::string& path);
    void closeWorkspace();
    bool saveWorkspace(const std::string& path);
    bool loadWorkspace(const std::string& path);

    // Access
    const std::vector<WorkspaceFolder>& getFolders() const { return folders_; }
    std::string getRootPath() const;
    std::string getWorkspaceName() const;
    bool hasWorkspace() const { return !folders_.empty(); }

    // File operations
    bool openFile(const std::string& path);
    bool closeFile(const std::string& path);
    bool saveFile(const std::string& path);
    bool saveAllFiles();
    bool fileExists(const std::string& path) const;
    bool isFileOpen(const std::string& path) const;
    std::vector<std::string> getOpenFiles() const;

    // Recent files
    void addRecentFile(const std::string& path);
    std::vector<RecentFile> getRecentFiles(size_t maxCount = 20) const;
    void clearRecentFiles();
    void removeRecentFile(const std::string& path);

    // Sessions
    bool saveSession(const std::string& name = "");
    bool loadSession(const std::string& name);
    std::vector<std::string> getSessionNames() const;
    bool deleteSession(const std::string& name);
    bool restoreLastSession();

    // File search
    std::vector<FileInfo> searchFiles(const FileSearchOptions& options) const;
    std::vector<FileInfo> searchFilesInFolder(const std::string& folderPath,
                                               const FileSearchOptions& options) const;

    // Text search
    std::vector<SearchResult> searchText(const TextSearchOptions& options) const;
    std::vector<SearchResult> searchTextInFiles(const std::vector<std::string>& paths,
                                                 const TextSearchOptions& options) const;

    // File system
    std::vector<FileInfo> listDirectory(const std::string& path) const;
    bool createDirectory(const std::string& path);
    bool deleteDirectory(const std::string& path, bool recursive = false);
    bool renameFile(const std::string& oldPath, const std::string& newPath);
    bool copyFile(const std::string& source, const std::string& dest);
    bool moveFile(const std::string& source, const std::string& dest);

    // File watching
    bool startWatching(const std::string& path);
    void stopWatching(const std::string& path);
    void stopAllWatching();

    using FileChangeCallback = std::function<void(const FileChangeEvent&)>;
    size_t addFileChangeListener(FileChangeCallback callback);
    void removeFileChangeListener(size_t id);

    // Exclusion patterns
    void addExcludePattern(const std::string& pattern);
    void removeExcludePattern(const std::string& pattern);
    const std::vector<std::string>& getExcludePatterns() const { return excludePatterns_; }

    // Utilities
    std::string relativePath(const std::string& fullPath) const;
    std::string absolutePath(const std::string& relativePath) const;
    bool isExcluded(const std::string& path) const;

    // Events
    std::function<void()> onWorkspaceOpened;
    std::function<void()> onWorkspaceClosed;
    std::function<void(const std::string&)> onFileOpened;
    std::function<void(const std::string&)> onFileClosed;
    std::function<void(const std::string&)> onFileSaved;
    std::function<void(const FileChangeEvent&)> onFileChanged;

private:
    Core::NexusEngine* engine_;
    std::vector<WorkspaceFolder> folders_;
    std::vector<RecentFile> recentFiles_;
    std::vector<std::string> excludePatterns_;
    std::unordered_map<std::string, size_t> openFiles_;  // path -> index

    // File watchers
    struct FileWatcher {
        std::string path;
        int watchId;
        bool active;
    };
    std::vector<FileWatcher> fileWatchers_;

    // Change listeners
    struct ChangeListener {
        size_t id;
        FileChangeCallback callback;
    };
    std::vector<ChangeListener> changeListeners_;
    size_t nextListenerId_ = 1;

    // Session management
    std::string sessionsPath_;
    std::string currentSessionId_;

    // Helpers
    void scanFolder(WorkspaceFolder& folder);
    bool shouldIncludeFile(const std::string& path) const;
    void notifyFileChange(const FileChangeEvent& event);
    std::string generateSessionId() const;
    std::string getSessionsDirectory() const;
};

} // namespace NexusForge::Workspace
