// extensions/marketplace/extension_installer.hpp
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace NexusForge::Extensions {

// Extension package info
struct ExtensionPackage {
    std::string id;
    std::string name;
    std::string version;
    std::string publisher;
    std::string description;
    std::string downloadUrl;
    std::string sha256;
    size_t size = 0;
    std::vector<std::string> platforms;
    std::vector<std::string> engineVersions;
};

// Installation result
enum class InstallResult {
    Success,
    AlreadyInstalled,
    DownloadFailed,
    ValidationFailed,
    InstallFailed,
    Incompatible,
    DependencyMissing
};

// Extension installer
class ExtensionInstaller {
public:
    ExtensionInstaller();
    ~ExtensionInstaller();

    // Installation
    InstallResult install(const std::string& extensionId, const std::string& version = "");
    InstallResult installFromFile(const std::string& path);
    InstallResult installFromVSIX(const std::string& path);

    // Uninstallation
    bool uninstall(const std::string& extensionId);
    bool uninstall(const std::string& extensionId, bool disableFirst);

    // Updates
    bool checkForUpdates(const std::string& extensionId);
    std::vector<std::string> getAvailableUpdates();
    InstallResult update(const std::string& extensionId);
    void updateAll();

    // Download
    bool download(const std::string& extensionId, const std::string& destination);
    bool validatePackage(const std::string& path);
    bool extractPackage(const std::string& packagePath, const std::string& destination);

    // Configuration
    void setExtensionsDirectory(const std::string& path);
    std::string getExtensionsDirectory() const { return extensionsDir_; }

    void setTempDirectory(const std::string& path);
    std::string getTempDirectory() const { return tempDir_; }

    // Events
    using ProgressCallback = std::function<void(const std::string&, float, const std::string&)>;
    void setProgressCallback(ProgressCallback callback) { progressCallback_ = callback; }

    using StatusCallback = std::function<void(const std::string&, const std::string&)>;
    void setStatusCallback(StatusCallback callback) { statusCallback_ = callback; }

private:
    std::string extensionsDir_;
    std::string tempDir_;
    ProgressCallback progressCallback_;
    StatusCallback statusCallback_;

    InstallResult installInternal(const std::string& extensionId,
                                   const std::string& packagePath);
    bool verifySignature(const std::string& packagePath);
    bool installDependencies(const std::vector<std::string>& dependencies);
    void notifyProgress(float progress, const std::string& message);
    void notifyStatus(const std::string& status);
};

// Marketplace client
class MarketplaceClient {
public:
    MarketplaceClient();
    ~MarketplaceClient();

    // Search
    std::vector<ExtensionPackage> search(const std::string& query,
                                          const std::vector<std::string>& categories = {});

    // Browse
    std::vector<ExtensionPackage> getPopular(int count = 20);
    std::vector<ExtensionPackage> getTrending(int count = 20);
    std::vector<ExtensionPackage> getNew(int count = 20);
    std::vector<ExtensionPackage> getByCategory(const std::string& category);
    std::vector<ExtensionPackage> getByPublisher(const std::string& publisher);

    // Details
    ExtensionPackage getDetails(const std::string& extensionId);
    std::string getReadme(const std::string& extensionId);
    std::vector<std::string> getVersions(const std::string& extensionId);

    // Download
    std::string getDownloadUrl(const std::string& extensionId, const std::string& version = "");

    // Configuration
    void setApiEndpoint(const std::string& url);
    void setApiKey(const std::string& key);

private:
    std::string apiEndpoint_;
    std::string apiKey_;

    std::vector<ExtensionPackage> fetchExtensions(const std::string& url);
};

} // namespace NexusForge::Extensions
