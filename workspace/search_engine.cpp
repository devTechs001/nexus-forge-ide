// workspace/search_engine.cpp
// Search Engine Implementation

#include "search_engine.hpp"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <iostream>

namespace NexusForge::Workspace {

SearchEngine::SearchEngine() = default;
SearchEngine::~SearchEngine() = default;

void SearchEngine::setRootDirectory(const std::string& path) {
    rootDir_ = path;
}

void SearchEngine::buildIndex() {
    if (rootDir_.empty() || !std::filesystem::exists(rootDir_)) return;
    indexing_ = true;
    indexProgress_ = 0;
    index_.clear();

    size_t total = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(rootDir_)) {
        if (entry.is_regular_file()) total++;
    }

    size_t processed = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(rootDir_)) {
        if (cancelRequested_) break;
        if (entry.is_regular_file()) {
            std::string path = entry.path().string();
            if (shouldInclude(path)) {
                indexFile(path);
            }
            processed++;
            indexProgress_ = (float)processed / total;
        }
    }

    indexing_ = false;
    indexProgress_ = 1.0f;
}

void SearchEngine::updateIndex(const std::string& filePath) {
    if (shouldInclude(filePath)) {
        indexFile(filePath);
    }
}

void SearchEngine::removeFromIndex(const std::string& filePath) {
    index_.erase(filePath);
}

void SearchEngine::clearIndex() {
    index_.clear();
}

std::vector<SearchResult> SearchEngine::searchFiles(const std::string& pattern,
                                                     const SearchOptions& options) {
    std::vector<SearchResult> results;
    for (const auto& [path, entry] : index_) {
        if (results.size() >= maxResults_) break;
        if (matchesPattern(entry.fileName, pattern)) {
            SearchResult r;
            r.type = ResultType::File;
            r.filePath = path;
            r.fileName = entry.fileName;
            results.push_back(r);
        }
    }
    return results;
}

std::vector<SearchResult> SearchEngine::searchText(const SearchOptions& options) {
    if (options.useRegex) {
        try {
            std::regex::flag_type flags = std::regex::ECMAScript;
            if (!options.caseSensitive) flags |= std::regex::icase;
            std::regex regex(options.pattern, flags);
            return searchWithRegex(regex, options);
        } catch (const std::regex_error&) {
            return {};
        }
    }
    return searchWithSubstring(options.pattern, options);
}

std::vector<SearchResult> SearchEngine::searchTextInFiles(
    const std::vector<std::string>& files, const SearchOptions& options) {
    return {};
}

std::vector<SearchResult> SearchEngine::searchSymbols(const SymbolSearchOptions& options) {
    std::vector<SearchResult> results;
    for (const auto& [path, entry] : index_) {
        for (const auto& sym : entry.symbols) {
            if (results.size() >= options.maxResults) break;
            if (options.fuzzy) {
                // Simple fuzzy match
                if (sym.find(options.query) != std::string::npos) {
                    SearchResult r;
                    r.type = ResultType::Symbol;
                    r.filePath = path;
                    r.matchText = sym;
                    results.push_back(r);
                }
            }
        }
    }
    return results;
}

std::vector<SearchResult> SearchEngine::quickOpen(const std::string& query, size_t maxResults) {
    std::vector<SearchResult> results;
    for (const auto& [path, entry] : index_) {
        if (results.size() >= maxResults) break;
        if (matchesPattern(entry.fileName, query)) {
            SearchResult r;
            r.type = ResultType::File;
            r.filePath = path;
            r.fileName = entry.fileName;
            results.push_back(r);
        }
    }
    return results;
}

void SearchEngine::searchTextAsync(const SearchOptions& options,
                                    SearchCallback resultCallback,
                                    ProgressCallback progressCallback) {
}

void SearchEngine::cancelSearch() {
    cancelRequested_ = true;
}

void SearchEngine::addIgnorePattern(const std::string& pattern) {
    ignorePatterns_.push_back(pattern);
}

void SearchEngine::removeIgnorePattern(const std::string& pattern) {
    ignorePatterns_.erase(std::remove(ignorePatterns_.begin(), ignorePatterns_.end(), pattern),
                          ignorePatterns_.end());
}

