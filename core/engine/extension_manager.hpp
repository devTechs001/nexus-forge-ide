// core/engine/extension_manager.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace NexusForge::Core {

class NexusEngine;

// Extension information
struct ExtensionInfo {
    std::string id;
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::string path;
    bool enabled = true;
    bool loaded = false;
    std::vector<std::string> dependencies;
};

// Extension interface
class IExtension {
public:
    virtual ~IExtension() = default;
    virtual bool activate() = 0;
    virtual void deactivate() = 0;
    virtual void update(double deltaTime) = 0;
    virtual const ExtensionInfo& getInfo() const = 0;
};

// Extension Manager
class ExtensionManager {
public:
    explicit ExtensionManager(NexusEngine* engine);
    ~ExtensionManager();

    bool initialize(const std::string& extensionsPath);
    void shutdown();

    // Extension management
    bool loadExtension(const std::string& path);
    bool unloadExtension(const std::string& extensionId);
    bool enableExtension(const std::string& extensionId);
    bool disableExtension(const std::string& extensionId);
    bool loadAllExtensions();

    // Extension access
    IExtension* getExtension(const std::string& extensionId);
    std::vector<ExtensionInfo> getAllExtensions() const;
    std::vector<ExtensionInfo> getLoadedExtensions() const;
    std::vector<ExtensionInfo> getEnabledExtensions() const;

    // Updates
    void update(double deltaTime);

    // Statistics
    int getLoadedCount() const { return static_cast<int>(extensions_.size()); }

private:
    NexusEngine* engine_;
    std::string extensionsPath_;
    std::unordered_map<std::string, std::shared_ptr<IExtension>> extensions_;
    std::unordered_map<std::string, ExtensionInfo> extensionInfos_;
};

} // namespace NexusForge::Core
