// extensions/marketplace/marketplace_client.cpp
#include "marketplace_client.hpp"
#include <algorithm>
#include <iostream>

namespace NexusForge::Extensions {

MarketplaceClient::MarketplaceClient()
    : apiEndpoint_("https://marketplace.nexusforge.dev/api") {}

MarketplaceClient::~MarketplaceClient() = default;

std::vector<ExtensionPackage> MarketplaceClient::search(
    const std::string& query, const std::vector<std::string>& categories) {
    
    // Build search URL
    std::string url = apiEndpoint_ + "/extensions/search?q=" + query;
    for (const auto& cat : categories) {
        url += "&category=" + cat;
    }

    // Would make HTTP request in real implementation
    std::cout << "Searching: " << url << std::endl;
    return {};
}

std::vector<ExtensionPackage> MarketplaceClient::getPopular(int count) {
    std::string url = apiEndpoint_ + "/extensions/popular?limit=" + std::to_string(count);
    return fetchExtensions(url);
}

std::vector<ExtensionPackage> MarketplaceClient::getTrending(int count) {
    std::string url = apiEndpoint_ + "/extensions/trending?limit=" + std::to_string(count);
    return fetchExtensions(url);
}

std::vector<ExtensionPackage> MarketplaceClient::getNew(int count) {
    std::string url = apiEndpoint_ + "/extensions/new?limit=" + std::to_string(count);
    return fetchExtensions(url);
}

std::vector<ExtensionPackage> MarketplaceClient::getByCategory(const std::string& category) {
    std::string url = apiEndpoint_ + "/extensions/category/" + category;
    return fetchExtensions(url);
}

std::vector<ExtensionPackage> MarketplaceClient::getByPublisher(const std::string& publisher) {
    std::string url = apiEndpoint_ + "/extensions/publisher/" + publisher;
    return fetchExtensions(url);
}

ExtensionPackage MarketplaceClient::getDetails(const std::string& extensionId) {
    std::string url = apiEndpoint_ + "/extensions/" + extensionId;
    // Would fetch and parse response
    return ExtensionPackage{};
}

std::string MarketplaceClient::getReadme(const std::string& extensionId) {
    std::string url = apiEndpoint_ + "/extensions/" + extensionId + "/readme";
    // Would fetch readme
    return "";
}

std::vector<std::string> MarketplaceClient::getVersions(const std::string& extensionId) {
    std::string url = apiEndpoint_ + "/extensions/" + extensionId + "/versions";
    // Would fetch version list
    return {};
}

std::string MarketplaceClient::getDownloadUrl(const std::string& extensionId,
                                                const std::string& version) {
    if (version.empty()) {
        return apiEndpoint_ + "/extensions/" + extensionId + "/download";
    }
    return apiEndpoint_ + "/extensions/" + extensionId + "/download/" + version;
}

void MarketplaceClient::setApiEndpoint(const std::string& url) {
    apiEndpoint_ = url;
}

void MarketplaceClient::setApiKey(const std::string& key) {
    apiKey_ = key;
}

std::vector<ExtensionPackage> MarketplaceClient::fetchExtensions(const std::string& url) {
    // Would make HTTP request and parse JSON response
    std::cout << "Fetching: " << url << std::endl;
    return {};
}

} // namespace NexusForge::Extensions
