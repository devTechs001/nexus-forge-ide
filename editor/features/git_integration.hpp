// editor/features/git_integration.hpp
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace NexusForge::Editor {

// Git status
enum class FileStatus {
    Untracked,
    Unmodified,
    Modified,
    Added,
    Deleted,
    Renamed,
    Copied,
    Ignored,
    Unmerged
};

// Git file info
struct GitFileInfo {
    std::string path;
    std::string relativePath;
    FileStatus status = FileStatus::Untracked;
    FileStatus stagedStatus = FileStatus::Unmodified;
    bool hasConflicts = false;
    size_t additions = 0;
    size_t deletions = 0;
};

// Git branch info
struct BranchInfo {
    std::string name;
    std::string upstream;
    int aheadBy = 0;
    int behindBy = 0;
    bool isDetached = false;
    std::string detachedCommit;
};

// Git commit info
struct CommitInfo {
    std::string hash;
    std::string shortHash;
    std::string subject;
    std::string body;
    std::string author;
    std::string authorEmail;
    std::chrono::system_clock::time_point date;
    std::vector<std::string> parents;
};

// Git diff
struct DiffHunk {
    int oldStart = 0;
    int oldLines = 0;
    int newStart = 0;
    int newLines = 0;
    std::string header;
    std::vector<std::string> lines;
};

struct FileDiff {
    std::string oldPath;
    std::string newPath;
    std::string oldMode;
    std::string newMode;
    std::vector<DiffHunk> hunks;
    size_t additions = 0;
    size_t deletions = 0;
};

// Git file entry (summary)
struct GitFile {
    std::string path;
    FileStatus status = FileStatus::Untracked;
};

// Git status summary
struct GitStatus {
    std::string branch;
    int ahead = 0;
    int behind = 0;
    bool isDirty = false;
    std::vector<GitFile> stagedFiles;
    std::vector<GitFile> modifiedFiles;
    std::vector<GitFile> untrackedFiles;
};

// Git commit summary
struct GitCommit {
    std::string hash;
    std::string shortHash;
    std::string message;
    std::string author;
    std::string date;
};

// Git branch summary
struct GitBranch {
    std::string name;
    bool isCurrent = false;
    bool isRemote = false;
};

// Git remote summary
struct GitRemote {
    std::string name;
    std::string url;
};

// Git integration
class GitIntegration {
public:
    GitIntegration();
    ~GitIntegration();

    // Repository
    bool initialize(const std::string& workingDirectory);
    bool openRepository(const std::string& path);
    void closeRepository();
    bool hasRepository() const { return initialized_ || !repoPath_.empty(); }
    const std::string& getRepositoryPath() const { return !repoPath_.empty() ? repoPath_ : workingDirectory_; }

    // Status
    GitStatus getStatus();
    std::vector<GitFileInfo> getFileStatuses() const;
    GitFileInfo getFileStatus(const std::string& path) const;
    bool isGitRepository(const std::string& path) const;
    static bool isRepository(const std::string& path);
    static std::string findRepositoryRoot(const std::string& startPath);
    bool hasUncommittedChanges() const;

    // Branch
    BranchInfo getCurrentBranch() const;
    std::vector<GitBranch> getBranches();
    std::vector<std::string> getRemoteBranches() const;
    bool checkout(const std::string& branchOrCommit);
    bool checkoutBranch(const std::string& name);
    bool createBranch(const std::string& name, const std::string& startPoint = "");
    bool deleteBranch(const std::string& name);

    // Remote
    std::vector<GitRemote> getRemotes();
    bool fetch(const std::string& remote = "origin");
    bool pull(const std::string& remote = "origin", const std::string& branch = "");
    bool push(const std::string& remote = "origin", const std::string& branch = "");

    // Staging
    bool stageFile(const std::string& path);
    bool stageAll();
    bool unstageFile(const std::string& path);
    bool unstageAll();

    // Commit
    bool commit(const std::string& message);
    bool amendCommit(const std::string& message);
    std::vector<GitCommit> getLog(int maxEntries = 20);
    CommitInfo getCommit(const std::string& hash) const;

    // Diff
    std::string getDiff(const std::string& filePath);
    FileDiff getFileDiff(const std::string& path) const;
    FileDiff getStagedDiff(const std::string& path) const;
    FileDiff getDiffBetweenCommits(const std::string& from, const std::string& to) const;
    std::string getFileContent(const std::string& filePath, const std::string& revision = "HEAD");
    std::string getFileAtCommit(const std::string& path, const std::string& commit) const;

    // Blame
    struct BlameLine {
        std::string hash;
        std::string author;
        std::chrono::system_clock::time_point date;
        int lineNumber = 0;
        std::string content;
    };
    std::string getBlame(const std::string& filePath, size_t line);
    std::vector<BlameLine> blame(const std::string& path) const;

    // Stash
    bool stash(const std::string& message = "");
    bool stashPop();
    bool stashApply(const std::string& stashRef = "stash@{0}");
    std::vector<std::string> getStashes() const;

    // Merge
    bool merge(const std::string& branch);
    bool abortMerge();
    std::vector<std::string> getMergeConflicts() const;
    bool markResolved(const std::string& path);

    // Events
    using RepositoryChangedCallback = std::function<void()>;
    void addChangeListener(RepositoryChangedCallback callback);

private:
    bool initialized_ = false;
    bool detachedHead_ = false;
    std::string workingDirectory_;
    std::string currentBranch_;
    std::string repoPath_;
    std::string gitPath_;
    std::vector<RepositoryChangedCallback> listeners_;

    void parseStagedFiles(std::vector<GitFile>& files);
    void parseModifiedFiles(std::vector<GitFile>& files);
    void parseUntrackedFiles(std::vector<GitFile>& files);
    std::string executeGitCommand(const std::vector<std::string>& args) const;
    void notifyChanged();
};

} // namespace NexusForge::Editor
