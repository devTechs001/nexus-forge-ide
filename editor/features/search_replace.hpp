// editor/features/search_replace.hpp
#pragma once

#include <string>
#include <vector>
#include <regex>
#include <functional>
#include <optional>
#include <utility>

namespace NexusForge::Editor {

class TextBuffer;

// Search options
struct SearchOptions {
    std::string pattern;
    bool caseSensitive = false;
    bool wholeWord = false;
    bool useRegex = false;
    bool matchLines = false;
    bool searchInSelection = false;
    bool wrapAround = true;
    size_t selectionStartLine = 0;
    size_t selectionEndLine = 0;
    size_t maxResults = 10000;
};

// Buffer search result
struct SearchResult {
    bool found = false;
    size_t line = 0;
    size_t startCol = 0;
    size_t endCol = 0;
    std::string matchedText;
};

// Replace result
struct ReplaceResult {
    bool success = false;
    size_t newLine = 0;
    size_t newCol = 0;
};

struct ReplaceAllResult {
    size_t count = 0;
};

// Buffer-backed search engine
class SearchEngine {
public:
    explicit SearchEngine(TextBuffer* buffer = nullptr);
    ~SearchEngine();

    void setBuffer(TextBuffer* buffer);
    std::vector<SearchResult> find(const std::string& pattern, const SearchOptions& options = {});
    SearchResult findNext(const std::string& pattern, const SearchOptions& options,
                          size_t currentLine, size_t currentCol);
    SearchResult findPrevious(const std::string& pattern, const SearchOptions& options,
                              size_t currentLine, size_t currentCol);

    ReplaceResult replace(const SearchResult& match, const std::string& replacement,
                          bool useRegex = false);
    ReplaceAllResult replaceAll(const std::string& pattern, const std::string& replacement,
                                const SearchOptions& searchOptions = {}, bool useRegex = false);
    std::vector<SearchResult> findAllInFile(const std::string& filePath,
                                            const std::string& pattern,
                                            const SearchOptions& options = {});
    static bool isValidRegex(const std::string& pattern);

private:
    TextBuffer* buffer_ = nullptr;

    void findPlain(const std::string& pattern, const SearchOptions& options,
                   std::vector<SearchResult>& results);
    void findRegex(const std::string& pattern, const SearchOptions& options,
                   std::vector<SearchResult>& results);
};

// Replace options
struct ReplaceOptions {
    std::string replacement;
    bool preserveCase = false;
    bool useBackreferences = false;
};

// Search result
struct SearchMatch {
    size_t startOffset = 0;
    size_t endOffset = 0;
    size_t line = 0;
    size_t column = 0;
    std::string matchedText;
    std::string lineText;
    std::vector<std::string> captureGroups;
};

// Search engine
class SearchReplaceEngine {
public:
    SearchReplaceEngine();
    ~SearchReplaceEngine();

    // Search
    std::vector<SearchMatch> search(const std::string& text,
                                     const SearchOptions& options);
    
    std::vector<SearchMatch> searchInLines(const std::vector<std::string>& lines,
                                            const SearchOptions& options);

    // Find next/previous
    std::optional<SearchMatch> findNext(const std::string& text,
                                         size_t fromPosition,
                                         const SearchOptions& options);
    std::optional<SearchMatch> findPrevious(const std::string& text,
                                             size_t fromPosition,
                                             const SearchOptions& options);

    // Replace
    std::string replace(const std::string& text,
                        const SearchOptions& searchOptions,
                        const ReplaceOptions& replaceOptions);
    
    size_t replaceAll(const std::string& text,
                      std::string& result,
                      const SearchOptions& searchOptions,
                      const ReplaceOptions& replaceOptions);

    // Highlight
    std::vector<std::pair<size_t, size_t>> getHighlightRanges(
        const std::string& text, const SearchOptions& options);

    // Count
    size_t countMatches(const std::string& text, const SearchOptions& options);

private:
    std::regex buildRegex(const SearchOptions& options);
    std::string applyReplacement(const std::string& match,
                                  const std::vector<std::string>& captures,
                                  const ReplaceOptions& options);
    std::string preserveCase(const std::string& source,
                              const std::string& target);
    bool matchesWholeWord(const std::string& text, size_t start, size_t end);
};

// Find/Replace widget controller
class FindReplaceController {
public:
    FindReplaceController();
    ~FindReplaceController();

    // State
    void setSearchText(const std::string& text) { searchText_ = text; }
    void setReplaceText(const std::string& text) { replaceText_ = text; }
    const std::string& getSearchText() const { return searchText_; }
    const std::string& getReplaceText() const { return replaceText_; }

    // Options
    void setCaseSensitive(bool sensitive) { options_.caseSensitive = sensitive; }
    void setWholeWord(bool whole) { options_.wholeWord = whole; }
    void setRegex(bool useRegex) { options_.useRegex = useRegex; }
    bool isCaseSensitive() const { return options_.caseSensitive; }
    bool isWholeWord() const { return options_.wholeWord; }
    bool isRegex() const { return options_.useRegex; }

    // Navigation
    void findNext();
    void findPrevious();
    void selectAll();

    // Replace
    void replace();
    void replaceAll();

    // Results
    size_t getCurrentMatchIndex() const { return currentMatchIndex_; }
    size_t getTotalMatches() const { return matches_.size(); }
    const std::vector<SearchMatch>& getMatches() const { return matches_; }

    // Events
    using MatchChangedCallback = std::function<void(size_t, size_t)>;
    void setMatchChangedCallback(MatchChangedCallback callback);

private:
    std::string searchText_;
    std::string replaceText_;
    SearchOptions options_;
    std::vector<SearchMatch> matches_;
    size_t currentMatchIndex_ = 0;

    MatchChangedCallback matchChangedCallback_;

    void updateMatches(const std::string& text);
    void notifyMatchChanged();
};

} // namespace NexusForge::Editor
