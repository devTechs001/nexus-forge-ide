// core/engine/plugin_loader.cpp
#include "plugin_loader.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace fs = std::filesystem;

namespace NexusForge::Core {

PluginLoader::PluginLoader() = default;

PluginLoader::~PluginLoader() {
    // Unload all plugins
    for (auto& [id, plugin] : plugins_) {
        if (plugin.instance) {
            plugin.instance->shutdown();
        }
        if (plugin.nativeHandle) {
            unloadNativeLibrary(plugin.nativeHandle);
        }
    }
    plugins_.clear();
}

PluginLoadResult PluginLoader::loadPlugin(const std::string& path) {
    // Check if plugin is already loaded
    if (!fs::exists(path)) {
        return PluginLoadResult::NotFound;
    }

    // Validate plugin
    if (!validatePlugin(path)) {
        return PluginLoadResult::InvalidFormat;
    }

    // Parse plugin info
    PluginInfo info = parsePluginInfo(path);
    if (info.id.empty()) {
        return PluginLoadResult::InvalidFormat;
    }

    // Check if already loaded
    if (plugins_.find(info.id) != plugins_.end()) {
        std::cerr << "Plugin " << info.id << " is already loaded" << std::endl;
        return PluginLoadResult::Success; // Already loaded
    }

    // Check dependencies
    if (!checkDependencies(info)) {
        return PluginLoadResult::DependencyMissing;
    }

    // Load native library
    void* handle = loadNativeLibrary(path);
    if (!handle) {
        std::cerr << "Failed to load plugin library: " << path << std::endl;
        return PluginLoadResult::InvalidFormat;
    }

    // Get plugin entry points
    CreatePluginFunc createFunc = reinterpret_cast<CreatePluginFunc>(
        getFunctionAddress(handle, "createPlugin")
    );
    DestroyPluginFunc destroyFunc = reinterpret_cast<DestroyPluginFunc>(
        getFunctionAddress(handle, "destroyPlugin")
    );

    if (!createFunc || !destroyFunc) {
        std::cerr << "Plugin missing required entry points" << std::endl;
        unloadNativeLibrary(handle);
        return PluginLoadResult::InvalidFormat;
    }

    // Create plugin instance
    IPlugin* plugin = createFunc();
    if (!plugin) {
        std::cerr << "Failed to create plugin instance" << std::endl;
        unloadNativeLibrary(handle);
        return PluginLoadResult::InitFailed;
    }

    // Initialize plugin
    if (!plugin->initialize()) {
        std::cerr << "Plugin initialization failed" << std::endl;
        destroyFunc(plugin);
        unloadNativeLibrary(handle);
        return PluginLoadResult::InitFailed;
    }

    // Store loaded plugin
    LoadedPlugin loaded;
    loaded.info = info;
    loaded.instance.reset(plugin);
    loaded.nativeHandle = handle;

    plugins_[info.id] = std::move(loaded);

    std::cout << "Loaded plugin: " << info.name << " v" << info.version << std::endl;

    if (onPluginLoaded) {
        onPluginLoaded(info.id);
    }

    return PluginLoadResult::Success;
}

bool PluginLoader::unloadPlugin(const std::string& pluginId) {
    auto it = plugins_.find(pluginId);
    if (it == plugins_.end()) {
        return false;
    }

    // Deactivate if active
    if (it->second.active && it->second.instance) {
        it->second.instance->shutdown();
    }

    // Destroy plugin instance
    if (it->second.instance) {
        it->second.instance.reset();
    }

    // Unload native library
    if (it->second.nativeHandle) {
        unloadNativeLibrary(it->second.nativeHandle);
    }

    std::cout << "Unloaded plugin: " << pluginId << std::endl;

    plugins_.erase(it);

    if (onPluginUnloaded) {
        onPluginUnloaded(pluginId);
    }

    return true;
}

bool PluginLoader::activatePlugin(const std::string& pluginId) {
    auto it = plugins_.find(pluginId);
    if (it == plugins_.end() || !it->second.instance) {
        return false;
    }

    if (!it->second.active) {
        it->second.active = true;
        std::cout << "Activated plugin: " << pluginId << std::endl;

        if (onPluginActivated) {
            onPluginActivated(pluginId);
        }
    }

    return true;
}

bool PluginLoader::deactivatePlugin(const std::string& pluginId) {
    auto it = plugins_.find(pluginId);
    if (it == plugins_.end()) {
        return false;
    }

    if (it->second.active) {
        it->second.active = false;
        std::cout << "Deactivated plugin: " << pluginId << std::endl;

        if (onPluginDeactivated) {
            onPluginDeactivated(pluginId);
        }
    }

    return true;
}

IPlugin* PluginLoader::getPlugin(const std::string& pluginId) {
    auto it = plugins_.find(pluginId);
    if (it != plugins_.end()) {
        return it->second.instance.get();
    }
    return nullptr;
}

const std::vector<PluginInfo>& PluginLoader::getLoadedPlugins() const {
    pluginDirectory_.clear();
    for (const auto& [id, plugin] : plugins_) {
        pluginDirectory_.push_back(plugin.info);
    }
    return pluginDirectory_;
}

std::vector<std::string> PluginLoader::getActivePluginIds() const {
    std::vector<std::string> activeIds;
    for (const auto& [id, plugin] : plugins_) {
        if (plugin.active) {
            activeIds.push_back(id);
        }
    }
    return activeIds;
}

std::vector<std::string> PluginLoader::scanPluginDirectory(const std::string& directory) {
    std::vector<std::string> pluginPaths;

    if (!fs::exists(directory)) {
        return pluginPaths;
    }

    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            std::string path = entry.path().string();
            std::string ext = entry.path().extension().string();

#ifdef _WIN32
            if (ext == ".dll") {
#else
            if (ext == ".so" || ext == ".dylib") {
#endif
                if (validatePlugin(path)) {
                    pluginPaths.push_back(path);
                }
            }
        }
    }

