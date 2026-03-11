// workspace/search_engine.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <regex>

namespace NexusForge::Workspace {

// Search result types
enum class ResultType {
    File,
    TextMatch,
    Symbol
};

// Search result
struct SearchResult {
    ResultType type;
    std::string filePath;
    std::string fileName;
    size_t lineNumber = 0;
    size_t column = 0;
    std::string lineText;
    std::string matchText;
    size_t matchStart = 0;
    size_t matchLength = 0;
    std::vector<std::string> contextBefore;
    std::vector<std::string> contextAfter;
    double relevance = 1.0;
};

// Search options
struct SearchOptions {
    std::string pattern;
    bool caseSensitive = false;
    bool wholeWord = false;
    bool useRegex = false;
    bool useGlob = false;

    std::vector<std::string> includePatterns;
    std::vector<std::string> excludePatterns;

    std::vector<std::string> fileTypes;
    size_t maxResults = 20000;
    int contextLines = 0;

    bool followSymlinks = false;
    bool respectGitIgnore = true;
    bool respectIgnoreFiles = true;
};

// Symbol search options
struct SymbolSearchOptions {
    std::string query;
    std::vector<std::string> kinds;  // function, class, variable, etc.
    std::vector<std::string> files;
    bool caseSensitive = false;
    bool fuzzy = true;
    size_t maxResults = 100;
};

// File index entry
struct FileIndexEntry {
    std::string path;
    std::string fileName;
    std::string extension;
    size_t size = 0;
    uint64_t modifiedTime = 0;
    std::vector<std::string> symbols;
    std::string content;  // Cached for small files
};

// Search engine
class SearchEngine {
public:
    SearchEngine();
    ~SearchEngine();

    // Index management
    void setRootDirectory(const std::string& path);
    const std::string& getRootDirectory() const { return rootDir_; }

    void buildIndex();
    void updateIndex(const std::string& filePath);
    void removeFromIndex(const std::string& filePath);
    void clearIndex();

    bool isIndexing() const { return indexing_; }
    float getIndexProgress() const { return indexProgress_; }

    // File search
    std::vector<SearchResult> searchFiles(const std::string& pattern,
                                           const SearchOptions& options = {});

    // Text search
    std::vector<SearchResult> searchText(const SearchOptions& options);
    std::vector<SearchResult> searchTextInFiles(const std::vector<std::string>& files,
                                                 const SearchOptions& options);

    // Symbol search
    std::vector<SearchResult> searchSymbols(const SymbolSearchOptions& options);

    // Quick open (fuzzy file search)
    std::vector<SearchResult> quickOpen(const std::string& query, size_t maxResults = 20);

    // Search with callback (for progress)
    using SearchCallback = std::function<void(const SearchResult&)>;
    using ProgressCallback = std::function<void(float, size_t)>;

    void searchTextAsync(const SearchOptions& options,
                         SearchCallback resultCallback,
                         ProgressCallback progressCallback);

    void cancelSearch();

    // Configuration
    void setMaxFileSize(size_t bytes) { maxFileSize_ = bytes; }
    void setMaxResults(size_t count) { maxResults_ = count; }

    // Ignore patterns
    void addIgnorePattern(const std::string& pattern);
    void removeIgnorePattern(const std::string& pattern);
    const std::vector<std::string>& getIgnorePatterns() const { return ignorePatterns_; }

    // Statistics
    size_t getIndexedFileCount() const { return index_.size(); }
    size_t getTotalIndexedSize() const { return totalIndexedSize_; }

private:
    std::string rootDir_;
    std::unordered_map<std::string, FileIndexEntry> index_;
    std::vector<std::string> ignorePatterns_;

    size_t maxFileSize_ = 10 * 1024 * 1024;  // 10MB
    size_t maxResults_ = 20000;

    bool indexing_ = false;
    float indexProgress_ = 0;
    bool cancelRequested_ = false;

    void indexFile(const std::string& path);
    void extractSymbols(const std::string& path, FileIndexEntry& entry);
    bool shouldInclude(const std::string& path) const;
    bool matchesPattern(const std::string& text, const std::string& pattern) const;

    std::vector<SearchResult> searchWithRegex(const std::regex& regex,
                                               const SearchOptions& options);
    std::vector<SearchResult> searchWithSubstring(const std::string& pattern,
                                                   const SearchOptions& options);

    void scoreResults(std::vector<SearchResult>& results, const std::string& query);
};

// Search history
class SearchHistory {
public:
    static SearchHistory& getInstance();

    void addSearch(const std::string& pattern);
    std::vector<std::string> getHistory(size_t maxCount = 20) const;
    void clear();
    void remove(const std::string& pattern);

private:
    SearchHistory() = default;
    std::vector<std::pair<std::string, uint64_t>> history_;
};

} // namespace NexusForge::Workspace
