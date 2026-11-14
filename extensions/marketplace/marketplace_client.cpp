// extensions/marketplace/marketplace_client.cpp
// Marketplace Client Implementation

#include "marketplace_client.hpp"
#include <sstream>
#include <algorithm>

namespace NexusForge::Extensions {

MarketplaceClient::MarketplaceClient() 
    : marketplaceUrl_("https://marketplace.nexusforge.com") {
}

MarketplaceClient::~MarketplaceClient() = default;

void MarketplaceClient::setMarketplaceUrl(const std::string& url) {
    marketplaceUrl_ = url;
}

std::vector<ExtensionInfo> MarketplaceClient::searchExtensions(const std::string& query,
                                                                const SearchOptions& options) {
    std::vector<ExtensionInfo> results;
    
    // In production, would query marketplace API
    // For now, return sample results
    
    if (query.empty() || query == "*") {
        ExtensionInfo ext1;
        ext1.id = "python";
        ext1.name = "Python";
        ext1.displayName = "Python Language Support";
        ext1.description = "Full Python language support with IntelliSense, linting, and debugging";
        ext1.version = "2024.1.0";
        ext1.publisher = "NexusForge";
        ext1.downloadCount = 1000000;
        ext1.rating = 4.8;
        ext1.categories = {"Programming Languages", "Linters", "Debuggers"};
        results.push_back(ext1);
        
        ExtensionInfo ext2;
        ext2.id = "prettier";
        ext2.name = "Prettier";
        ext2.displayName = "Prettier - Code Formatter";
        ext2.description = "Code formatter using prettier";
        ext2.version = "9.0.0";
        ext2.publisher = "Prettier";
        ext2.downloadCount = 500000;
        ext2.rating = 4.7;
        ext2.categories = {"Formatters"};
        results.push_back(ext2);
    }
    
    return results;
}

ExtensionInfo MarketplaceClient::getExtensionDetails(const std::string& extensionId) {
    ExtensionInfo info;
    
    // In production, would fetch from marketplace API
    info.id = extensionId;
    info.name = extensionId;
    info.displayName = extensionId;
    info.description = "Extension details would come from marketplace";
    info.version = "1.0.0";
    info.publisher = "Unknown";
    
    return info;
}

std::vector<ExtensionVersion> MarketplaceClient::getExtensionVersions(const std::string& extensionId) {
    std::vector<ExtensionVersion> versions;
    
    // In production, would fetch version history from API
    ExtensionVersion v1;
    v1.version = "1.0.0";
    v1.date = "2024-01-01";
    v1.downloadUrl = marketplaceUrl_ + "/extensions/" + extensionId + "/1.0.0/download";
    versions.push_back(v1);
    
    return versions;
}

std::vector<ExtensionInfo> MarketplaceClient::getPopularExtensions(int limit) {
    // In production, would fetch popular extensions from API
    return searchExtensions("*", SearchOptions());
}

std::vector<ExtensionInfo> MarketplaceClient::getRecommendedExtensions() {
    // In production, would fetch recommendations based on installed extensions
    return searchExtensions("*", SearchOptions());
}

std::vector<ExtensionInfo> MarketplaceClient::getPublisherExtensions(const std::string& publisher) {
    // In production, would fetch all extensions by publisher
    return searchExtensions("*", SearchOptions());
}

std::vector<ExtensionInfo> MarketplaceClient::getRelatedExtensions(const std::string& extensionId) {
    // In production, would fetch related extensions
    return searchExtensions("*", SearchOptions());
}

std::vector<ExtensionReview> MarketplaceClient::getExtensionReviews(const std::string& extensionId) {
    std::vector<ExtensionReview> reviews;
    
    // In production, would fetch reviews from API
    ExtensionReview review;
    review.rating = 5;
    review.title = "Great extension!";
    review.comment = "This extension is very useful.";
    review.author = "user123";
    review.date = "2024-01-15";
    reviews.push_back(review);
    
    return reviews;
}

bool MarketplaceClient::submitReview(const std::string& extensionId, const ExtensionReview& review) {
    // In production, would submit review to API
    return true;
}

DownloadResult MarketplaceClient::downloadExtension(const std::string& extensionId,
                                                     const std::string& version,
                                                     const std::string& destinationPath) {
    DownloadResult result;
    
    // In production, would download from marketplace
    result.success = true;
    result.downloadedPath = destinationPath + "/" + extensionId;
    result.version = version;
    
    return result;
}

UpdateInfo MarketplaceClient::checkForUpdates(const std::string& extensionId,
                                               const std::string& currentVersion) {
    UpdateInfo info;
    info.hasUpdate = false;
    
    // In production, would check against marketplace API
    // For demo, pretend there's an update
    if (currentVersion == "1.0.0") {
        info.hasUpdate = true;
        info.latestVersion = "1.1.0";
        info.downloadUrl = marketplaceUrl_ + "/extensions/" + extensionId + "/1.1.0/download";
        info.releaseNotes = "Bug fixes and improvements";
    }
    
    return info;
}

std::vector<UpdateInfo> MarketplaceClient::checkForUpdates(
    const std::vector<std::pair<std::string, std::string>>& installedExtensions) {
    
    std::vector<UpdateInfo> updates;
    
    for (const auto& [id, version] : installedExtensions) {
        UpdateInfo info = checkForUpdates(id, version);
        if (info.hasUpdate) {
            updates.push_back(info);
        }
    }
    
    return updates;
}

SearchOptions MarketplaceClient::getDefaultSearchOptions() const {
    SearchOptions options;
    options.category = "";
    options.sortBy = "relevance";
    options.sortOrder = "desc";
    options.limit = 20;
    options.offset = 0;
    return options;
}

} // namespace NexusForge::Extensions
