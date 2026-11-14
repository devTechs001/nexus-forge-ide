// core/engine/plugin_loader.hpp
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace NexusForge::Core {

class NexusEngine;

// Plugin information
struct PluginInfo {
    std::string id;
    std::string name;
    std::string version;
    std::string path;
    bool loaded = false;
};

// Plugin interface
class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual bool load() = 0;
    virtual void unload() = 0;
    virtual void update(double deltaTime) = 0;
    virtual const PluginInfo& getInfo() const = 0;
};

// Plugin Loader
class PluginLoader {
public:
    PluginLoader();
    ~PluginLoader();

    bool initialize(const std::string& pluginsPath);
    void shutdown();

    std::shared_ptr<IPlugin> loadPlugin(const std::string& path);
    void unloadPlugin(std::shared_ptr<IPlugin> plugin);
    void unloadAllPlugins();

    std::vector<PluginInfo> scanPlugins();
    const std::vector<std::shared_ptr<IPlugin>>& getLoadedPlugins() const { return plugins_; }

private:
    std::string pluginsPath_;
    std::vector<std::shared_ptr<IPlugin>> plugins_;
};

} // namespace NexusForge::Core
