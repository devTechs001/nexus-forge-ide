// core/engine/plugin_loader.hpp
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace NexusForge::Core {

// Forward declaration
class NexusEngine;

// Plugin information
struct PluginInfo {
    std::string id;
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::vector<std::string> dependencies;
};

// Plugin interface
class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual bool initialize(NexusEngine* engine) = 0;
    virtual void shutdown() = 0;
    virtual void update(double deltaTime) = 0;
    virtual const PluginInfo& getInfo() const = 0;
};

// Plugin loader
class PluginLoader {
public:
    PluginLoader();
    ~PluginLoader();

    bool initialize(const std::string& pluginsPath);
    void shutdown();

    // Load/unload plugins
    bool loadPlugin(const std::string& path);
    bool unloadPlugin(const std::string& pluginId);
    bool reloadPlugin(const std::string& pluginId);

    // Plugin access
    IPlugin* getPlugin(const std::string& pluginId);
    std::vector<std::string> getLoadedPlugins() const;

    // Plugin discovery
    std::vector<std::string> discoverPlugins(const std::string& path);

private:
    struct LoadedPlugin {
        void* handle;
        std::unique_ptr<IPlugin> plugin;
        PluginInfo info;
        std::string path;
    };

    std::string pluginsPath_;
    std::unordered_map<std::string, LoadedPlugin> plugins_;

    void* loadLibrary(const std::string& path);
    void unloadLibrary(void* handle);
    IPlugin* createPlugin(void* handle);
};

} // namespace NexusForge::Core
