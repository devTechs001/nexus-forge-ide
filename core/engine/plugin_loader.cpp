// core/engine/plugin_loader.cpp
#include "plugin_loader.hpp"
#include "nexus_core.hpp"
#include "../platform/platform_abstraction.hpp"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace NexusForge::Core {

PluginLoader::PluginLoader() = default;

PluginLoader::~PluginLoader() {
    shutdown();
}

bool PluginLoader::initialize(const std::string& pluginsPath) {
    pluginsPath_ = pluginsPath;

    // Create plugins directory if it doesn't exist
    if (!fs::exists(pluginsPath_)) {
        fs::create_directories(pluginsPath_);
    }

    return true;
}

void PluginLoader::shutdown() {
    // Unload all plugins in reverse order
    std::vector<std::string> pluginIds;
    for (const auto& [id, plugin] : plugins_) {
        pluginIds.push_back(id);
    }

    for (auto it = pluginIds.rbegin(); it != pluginIds.rend(); ++it) {
        unloadPlugin(*it);
    }

    plugins_.clear();
}

bool PluginLoader::loadPlugin(const std::string& path) {
    try {
        // Load the shared library
        void* handle = loadLibrary(path);
        if (!handle) {
            std::cerr << "Failed to load plugin library: " << path << std::endl;
            return false;
        }

        // Create plugin instance
        IPlugin* plugin = createPlugin(handle);
        if (!plugin) {
            std::cerr << "Failed to create plugin instance: " << path << std::endl;
            unloadLibrary(handle);
            return false;
        }

        // Get plugin info
        const PluginInfo& info = plugin->getInfo();

        // Check dependencies
        for (const auto& dep : info.dependencies) {
            if (plugins_.find(dep) == plugins_.end()) {
                std::cerr << "Plugin " << info.name << " requires missing dependency: " << dep << std::endl;
                plugin->shutdown();
                unloadLibrary(handle);
                return false;
            }
        }

        // Initialize plugin
        if (!plugin->initialize(nullptr)) {
            std::cerr << "Plugin initialization failed: " << info.name << std::endl;
            plugin->shutdown();
            unloadLibrary(handle);
            return false;
        }

        // Store loaded plugin
        plugins_[info.id] = {
            handle,
            std::unique_ptr<IPlugin>(plugin),
            info,
            path
        };

        std::cout << "Loaded plugin: " << info.name << " v" << info.version << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error loading plugin: " << e.what() << std::endl;
        return false;
    }
}

bool PluginLoader::unloadPlugin(const std::string& pluginId) {
    auto it = plugins_.find(pluginId);
    if (it == plugins_.end()) {
        return false;
    }

    // Shutdown plugin
    it->second.plugin->shutdown();

    // Unload library
    unloadLibrary(it->second.handle);

    // Remove from map
    plugins_.erase(it);

    std::cout << "Unloaded plugin: " << pluginId << std::endl;
    return true;
}

bool PluginLoader::reloadPlugin(const std::string& pluginId) {
    auto it = plugins_.find(pluginId);
    if (it == plugins_.end()) {
        return false;
    }

    std::string path = it->second.path;
    unloadPlugin(pluginId);
    return loadPlugin(path);
}

IPlugin* PluginLoader::getPlugin(const std::string& pluginId) {
    auto it = plugins_.find(pluginId);
    if (it != plugins_.end()) {
        return it->second.plugin.get();
    }
    return nullptr;
}

std::vector<std::string> PluginLoader::getLoadedPlugins() const {
    std::vector<std::string> ids;
    for (const auto& [id, plugin] : plugins_) {
        ids.push_back(id);
    }
    return ids;
}

std::vector<std::string> PluginLoader::discoverPlugins(const std::string& path) {
    std::vector<std::string> plugins;

    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
#ifdef _WIN32
                if (ext == ".dll") plugins.push_back(entry.path().string());
#elif defined(__APPLE__)
                if (ext == ".dylib") plugins.push_back(entry.path().string());
#else
                if (ext == ".so") plugins.push_back(entry.path().string());
#endif
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error discovering plugins: " << e.what() << std::endl;
    }

    return plugins;
}

void* PluginLoader::loadLibrary(const std::string& path) {
    return Platform::PlatformAbstraction::loadLibrary(path);
}

void PluginLoader::unloadLibrary(void* handle) {
    Platform::PlatformAbstraction::unloadLibrary(handle);
}

IPlugin* PluginLoader::createPlugin(void* handle) {
    using CreatePluginFunc = IPlugin*(*)();
    CreatePluginFunc createFunc = reinterpret_cast<CreatePluginFunc>(
        Platform::PlatformAbstraction::getProcAddress(handle, "createPlugin")
    );

    if (createFunc) {
        return createFunc();
    }

    return nullptr;
}

} // namespace NexusForge::Core
