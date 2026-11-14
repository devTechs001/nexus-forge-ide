// workspace/search_engine.cpp
// Search Engine Implementation

#include "search_engine.hpp"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <iostream>

namespace NexusForge::Workspace {

SearchEngine::SearchEngine() = default;
SearchEngine::~SearchEngine() = default;

void SearchEngine::setRootDirectory(const std::string& path) {
    rootDir_ = path;
}

void SearchEngine::buildIndex() {
    std::error_code ec;
    if (rootDir_.empty() || !std::filesystem::exists(rootDir_, ec)) return;
    indexing_ = true;
    cancelRequested_ = false;
    indexProgress_ = 0.0f;
    index_.clear();
    totalIndexedSize_ = 0;

    std::vector<std::string> filesToIndex;
    auto opts = std::filesystem::directory_options::skip_permission_denied;
    for (auto it = std::filesystem::recursive_directory_iterator(rootDir_, opts, ec);
         !ec && it != std::filesystem::recursive_directory_iterator();
         it.increment(ec)) {
        if (cancelRequested_) break;
        std::error_code entryEc;
        if (it->is_regular_file(entryEc) && !entryEc) {
            filesToIndex.push_back(it->path().string());
        }
    }

    size_t total = filesToIndex.size();
    size_t processed = 0;
    for (const auto& path : filesToIndex) {
        if (cancelRequested_) break;
        if (shouldInclude(path)) {
            indexFile(path);
        }
        processed++;
        indexProgress_ = (total > 0) ? static_cast<float>(processed) / static_cast<float>(total) : 1.0f;
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
    auto it = index_.find(filePath);
    if (it != index_.end()) {
        if (totalIndexedSize_ >= it->second.size) {
            totalIndexedSize_ -= it->second.size;
        } else {
            totalIndexedSize_ = 0;
        }
        index_.erase(it);
    }
}

void SearchEngine::clearIndex() {
    index_.clear();
    totalIndexedSize_ = 0;
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
    auto toLower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    };
    std::string needle = options.caseSensitive ? options.query : toLower(options.query);
    for (const auto& [path, entry] : index_) {
        for (const auto& sym : entry.symbols) {
            if (results.size() >= options.maxResults) break;
            std::string hay = options.caseSensitive ? sym : toLower(sym);
            bool matched = false;
            if (needle.empty()) {
                matched = true;
            } else if (options.fuzzy) {
                // Subsequence fuzzy match
                size_t qi = 0;
                for (char ch : hay) {
                    if (qi < needle.size() && ch == needle[qi]) ++qi;
                }
                matched = (qi == needle.size());
            } else {
                matched = (hay.find(needle) != std::string::npos);
            }
            if (matched) {
                SearchResult r;
                r.type = ResultType::Symbol;
                r.filePath = path;
                r.fileName = entry.fileName;
                r.matchText = sym;
                results.push_back(r);
            }
        }
        if (results.size() >= options.maxResults) break;
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
    size_t slashPos = path.find_last_of("/\\");
    entry.fileName = (slashPos != std::string::npos) ? path.substr(slashPos + 1) : path;
    auto dot = entry.fileName.rfind('.');
    entry.extension = (dot != std::string::npos) ? entry.fileName.substr(dot) : "";
    try {
        entry.size = std::filesystem::file_size(path);
        auto ft = std::filesystem::last_write_time(path);
        entry.modifiedTime = static_cast<uint64_t>(ft.time_since_epoch().count());
    } catch (...) {}
    auto prev = index_.find(path);
    if (prev != index_.end() && totalIndexedSize_ >= prev->second.size) {
        totalIndexedSize_ -= prev->second.size;
    }
    totalIndexedSize_ += entry.size;
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
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    std::transform(lowerPattern.begin(), lowerPattern.end(), lowerPattern.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lowerText.find(lowerPattern) != std::string::npos;
}

std::vector<SearchResult> SearchEngine::searchWithRegex(const std::regex& regex,
                                                         const SearchOptions& options) {
    std::vector<SearchResult> results;
    size_t limit = std::min(maxResults_, options.maxResults);
    for (const auto& [path, entry] : index_) {
        if (results.size() >= limit) break;
        std::ifstream file(path);
        std::string line;
        size_t lineNum = 0;
        while (std::getline(file, line)) {
            if (results.size() >= limit) break;
            lineNum++;
            std::smatch match;
            if (std::regex_search(line, match, regex)) {
                SearchResult r;
                r.type = ResultType::TextMatch;
                r.filePath = path;
                r.fileName = entry.fileName;
                r.lineNumber = lineNum;
                r.column = static_cast<size_t>(match.position());
                r.lineText = line;
                r.matchText = match.str();
                r.matchStart = static_cast<size_t>(match.position());
                r.matchLength = static_cast<size_t>(match.length());
                results.push_back(r);
            }
        }
    }
    return results;
}

std::vector<SearchResult> SearchEngine::searchWithSubstring(const std::string& pattern,
                                                             const SearchOptions& options) {
    std::vector<SearchResult> results;
    if (pattern.empty()) return results;
    size_t limit = std::min(maxResults_, options.maxResults);
    auto toLower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    };
    auto isWordChar = [](char c) {
        unsigned char uc = static_cast<unsigned char>(c);
        return std::isalnum(uc) || uc == '_';
    };
    std::string needle = options.caseSensitive ? pattern : toLower(pattern);

    for (const auto& [path, entry] : index_) {
        if (results.size() >= limit) break;
        std::ifstream file(path);
        std::string line;
        size_t lineNum = 0;
        while (std::getline(file, line)) {
            if (results.size() >= limit) break;
            lineNum++;
            std::string hay = options.caseSensitive ? line : toLower(line);
            size_t searchPos = 0;
            while (searchPos <= hay.size()) {
                size_t pos = hay.find(needle, searchPos);
                if (pos == std::string::npos) break;
                bool ok = true;
                if (options.wholeWord) {
                    if (pos > 0 && isWordChar(line[pos - 1])) ok = false;
                    if (pos + needle.size() < line.size() && isWordChar(line[pos + needle.size()])) ok = false;
                }
                if (ok) {
                    SearchResult r;
                    r.type = ResultType::TextMatch;
                    r.filePath = path;
                    r.fileName = entry.fileName;
                    r.lineNumber = lineNum;
                    r.column = pos;
                    r.lineText = line;
                    r.matchText = line.substr(pos, needle.size());
                    r.matchStart = pos;
                    r.matchLength = needle.size();
                    results.push_back(r);
                    if (results.size() >= limit) break;
                }
                searchPos = pos + needle.size();
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