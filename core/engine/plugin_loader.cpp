// core/engine/plugin_loader.cpp
#include "plugin_loader.hpp"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace NexusForge::Core {

PluginLoader::PluginLoader() = default;

PluginLoader::~PluginLoader() {
    unloadAllPlugins();
}

bool PluginLoader::loadPlugin(const std::string& path) {
    // Load dynamic library
    void* handle = Platform::loadLibrary(path);
    if (!handle) {
        std::cerr << "Failed to load plugin: " << path << std::endl;
        return false;
    }

    // Get create function
    using CreateFunc = IPlugin*(*)();
    auto createFunc = reinterpret_cast<CreateFunc>(Platform::getProcAddress(handle, "createPlugin"));
    if (!createFunc) {
        std::cerr << "Plugin missing createPlugin function: " << path << std::endl;
        Platform::unloadLibrary(handle);
        return false;
    }

    // Create plugin instance
    IPlugin* plugin = createFunc();
    if (!plugin) {
        std::cerr << "Failed to create plugin instance: " << path << std::endl;
        Platform::unloadLibrary(handle);
        return false;
    }

    // Get plugin info
    const PluginInfo& info = plugin->getInfo();
    
    // Check dependencies
    if (!checkDependencies(info)) {
        std::cerr << "Plugin has unsatisfied dependencies: " << info.name << std::endl;
        plugin->shutdown();
        Platform::unloadLibrary(handle);
        return false;
    }

    // Initialize plugin
    if (!plugin->initialize()) {
        std::cerr << "Plugin initialization failed: " << info.name << std::endl;
        plugin->shutdown();
        Platform::unloadLibrary(handle);
        return false;
    }

    // Store plugin
    plugins_[info.id] = info;
    instances_[info.id] = std::unique_ptr<IPlugin>(plugin);
    handles_[info.id] = handle;

    std::cout << "Loaded plugin: " << info.name << " v" << info.version << std::endl;
    notifyLoaded(info.id);

    return true;
}

bool PluginLoader::unloadPlugin(const std::string& id) {
    auto it = instances_.find(id);
    if (it == instances_.end()) {
        return false;
    }

    // Shutdown plugin
    it->second->shutdown();

    // Unload library
    auto handleIt = handles_.find(id);
    if (handleIt != handles_.end()) {
        Platform::unloadLibrary(handleIt->second);
        handles_.erase(handleIt);
    }

    std::cout << "Unloaded plugin: " << id << std::endl;
    notifyUnloaded(id);

    plugins_.erase(id);
    instances_.erase(it);

    return true;
}

bool PluginLoader::enablePlugin(const std::string& id) {
    auto it = plugins_.find(id);
    if (it == plugins_.end()) {
        return false;
    }
    it->second.enabled = true;
    return true;
}

bool PluginLoader::disablePlugin(const std::string& id) {
    auto it = plugins_.find(id);
    if (it == plugins_.end()) {
        return false;
    }
    it->second.enabled = false;
    return true;
}

IPlugin* PluginLoader::getPlugin(const std::string& id) {
    auto it = instances_.find(id);
    if (it != instances_.end()) {
        return it->second.get();
    }
    return nullptr;
}

const PluginInfo* PluginLoader::getPluginInfo(const std::string& id) const {
    auto it = plugins_.find(id);
    if (it != plugins_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> PluginLoader::getLoadedPlugins() const {
    std::vector<std::string> ids;
    for (const auto& [id, info] : plugins_) {
        if (info.loaded) {
            ids.push_back(id);
        }
    }
    return ids;
}

std::vector<std::string> PluginLoader::getEnabledPlugins() const {
    std::vector<std::string> ids;
    for (const auto& [id, info] : plugins_) {
        if (info.loaded && info.enabled) {
            ids.push_back(id);
        }
    }
    return ids;
}

void PluginLoader::setPluginDirectory(const std::string& path) {
    pluginDir_ = path;
}

void PluginLoader::scanPlugins() {
    if (pluginDir_.empty()) {
        return;
    }

    try {
        for (const auto& entry : fs::directory_iterator(pluginDir_)) {
            if (entry.is_regular_file()) {
                std::string path = entry.path().string();
                std::string ext = entry.path().extension().string();

#ifdef _WIN32
                if (ext == ".dll") {
#elif defined(__APPLE__)
                if (ext == ".dylib") {
#else
                if (ext == ".so") {
#endif
                    loadPlugin(path);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning plugins: " << e.what() << std::endl;
    }
}

void PluginLoader::loadAllPlugins() {
    scanPlugins();
}

void PluginLoader::unloadAllPlugins() {
    // Unload in reverse order
    std::vector<std::string> ids;
    for (const auto& [id, info] : plugins_) {
        ids.push_back(id);
    }

    for (auto it = ids.rbegin(); it != ids.rend(); ++it) {
        unloadPlugin(*it);
    }
}

void PluginLoader::addPluginLoadedCallback(PluginCallback callback) {
    loadedCallbacks_.push_back(callback);
}

void PluginLoader::addPluginUnloadedCallback(PluginCallback callback) {
    unloadedCallbacks_.push_back(callback);
}

IPlugin* PluginLoader::createPluginInstance(void* handle) {
    // Would use factory function from plugin
    return nullptr;
}

bool PluginLoader::checkDependencies(const PluginInfo& info) {
    for (const auto& dep : info.dependencies) {
        auto it = plugins_.find(dep);
        if (it == plugins_.end() || !it->second.loaded) {
            return false;
        }
    }
    return true;
}

void PluginLoader::notifyLoaded(const std::string& id) {
    for (auto& callback : loadedCallbacks_) {
        callback(id);
    }
}

void PluginLoader::notifyUnloaded(const std::string& id) {
    for (auto& callback : unloadedCallbacks_) {
        callback(id);
    }
}

} // namespace NexusForge::Core
