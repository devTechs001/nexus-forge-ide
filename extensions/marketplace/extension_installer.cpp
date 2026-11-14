// extensions/marketplace/extension_installer.cpp
// Extension Installer Implementation

#include "extension_installer.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

namespace NexusForge::Extensions {

ExtensionInstaller::ExtensionInstaller() = default;
ExtensionInstaller::~ExtensionInstaller() = default;

InstallResult ExtensionInstaller::install(const std::string& extensionId, const std::string& version) {
    return InstallResult::Success;
}

InstallResult ExtensionInstaller::installFromFile(const std::string& path) {
    return InstallResult::Success;
}

InstallResult ExtensionInstaller::installFromVSIX(const std::string& path) {
    return InstallResult::Success;
}

bool ExtensionInstaller::uninstall(const std::string& extensionId) {
    return true;
}

bool ExtensionInstaller::uninstall(const std::string& extensionId, bool disableFirst) {
    return true;
}

bool ExtensionInstaller::checkForUpdates(const std::string& extensionId) {
    return false;
}

std::vector<std::string> ExtensionInstaller::getAvailableUpdates() {
    return {};
}

InstallResult ExtensionInstaller::update(const std::string& extensionId) {
    return InstallResult::Success;
}

void ExtensionInstaller::updateAll() {
}

bool ExtensionInstaller::download(const std::string& extensionId, const std::string& destination) {
    return true;
}

bool ExtensionInstaller::validatePackage(const std::string& path) {
    return true;
}

bool ExtensionInstaller::extractPackage(const std::string& packagePath, const std::string& destination) {
    return true;
}

void ExtensionInstaller::setExtensionsDirectory(const std::string& path) {
    extensionsDir_ = path;
}

void ExtensionInstaller::setTempDirectory(const std::string& path) {
    tempDir_ = path;
}

InstallResult ExtensionInstaller::installInternal(const std::string& extensionId,
                                                    const std::string& packagePath) {
    return InstallResult::Success;
}

bool ExtensionInstaller::verifySignature(const std::string& packagePath) {
    return true;
}

bool ExtensionInstaller::installDependencies(const std::vector<std::string>& dependencies) {
    return true;
}

void ExtensionInstaller::notifyProgress(float progress, const std::string& message) {
    if (progressCallback_) progressCallback_("", progress, message);
}

void ExtensionInstaller::notifyStatus(const std::string& status) {
    if (statusCallback_) statusCallback_("", status);
}

} // namespace NexusForge::Extensions