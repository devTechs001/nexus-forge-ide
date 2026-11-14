// workspace/project_system.cpp
// Project System Implementation

#include "project_system.hpp"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace NexusForge::Workspace {

Project::Project() = default;
Project::~Project() = default;

void Project::addFile(const std::string& path) {
    ProjectFile pf;
    pf.path = path;
    auto pos = path.find_last_of("/\\");
    pf.relativePath = (pos != std::string::npos) ? path.substr(pos + 1) : path;
    auto dot = pf.relativePath.rfind('.');
    pf.extension = (dot != std::string::npos) ? pf.relativePath.substr(dot) : "";
    std::string ext = pf.extension;
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (ext == ".c" || ext == ".cc" || ext == ".cpp" || ext == ".cxx" ||
        ext == ".m" || ext == ".mm" || ext == ".rs" || ext == ".go" ||
        ext == ".py" || ext == ".js" || ext == ".ts" || ext == ".java") {
        pf.isSource = true;
        pf.itemType = "source";
    } else if (ext == ".h" || ext == ".hh" || ext == ".hpp" || ext == ".hxx" || ext == ".inl") {
        pf.isHeader = true;
        pf.itemType = "header";
    } else {
        pf.itemType = "other";
    }
    files_.push_back(pf);
}

void Project::removeFile(const std::string& path) {
    files_.erase(std::remove_if(files_.begin(), files_.end(),
        [&](const ProjectFile& f) { return f.path == path; }), files_.end());
}

bool Project::hasFile(const std::string& path) const {
    return std::any_of(files_.begin(), files_.end(),
        [&](const ProjectFile& f) { return f.path == path; });
}

std::vector<ProjectFile> Project::getFilesByType(const std::string& type) const {
    std::vector<ProjectFile> result;
    for (const auto& f : files_) {
        if (f.itemType == type) result.push_back(f);
    }
    return result;
}

std::vector<ProjectFile> Project::getSourceFiles() const {
    std::vector<ProjectFile> result;
    for (const auto& f : files_) {
        if (f.isSource) result.push_back(f);
    }
    return result;
}

std::vector<ProjectFile> Project::getHeaderFiles() const {
    std::vector<ProjectFile> result;
    for (const auto& f : files_) {
        if (f.isHeader) result.push_back(f);
    }
    return result;
}

void Project::addConfig(const BuildConfig& config) {
    configs_.push_back(config);
}

BuildConfig* Project::getConfig(const std::string& name) {
    for (auto& c : configs_) {
        if (c.name == name) return &c;
    }
    return nullptr;
}

const BuildConfig* Project::getConfig(const std::string& name) const {
    for (const auto& c : configs_) {
        if (c.name == name) return &c;
    }
    return nullptr;
}

void Project::setSetting(const std::string& key, const std::string& value) {
    settings_[key] = value;
}

std::string Project::getSetting(const std::string& key, const std::string& defaultValue) const {
    auto it = settings_.find(key);
    return it != settings_.end() ? it->second : defaultValue;
}

void Project::addDependency(const std::string& projectId) {
    dependencies_.push_back(projectId);
}

void Project::removeDependency(const std::string& projectId) {
    dependencies_.erase(std::remove(dependencies_.begin(), dependencies_.end(), projectId),
                        dependencies_.end());
}

bool Project::load(const std::string& path) {
    filePath_ = path;
    auto pos = path.rfind('/');
    if (pos != std::string::npos) {
        rootDir_ = path.substr(0, pos);
    }
    // In production, would parse project file
    return true;
}

bool Project::save(const std::string& path) const {
    // In production, would write project file
    return true;
}

ProjectType Project::detectProjectType(const std::string& directory) {
    if (std::filesystem::exists(directory + "/CMakeLists.txt")) return ProjectType::CMake;
    if (std::filesystem::exists(directory + "/Makefile")) return ProjectType::Makefile;
    if (std::filesystem::exists(directory + "/package.json")) return ProjectType::NodeJS;
    if (std::filesystem::exists(directory + "/Cargo.toml")) return ProjectType::Rust;
    return ProjectType::Unknown;
}

bool Project::isProjectDirectory(const std::string& directory) {
    return detectProjectType(directory) != ProjectType::Unknown;
}

bool Project::loadCMakeProject(const std::string& path) { return true; }
bool Project::loadMakeProject(const std::string& path) { return true; }
bool Project::loadNodeProject(const std::string& path) { return true; }
bool Project::loadDotNetProject(const std::string& path) { return true; }

// ProjectManager
ProjectManager& ProjectManager::getInstance() {
    static ProjectManager instance;
    return instance;
}

ProjectManager::ProjectManager() = default;

bool ProjectManager::openProject(const std::string& path) {
    auto project = std::make_unique<Project>();
    if (!project->load(path)) return false;
    currentProject_ = std::move(project);
    return true;
}

void ProjectManager::closeProject() {
    currentProject_.reset();
}

bool ProjectManager::saveProject() {
    return currentProject_ && currentProject_->save();
}

bool ProjectManager::saveProjectAs(const std::string& path) {
    return currentProject_ && currentProject_->save(path);
}

void ProjectManager::addRecentProject(const std::string& path) {
    recentProjects_.erase(std::remove(recentProjects_.begin(), recentProjects_.end(), path),
                          recentProjects_.end());
    recentProjects_.insert(recentProjects_.begin(), path);
}

std::vector<std::string> ProjectManager::getRecentProjects(size_t maxCount) const {
    if (recentProjects_.size() <= maxCount) return recentProjects_;
    return std::vector<std::string>(recentProjects_.begin(), recentProjects_.begin() + maxCount);
}

void ProjectManager::clearRecentProjects() {
    recentProjects_.clear();
}

std::vector<std::string> ProjectManager::getProjectTemplates() const {
    return {};
}

bool ProjectManager::createProjectFromTemplate(const std::string& templateName,
                                                const std::string& destination,
                                                const std::string& name) {
    return true;
}

} // namespace NexusForge::Workspace