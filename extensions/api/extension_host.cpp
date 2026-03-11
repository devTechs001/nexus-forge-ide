// extensions/api/extension_host.cpp
#include "extension_host.hpp"
#include <iostream>
#include <fstream>

namespace NexusForge::Extensions {

ExtensionHost::ExtensionHost() = default;

ExtensionHost::~ExtensionHost() {
    deactivateAllExtensions();
}

bool ExtensionHost::loadExtension(const std::string& path) {
    // Parse manifest
    ExtensionManifest manifest = parseManifest(path);
    if (manifest.id.empty()) {
        std::cerr << "Failed to load extension from " << path << std::endl;
        return false;
    }

    // Check dependencies
    if (!satisfiesDependencies(manifest)) {
        std::cerr << "Extension " << manifest.id << " has unsatisfied dependencies" << std::endl;
        return false;
    }

    // Create extension context
    auto context = std::make_unique<ExtensionContext>(manifest.id, path);

    // Load extension (simplified - would use dynamic loading in real impl)
    auto extension = createExtension(path);
    if (!extension) {
        std::cerr << "Failed to create extension instance for " << manifest.id << std::endl;
        return false;
    }

    // Store extension
    LoadedExtension loaded;
    loaded.extension = std::move(extension);
    loaded.context = std::move(context);
    loaded.active = false;
    extensions_[manifest.id] = std::move(loaded);

    std::cout << "Loaded extension: " << manifest.displayName << std::endl;
    return true;
}

bool ExtensionHost::unloadExtension(const std::string& id) {
    auto it = extensions_.find(id);
    if (it == extensions_.end()) {
        return false;
    }

    if (it->second.active) {
        deactivateExtension(id);
    }

    extensions_.erase(it);
    std::cout << "Unloaded extension: " << id << std::endl;
    return true;
}

void ExtensionHost::activateExtension(const std::string& id) {
    auto it = extensions_.find(id);
    if (it == extensions_.end() || it->second.active) {
        return;
    }

    try {
        it->second.extension->activate(*it->second.context);
        it->second.active = true;
        std::cout << "Activated extension: " << id << std::endl;

        if (onExtensionActivated) {
            onExtensionActivated(id);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to activate extension " << id << ": " << e.what() << std::endl;
        if (onExtensionError) {
            onExtensionError(id, e.what());
        }
    }
}

void ExtensionHost::deactivateExtension(const std::string& id) {
    auto it = extensions_.find(id);
    if (it == extensions_.end() || !it->second.active) {
        return;
    }

    try {
        it->second.extension->deactivate();
        it->second.active = false;
        std::cout << "Deactivated extension: " << id << std::endl;

        if (onExtensionDeactivated) {
            onExtensionDeactivated(id);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to deactivate extension " << id << ": " << e.what() << std::endl;
    }
}

void ExtensionHost::activateAllExtensions() {
    for (auto& [id, ext] : extensions_) {
        if (!ext.active) {
            activateExtension(id);
        }
    }
}

void ExtensionHost::deactivateAllExtensions() {
    for (auto& [id, ext] : extensions_) {
        if (ext.active) {
            deactivateExtension(id);
        }
    }
}

IExtension* ExtensionHost::getExtension(const std::string& id) {
    auto it = extensions_.find(id);
    if (it != extensions_.end()) {
        return it->second.extension.get();
    }
    return nullptr;
}

std::vector<ExtensionManifest> ExtensionHost::getInstalledExtensions() const {
    std::vector<ExtensionManifest> manifests;
    for (const auto& [id, ext] : extensions_) {
        // Would get manifest from loaded extension
    }
    return manifests;
}

std::vector<std::string> ExtensionHost::getActiveExtensions() const {
    std::vector<std::string> active;
    for (const auto& [id, ext] : extensions_) {
        if (ext.active) {
            active.push_back(id);
        }
    }
    return active;
}

ExtensionManifest ExtensionHost::parseManifest(const std::string& path) {
    ExtensionManifest manifest;
    // Would parse package.json or similar
    return manifest;
}

bool ExtensionHost::satisfiesDependencies(const ExtensionManifest& manifest) {
    for (const auto& dep : manifest.dependencies) {
        // Check if dependency is satisfied
        auto it = extensions_.find(dep);
        if (it == extensions_.end()) {
            return false;
        }
    }
    return true;
}

IExtension* ExtensionHost::createExtension(const std::string& path) {
    // Would use dynamic loading to create extension instance
    return nullptr;
}

} // namespace NexusForge::Extensions