    return pluginPaths;
}

bool PluginLoader::loadAllPlugins(const std::string& directory) {
    auto plugins = scanPluginDirectory(directory);
    bool allLoaded = true;

    for (const auto& path : plugins) {
        PluginLoadResult result = loadPlugin(path);
        if (result != PluginLoadResult::Success &&
            result != PluginLoadResult::Success) {
            allLoaded = false;
        }
    }

    return allLoaded;
}

void PluginLoader::registerPlugin(std::unique_ptr<IPlugin> plugin) {
    if (!plugin) return;

    const PluginInfo& info = plugin->getInfo();
    
    LoadedPlugin loaded;
    loaded.info = info;
    loaded.instance = std::move(plugin);
    loaded.nativeHandle = nullptr;

    plugins_[info.id] = std::move(loaded);

    std::cout << "Registered plugin: " << info.name << std::endl;

    if (onPluginLoaded) {
        onPluginLoaded(info.id);
    }
}

void* PluginLoader::loadNativeLibrary(const std::string& path) {
#ifdef _WIN32
    return LoadLibraryA(path.c_str());
#else
    return dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
#endif
}

void PluginLoader::unloadNativeLibrary(void* handle) {
    if (!handle) return;

#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

void* PluginLoader::getFunctionAddress(void* handle, const std::string& name) {
#ifdef _WIN32
    return GetProcAddress(static_cast<HMODULE>(handle), name.c_str());
#else
    return dlsym(handle, name.c_str());
#endif
}

bool PluginLoader::validatePlugin(const std::string& path) {
    // Check if file exists
    if (!fs::exists(path)) {
        return false;
    }

    // Check file size (basic sanity check)
    auto size = fs::file_size(path);
    if (size < 1024 || size > 100 * 1024 * 1024) { // 1KB to 100MB
        return false;
    }

    // Try to load and check for required symbols
    void* handle = loadNativeLibrary(path);
    if (!handle) {
        return false;
    }

    bool valid = true;
    
    // Check for required entry points
    if (!getFunctionAddress(handle, "createPlugin")) {
        valid = false;
    }
    if (!getFunctionAddress(handle, "destroyPlugin")) {
        valid = false;
    }

    unloadNativeLibrary(handle);
    return valid;
}

PluginInfo PluginLoader::parsePluginInfo(const std::string& path) {
    PluginInfo info;
    info.path = path;

    // Look for plugin manifest (JSON file next to the library)
    fs::path pluginPath(path);
    fs::path manifestPath = pluginPath.parent_path() / "plugin.json";

    if (fs::exists(manifestPath)) {
        std::ifstream file(manifestPath);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();

            // Simple JSON parsing (in production, use a proper JSON library)
            auto findValue = [&content](const std::string& key) -> std::string {
                std::string searchKey = "\"" + key + "\"";
                size_t pos = content.find(searchKey);
                if (pos == std::string::npos) return "";

                pos = content.find(":", pos);
                if (pos == std::string::npos) return "";

                pos = content.find("\"", pos);
                if (pos == std::string::npos) return "";

                size_t endPos = content.find("\"", pos + 1);
                if (endPos == std::string::npos) return "";

                return content.substr(pos + 1, endPos - pos - 1);
            };

            info.id = findValue("id");
            info.name = findValue("name");
            info.version = findValue("version");
            info.description = findValue("description");
            info.author = findValue("author");
        }
    }

    // Fallback: use filename as ID
    if (info.id.empty()) {
        info.id = pluginPath.stem().string();
    }
    if (info.name.empty()) {
        info.name = info.id;
    }
    if (info.version.empty()) {
        info.version = "1.0.0";
    }

    return info;
}

bool PluginLoader::checkDependencies(const PluginInfo& info) {
    // In a full implementation, this would check:
    // - Required plugin dependencies
    // - API version compatibility
    // - Platform compatibility
    // For now, just return true
    return true;
}

} // namespace NexusForge::Core
