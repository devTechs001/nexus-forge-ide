// core/engine/plugin_loader.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

namespace NexusForge::Core {

// Plugin info
struct PluginInfo {
    std::string id;
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::string path;
    bool loaded = false;
    bool enabled = true;
    std::vector<std::string> dependencies;
};

// Plugin interface
class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual const PluginInfo& getInfo() const = 0;
};

// Plugin loader
class PluginLoader {
public:
    PluginLoader();
    ~PluginLoader();

    // Plugin management
    bool loadPlugin(const std::string& path);
    bool unloadPlugin(const std::string& id);
    bool enablePlugin(const std::string& id);
    bool disablePlugin(const std::string& id);

    // Access
    IPlugin* getPlugin(const std::string& id);
    const PluginInfo* getPluginInfo(const std::string& id) const;
    std::vector<std::string> getLoadedPlugins() const;
    std::vector<std::string> getEnabledPlugins() const;

    // Plugin directory
    void setPluginDirectory(const std::string& path);
    const std::string& getPluginDirectory() const { return pluginDir_; }

    // Scan and load
    void scanPlugins();
    void loadAllPlugins();
    void unloadAllPlugins();

    // Events
    using PluginCallback = std::function<void(const std::string&)>;
    void addPluginLoadedCallback(PluginCallback callback);
    void addPluginUnloadedCallback(PluginCallback callback);

private:
    std::string pluginDir_;
    std::unordered_map<std::string, PluginInfo> plugins_;
    std::unordered_map<std::string, std::unique_ptr<IPlugin>> instances_;
    std::unordered_map<std::string, void*> handles_;

    std::vector<PluginCallback> loadedCallbacks_;
    std::vector<PluginCallback> unloadedCallbacks_;

    IPlugin* createPluginInstance(void* handle);
    bool checkDependencies(const PluginInfo& info);
    void notifyLoaded(const std::string& id);
    void notifyUnloaded(const std::string& id);
};

// Plugin registration macro
#define NEXUS_PLUGIN_EXPORT \
    extern "C" __declspec(dllexport) NexusForge::Core::IPlugin* createPlugin(); \
    extern "C" __declspec(dllexport) void destroyPlugin(NexusForge::Core::IPlugin* plugin);

} // namespace NexusForge::Core