void SearchEngine::indexFile(const std::string& path) {
    FileIndexEntry entry;
    entry.path = path;
    entry.fileName = path.substr(path.rfind('/') + 1);
    auto dot = entry.fileName.rfind('.');
    entry.extension = (dot != std::string::npos) ? entry.fileName.substr(dot) : "";
    try {
        entry.size = std::filesystem::file_size(path);
        auto ft = std::filesystem::last_write_time(path);
        entry.modifiedTime = ft.time_since_epoch().count();
    } catch (...) {}
    index_[path] = entry;
}

void SearchEngine::extractSymbols(const std::string& path, FileIndexEntry& entry) {
}

bool SearchEngine::shouldInclude(const std::string& path) const {
    for (const auto& pattern : ignorePatterns_) {
        if (path.find(pattern) != std::string::npos) return false;
    }
    try {
        if (std::filesystem::file_size(path) > maxFileSize_) return false;
    } catch (...) {}
    return true;
}

bool SearchEngine::matchesPattern(const std::string& text, const std::string& pattern) const {
    if (pattern.empty()) return true;
    std::string lowerText = text;
    std::string lowerPattern = pattern;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
    std::transform(lowerPattern.begin(), lowerPattern.end(), lowerPattern.begin(), ::tolower);
    return lowerText.find(lowerPattern) != std::string::npos;
}

std::vector<SearchResult> SearchEngine::searchWithRegex(const std::regex& regex,
                                                         const SearchOptions& options) {
    std::vector<SearchResult> results;
    for (const auto& [path, entry] : index_) {
        if (results.size() >= maxResults_) break;
        std::ifstream file(path);
        std::string line;
        size_t lineNum = 0;
        while (std::getline(file, line)) {
            lineNum++;
            std::smatch match;
            if (std::regex_search(line, match, regex)) {
                SearchResult r;
                r.type = ResultType::TextMatch;
                r.filePath = path;
                r.fileName = entry.fileName;
                r.lineNumber = lineNum;
                r.column = match.position();
                r.lineText = line;
                r.matchText = match.str();
                r.matchStart = match.position();
                r.matchLength = match.length();
                results.push_back(r);
            }
        }
    }
    return results;
}

std::vector<SearchResult> SearchEngine::searchWithSubstring(const std::string& pattern,
                                                             const SearchOptions& options) {
    std::vector<SearchResult> results;
    for (const auto& [path, entry] : index_) {
        if (results.size() >= maxResults_) break;
        std::ifstream file(path);
        std::string line;
        size_t lineNum = 0;
        while (std::getline(file, line)) {
            lineNum++;
            size_t pos = line.find(pattern);
            if (pos != std::string::npos) {
                SearchResult r;
                r.type = ResultType::TextMatch;
                r.filePath = path;
                r.fileName = entry.fileName;
                r.lineNumber = lineNum;
                r.column = pos;
                r.lineText = line;
                r.matchText = pattern;
                r.matchStart = pos;
                r.matchLength = pattern.length();
                results.push_back(r);
            }
        }
    }
    return results;
}

void SearchEngine::scoreResults(std::vector<SearchResult>& results, const std::string& query) {
}

// SearchHistory
SearchHistory& SearchHistory::getInstance() {
    static SearchHistory instance;
    return instance;
}

void SearchHistory::addSearch(const std::string& pattern) {
    history_.erase(std::remove_if(history_.begin(), history_.end(),
        [&](const auto& p) { return p.first == pattern; }), history_.end());
    history_.insert(history_.begin(), {pattern, 0});
}

std::vector<std::string> SearchHistory::getHistory(size_t maxCount) const {
    std::vector<std::string> result;
    for (size_t i = 0; i < std::min(maxCount, history_.size()); i++) {
        result.push_back(history_[i].first);
    }
    return result;
}

void SearchHistory::clear() {
    history_.clear();
}

void SearchHistory::remove(const std::string& pattern) {
    history_.erase(std::remove_if(history_.begin(), history_.end(),
        [&](const auto& p) { return p.first == pattern; }), history_.end());
}

} // namespace NexusForge::Workspace