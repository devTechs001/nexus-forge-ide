// core/engine/plugin_loader.cpp
#include "plugin_loader.hpp"
#include <iostream>
#include <algorithm>

namespace NexusForge::Core {

PluginLoader::PluginLoader() = default;

PluginLoader::~PluginLoader() {
    shutdown();
}

bool PluginLoader::initialize(const std::string& pluginsPath) {
    pluginsPath_ = pluginsPath;
    return true;
}

void PluginLoader::shutdown() {
    unloadAllPlugins();
}

std::shared_ptr<IPlugin> PluginLoader::loadPlugin(const std::string& path) {
    std::cout << "Loading plugin from: " << path << std::endl;
    // Placeholder - actual implementation would use dlopen/LoadLibrary
    return nullptr;
}

void PluginLoader::unloadPlugin(std::shared_ptr<IPlugin> plugin) {
    if (plugin) {
        plugin->unload();
        plugins_.erase(
            std::remove(plugins_.begin(), plugins_.end(), plugin),
            plugins_.end()
        );
    }
}

void PluginLoader::unloadAllPlugins() {
    for (auto& plugin : plugins_) {
        if (plugin) {
            plugin->unload();
        }
    }
    plugins_.clear();
}

std::vector<PluginInfo> PluginLoader::scanPlugins() {
    // Placeholder - would scan directory for plugin files
    return {};
}

} // namespace NexusForge::Core
