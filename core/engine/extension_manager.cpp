// core/engine/extension_manager.cpp
#include "extension_manager.hpp"
#include "nexus_core.hpp"
#include <iostream>

namespace NexusForge::Core {

ExtensionManager::ExtensionManager(NexusEngine* engine) : engine_(engine) {}

ExtensionManager::~ExtensionManager() {
    shutdown();
}

bool ExtensionManager::initialize(const std::string& extensionsPath) {
    extensionsPath_ = extensionsPath;
    std::cout << "ExtensionManager initialized with path: " << extensionsPath_ << std::endl;
    return true;
}

void ExtensionManager::shutdown() {
    for (auto& [id, ext] : extensions_) {
        if (ext) {
            ext->deactivate();
        }
    }
    extensions_.clear();
}

bool ExtensionManager::loadExtension(const std::string& path) {
    std::cout << "Loading extension from: " << path << std::endl;
    // Placeholder - actual implementation would load extension
    return true;
}

bool ExtensionManager::unloadExtension(const std::string& extensionId) {
    auto it = extensions_.find(extensionId);
    if (it != extensions_.end()) {
        it->second->deactivate();
        extensions_.erase(it);
        return true;
    }
    return false;
}

bool ExtensionManager::enableExtension(const std::string& extensionId) {
    auto it = extensionInfos_.find(extensionId);
    if (it != extensionInfos_.end()) {
        it->second.enabled = true;
        return true;
    }
    return false;
}

bool ExtensionManager::disableExtension(const std::string& extensionId) {
    auto it = extensionInfos_.find(extensionId);
    if (it != extensionInfos_.end()) {
        it->second.enabled = false;
        return true;
    }
    return false;
}

bool ExtensionManager::loadAllExtensions() {
    std::cout << "Loading all extensions from: " << extensionsPath_ << std::endl;
    // Placeholder - would load all extensions
    return true;
}

IExtension* ExtensionManager::getExtension(const std::string& extensionId) {
    auto it = extensions_.find(extensionId);
    if (it != extensions_.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<ExtensionInfo> ExtensionManager::getAllExtensions() const {
    std::vector<ExtensionInfo> result;
    for (const auto& [id, info] : extensionInfos_) {
        result.push_back(info);
    }
    return result;
}

std::vector<ExtensionInfo> ExtensionManager::getLoadedExtensions() const {
    std::vector<ExtensionInfo> result;
    for (const auto& [id, info] : extensionInfos_) {
        if (info.loaded) {
            result.push_back(info);
        }
    }
    return result;
}

std::vector<ExtensionInfo> ExtensionManager::getEnabledExtensions() const {
    std::vector<ExtensionInfo> result;
    for (const auto& [id, info] : extensionInfos_) {
        if (info.enabled) {
            result.push_back(info);
        }
    }
    return result;
}

void ExtensionManager::update(double deltaTime) {
    for (auto& [id, ext] : extensions_) {
        if (ext && extensionInfos_[id].enabled) {
            ext->update(deltaTime);
        }
    }
}

} // namespace NexusForge::Core
