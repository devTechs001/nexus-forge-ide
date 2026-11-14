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

std::vector<ExtensionPackage> MarketplaceClient::search(const std::string& query,
                                                         const std::vector<std::string>& categories) {
    (void)categories;
    std::vector<ExtensionPackage> packages;
    for (const auto& info : searchExtensions(query, getDefaultSearchOptions())) {
        ExtensionPackage pkg;
        pkg.id = info.id;
        pkg.name = info.name;
        pkg.version = info.version;
        pkg.publisher = info.publisher;
        pkg.description = info.description;
        pkg.downloadUrl = getDownloadUrl(info.id, info.version);
        pkg.platforms = info.platforms;
        packages.push_back(std::move(pkg));
    }
    return packages;
}

std::vector<ExtensionPackage> MarketplaceClient::getPopular(int count) {
    auto pkgs = search("*");
    if (count >= 0 && static_cast<size_t>(count) < pkgs.size()) {
        pkgs.resize(static_cast<size_t>(count));
    }
    return pkgs;
}

std::vector<ExtensionPackage> MarketplaceClient::getTrending(int count) {
    return getPopular(count);
}

std::vector<ExtensionPackage> MarketplaceClient::getNew(int count) {
    return getPopular(count);
}

std::vector<ExtensionPackage> MarketplaceClient::getByCategory(const std::string& category) {
    return search("*", {category});
}

std::vector<ExtensionPackage> MarketplaceClient::getByPublisher(const std::string& publisher) {
    std::vector<ExtensionPackage> packages;
    for (const auto& info : getPublisherExtensions(publisher)) {
        if (publisher.empty() || info.publisher == publisher) {
            ExtensionPackage pkg;
            pkg.id = info.id;
            pkg.name = info.name;
            pkg.version = info.version;
            pkg.publisher = info.publisher;
            pkg.description = info.description;
            pkg.downloadUrl = getDownloadUrl(info.id, info.version);
            packages.push_back(std::move(pkg));
        }
    }
    return packages;
}

ExtensionPackage MarketplaceClient::getDetails(const std::string& extensionId) {
    ExtensionInfo info = getExtensionDetails(extensionId);
    ExtensionPackage pkg;
    pkg.id = info.id;
    pkg.name = info.name;
    pkg.version = info.version;
    pkg.publisher = info.publisher;
    pkg.description = info.description;
    pkg.downloadUrl = getDownloadUrl(info.id, info.version);
    return pkg;
}

std::string MarketplaceClient::getReadme(const std::string& extensionId) {
    return "# " + extensionId + "\n";
}

std::vector<std::string> MarketplaceClient::getVersions(const std::string& extensionId) {
    std::vector<std::string> result;
    for (const auto& v : getExtensionVersions(extensionId)) {
        result.push_back(v.version);
    }
    return result;
}

std::string MarketplaceClient::getDownloadUrl(const std::string& extensionId, const std::string& version) {
    std::string base = !apiEndpoint_.empty() ? apiEndpoint_ : marketplaceUrl_;
    std::string ver = version.empty() ? "latest" : version;
    return base + "/extensions/" + extensionId + "/" + ver + "/download";
}

void MarketplaceClient::setApiEndpoint(const std::string& url) {
    apiEndpoint_ = url;
    marketplaceUrl_ = url;
}

void MarketplaceClient::setApiKey(const std::string& key) {
    apiKey_ = key;
    authToken_ = key;
    authenticated_ = !key.empty();
}

std::vector<ExtensionPackage> MarketplaceClient::fetchExtensions(const std::string& url) {
    (void)url;
    return {};
}

} // namespace NexusForge::Extensions
