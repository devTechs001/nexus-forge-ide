// core/engine/plugin_loader.hpp
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <any>

namespace NexusForge::Core {

// Plugin information
struct PluginInfo {
    std::string id;
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::string path;
    bool loaded = false;
    bool active = false;
};

// Plugin interface
class IPlugin {
public:
    virtual ~IPlugin() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(double deltaTime) = 0;

    virtual const PluginInfo& getInfo() const = 0;
};

// Plugin loader result
enum class PluginLoadResult {
    Success,
    NotFound,
    InvalidFormat,
    InitFailed,
    VersionMismatch,
    DependencyMissing
};

// Plugin loader class
class PluginLoader {
public:
    PluginLoader();
    ~PluginLoader();

    // Plugin management
    PluginLoadResult loadPlugin(const std::string& path);
    bool unloadPlugin(const std::string& pluginId);
    bool activatePlugin(const std::string& pluginId);
    bool deactivatePlugin(const std::string& pluginId);

    // Plugin access
    IPlugin* getPlugin(const std::string& pluginId);
    const std::vector<PluginInfo>& getLoadedPlugins() const;
    std::vector<std::string> getActivePluginIds() const;

    // Plugin directory scanning
    std::vector<std::string> scanPluginDirectory(const std::string& directory);
    bool loadAllPlugins(const std::string& directory);

    // Plugin registration (for statically linked plugins)
    void registerPlugin(std::unique_ptr<IPlugin> plugin);

    // Events
    std::function<void(const std::string& pluginId)> onPluginLoaded;
    std::function<void(const std::string& pluginId)> onPluginUnloaded;
    std::function<void(const std::string& pluginId)> onPluginActivated;
    std::function<void(const std::string& pluginId)> onPluginDeactivated;
    std::function<void(const std::string& pluginId, const std::string& error)> onPluginError;

private:
    struct LoadedPlugin {
        PluginInfo info;
        std::unique_ptr<IPlugin> instance;
        void* nativeHandle = nullptr;
    };

    std::unordered_map<std::string, LoadedPlugin> plugins_;
    std::vector<PluginInfo> pluginDirectory_;

    // Platform-specific loading
    void* loadNativeLibrary(const std::string& path);
    void unloadNativeLibrary(void* handle);
    void* getFunctionAddress(void* handle, const std::string& name);

    // Plugin validation
    bool validatePlugin(const std::string& path);
    PluginInfo parsePluginInfo(const std::string& path);
    bool checkDependencies(const PluginInfo& info);

    // Helper to get plugin entry point
    using CreatePluginFunc = IPlugin* (*)();
    using DestroyPluginFunc = void (*)(IPlugin*);
};

// Plugin export macros
#ifdef _WIN32
    #define NEXUS_PLUGIN_EXPORT __declspec(dllexport)
#else
    #define NEXUS_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

#define NEXUS_PLUGIN_ENTRY(PluginClass) \
    extern "C" NEXUS_PLUGIN_EXPORT IPlugin* createPlugin() { \
        return new PluginClass(); \
    } \
    extern "C" NEXUS_PLUGIN_EXPORT void destroyPlugin(IPlugin* plugin) { \
        delete plugin; \
    }

} // namespace NexusForge::Core
