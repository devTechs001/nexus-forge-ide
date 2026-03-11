// workspace/project_system.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

namespace NexusForge::Workspace {

// Project types
enum class ProjectType {
    Unknown,
    Cpp,
    C,
    Python,
    JavaScript,
    TypeScript,
    Rust,
    Go,
    Java,
    CSharp,
    NodeJS,
    Web,
    CMake,
    Makefile,
    DotNet
};

// Build configuration
struct BuildConfig {
    std::string name;
    std::string platform;
    std::string configuration;  // Debug, Release, etc.
    std::string outputDirectory;
    std::string intermediateDirectory;
    std::vector<std::string> defines;
    std::vector<std::string> includePaths;
    std::vector<std::string> libraryPaths;
    std::vector<std::string> libraries;
    std::vector<std::string> compilerFlags;
    std::vector<std::string> linkerFlags;
};

// Project file info
struct ProjectFile {
    std::string path;
    std::string relativePath;
    std::string extension;
    bool isHeader = false;
    bool isSource = false;
    bool isResource = false;
    bool excluded = false;
    std::string itemType;
};

// Project definition
class Project {
public:
    Project();
    ~Project();

    // Identity
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }

    const std::string& getFilePath() const { return filePath_; }
    void setFilePath(const std::string& path) { filePath_ = path; }

    ProjectType getType() const { return type_; }
    void setType(ProjectType type) { type_ = type; }

    // Root
    const std::string& getRootDirectory() const { return rootDir_; }
    void setRootDirectory(const std::string& dir) { rootDir_ = dir; }

    // Files
    void addFile(const std::string& path);
    void removeFile(const std::string& path);
    bool hasFile(const std::string& path) const;

    const std::vector<ProjectFile>& getFiles() const { return files_; }
    std::vector<ProjectFile> getFilesByType(const std::string& type) const;
    std::vector<ProjectFile> getSourceFiles() const;
    std::vector<ProjectFile> getHeaderFiles() const;

    // Configurations
    void addConfig(const BuildConfig& config);
    BuildConfig* getConfig(const std::string& name);
    const BuildConfig* getConfig(const std::string& name) const;
    const std::vector<BuildConfig>& getConfigs() const { return configs_; }

    // Settings
    void setSetting(const std::string& key, const std::string& value);
    std::string getSetting(const std::string& key, const std::string& defaultValue = "") const;
    const std::unordered_map<std::string, std::string>& getSettings() const { return settings_; }

    // Dependencies
    void addDependency(const std::string& projectId);
    void removeDependency(const std::string& projectId);
    const std::vector<std::string>& getDependencies() const { return dependencies_; }

    // Load/Save
    bool load(const std::string& path);
    bool save(const std::string& path = "") const;

    // Detection
    static ProjectType detectProjectType(const std::string& directory);
    static bool isProjectDirectory(const std::string& directory);

private:
    std::string name_;
    std::string filePath_;
    std::string rootDir_;
    ProjectType type_ = ProjectType::Unknown;

    std::vector<ProjectFile> files_;
    std::vector<BuildConfig> configs_;
    std::unordered_map<std::string, std::string> settings_;
    std::vector<std::string> dependencies_;

    bool loadCMakeProject(const std::string& path);
    bool loadMakeProject(const std::string& path);
    bool loadNodeProject(const std::string& path);
    bool loadDotNetProject(const std::string& path);
};

// Project manager
class ProjectManager {
public:
    static ProjectManager& getInstance();

    // Project management
    bool openProject(const std::string& path);
    void closeProject();
    bool saveProject();
    bool saveProjectAs(const std::string& path);

    Project* getCurrentProject() { return currentProject_.get(); }
    const Project* getCurrentProject() const { return currentProject_.get(); }

    bool hasProject() const { return currentProject_ != nullptr; }

    // Recent projects
    void addRecentProject(const std::string& path);
    std::vector<std::string> getRecentProjects(size_t maxCount = 10) const;
    void clearRecentProjects();

    // Templates
    std::vector<std::string> getProjectTemplates() const;
    bool createProjectFromTemplate(const std::string& templateName,
                                    const std::string& destination,
                                    const std::string& name);

private:
    ProjectManager();
    std::unique_ptr<Project> currentProject_;
    std::vector<std::string> recentProjects_;
};

} // namespace NexusForge::Workspace
