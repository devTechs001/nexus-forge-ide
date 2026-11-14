// extensions/marketplace/marketplace_client.hpp
#pragma once

#include "../api/extension_api.hpp"

#include <vector>
#include <string>
#include <functional>

namespace NexusForge::Extensions {

// Extension version info
struct ExtensionVersion {
    std::string version;
    std::string date;
    std::string downloadUrl;
    std::string changelog;
    bool isPreRelease = false;
    std::vector<std::string> platforms;
};

// Extension review
struct ExtensionReview {
    int rating;  // 1-5
    std::string title;
    std::string comment;
    std::string author;
    std::string date;
    int helpfulCount = 0;
};

// Extension info (lightweight)
struct ExtensionInfo {
    std::string id;
    std::string name;
    std::string displayName;
    std::string description;
    std::string version;
    std::string publisher;
    std::string icon;
    std::string galleryBanner;

    std::vector<std::string> categories;
    std::vector<std::string> keywords;
    std::vector<std::string> tags;

    long long downloadCount = 0;
    double rating = 0.0;
    int reviewCount = 0;

    std::string repository;
    std::string homepage;
    std::string bugs;
    std::string license;

    std::vector<std::string> platforms;
    std::vector<std::string> engines;

    bool verified = false;
    bool featured = false;
    bool deprecated = false;

    // Transient state
    bool installed = false;
    bool disabled = false;
    bool hasUpdate = false;
    std::string latestVersion;
    bool hovered = false;
};

// Search options
struct SearchOptions {
    std::string category;
    std::string sortBy = "relevance";  // relevance, downloads, rating, date, name
    std::string sortOrder = "desc";    // asc, desc
    int limit = 20;
    int offset = 0;
    std::vector<std::string> platforms;
    bool includePreRelease = false;
};

// Download result
struct DownloadResult {
    bool success;
    std::string downloadedPath;
    std::string version;
    std::string errorMessage;
    long long bytesDownloaded = 0;
    long long totalBytes = 0;
};

// Update info
struct UpdateInfo {
    bool hasUpdate;
    std::string latestVersion;
    std::string downloadUrl;
    std::string releaseNotes;
    bool isCritical = false;
    bool isPreRelease = false;
};

// Marketplace client
class MarketplaceClient {
public:
    MarketplaceClient();
    ~MarketplaceClient();

    // Configuration
    void setMarketplaceUrl(const std::string& url);
    const std::string& getMarketplaceUrl() const { return marketplaceUrl_; }

    // Search
    std::vector<ExtensionInfo> searchExtensions(const std::string& query,
                                                 const SearchOptions& options = SearchOptions());
    ExtensionInfo getExtensionDetails(const std::string& extensionId);
    std::vector<ExtensionVersion> getExtensionVersions(const std::string& extensionId);

    // Discovery
    std::vector<ExtensionInfo> getPopularExtensions(int limit = 20);
    std::vector<ExtensionInfo> getRecommendedExtensions();
    std::vector<ExtensionInfo> getPublisherExtensions(const std::string& publisher);
    std::vector<ExtensionInfo> getRelatedExtensions(const std::string& extensionId);
    std::vector<ExtensionInfo> getFeaturedExtensions();
    std::vector<ExtensionInfo> getNewExtensions(int limit = 20);

    // Reviews
    std::vector<ExtensionReview> getExtensionReviews(const std::string& extensionId);
    bool submitReview(const std::string& extensionId, const ExtensionReview& review);

    // Download & Install
    DownloadResult downloadExtension(const std::string& extensionId,
                                      const std::string& version,
                                      const std::string& destinationPath);
    bool installExtension(const std::string& extensionId,
                          const std::string& version = "",
                          bool enable = true);
    bool uninstallExtension(const std::string& extensionId);

    // Updates
    UpdateInfo checkForUpdates(const std::string& extensionId,
                                const std::string& currentVersion);
    std::vector<UpdateInfo> checkForUpdates(
        const std::vector<std::pair<std::string, std::string>>& installedExtensions);

    // Async operations
    using SearchCallback = std::function<void(const std::vector<ExtensionInfo>&, const std::string&)>;
    using DownloadProgress = std::function<void(const std::string&, float)>;
    using DownloadCallback = std::function<void(const DownloadResult&)>;

    void searchExtensionsAsync(const std::string& query, const SearchOptions& options,
                               SearchCallback callback);
    void downloadExtensionAsync(const std::string& extensionId, const std::string& version,
                                const std::string& destinationPath,
                                DownloadProgress progress, DownloadCallback callback);

    // Cache
    void clearCache();
    void setCacheEnabled(bool enabled);
    bool isCacheEnabled() const { return cacheEnabled_; }

    // Authentication
    void setAuthToken(const std::string& token);
    bool isAuthenticated() const { return authenticated_; }

    // Helpers
    SearchOptions getDefaultSearchOptions() const;

private:
    std::string marketplaceUrl_;
    std::string authToken_;
    bool cacheEnabled_ = true;
    bool authenticated_ = false;

    // Cache
    std::unordered_map<std::string, ExtensionInfo> extensionCache_;
    std::unordered_map<std::string, std::vector<ExtensionInfo>> searchCache_;

    // HTTP helpers (would use networking module in production)
    std::string httpGet(const std::string& url);
    std::string httpPost(const std::string& url, const std::string& data);
};

} // namespace NexusForge::Extensions
