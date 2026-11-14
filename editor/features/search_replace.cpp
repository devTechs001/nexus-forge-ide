// editor/features/search_replace.cpp
// Search and Replace Implementation

#include "search_replace.hpp"
#include "../buffer/text_buffer.hpp"
#include <algorithm>
#include <cctype>
#include <regex>

namespace NexusForge::Editor {

namespace {
bool isWordCharSR(char c) {
    unsigned char uc = static_cast<unsigned char>(c);
    return std::isalnum(uc) || uc == '_';
}
}

SearchEngine::SearchEngine(TextBuffer* buffer) : buffer_(buffer) {
}

SearchEngine::~SearchEngine() = default;

std::vector<SearchResult> SearchEngine::find(const std::string& pattern, const SearchOptions& options) {
    std::vector<SearchResult> results;

    if (!buffer_ || pattern.empty() || buffer_->getLineCount() == 0) {
        return results;
    }

    if (options.useRegex) {
        findRegex(pattern, options, results);
    } else {
        findPlain(pattern, options, results);
    }

    return results;
}

void SearchEngine::findPlain(const std::string& pattern, const SearchOptions& options,
                              std::vector<SearchResult>& results) {
    if (!buffer_ || buffer_->getLineCount() == 0 || pattern.empty()) return;

    std::string searchPattern = pattern;
    if (!options.caseSensitive) {
        std::transform(searchPattern.begin(), searchPattern.end(),
                      searchPattern.begin(),
                      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    }

    size_t lastLine = buffer_->getLineCount() - 1;
    size_t startLine = options.searchInSelection ? std::min(options.selectionStartLine, lastLine) : 0;
    size_t endLine = options.searchInSelection ? std::min(options.selectionEndLine, lastLine) : lastLine;

    for (size_t line = startLine; line <= endLine && line < buffer_->getLineCount(); ++line) {
        std::string lineText = buffer_->getLine(line);
        std::string searchLine = lineText;

        if (!options.caseSensitive) {
            std::transform(searchLine.begin(), searchLine.end(),
                          searchLine.begin(),
                          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        }

        size_t pos = 0;
        while ((pos = searchLine.find(searchPattern, pos)) != std::string::npos) {
            bool ok = true;
            if (options.wholeWord) {
                if (pos > 0 && isWordCharSR(lineText[pos - 1])) ok = false;
                if (pos + pattern.length() < lineText.length() && isWordCharSR(lineText[pos + pattern.length()])) ok = false;
            }
            if (ok) {
                SearchResult result;
                result.found = true;
                result.line = line;
                result.startCol = pos;
                result.endCol = pos + pattern.length();
                result.matchedText = lineText.substr(pos, pattern.length());
                results.push_back(result);
                if (results.size() >= options.maxResults) return;
            }
            pos += pattern.length();
        }
    }
}

void SearchEngine::findRegex(const std::string& pattern, const SearchOptions& options,
                              std::vector<SearchResult>& results) {
    if (!buffer_ || buffer_->getLineCount() == 0 || pattern.empty()) return;
    try {
        std::regex_constants::syntax_option_type flags = std::regex::ECMAScript;
        if (!options.caseSensitive) {
            flags |= std::regex::icase;
        }

        std::regex re(pattern, flags);

        size_t lastLine = buffer_->getLineCount() - 1;
        size_t startLine = options.searchInSelection ? std::min(options.selectionStartLine, lastLine) : 0;
        size_t endLine = options.searchInSelection ? std::min(options.selectionEndLine, lastLine) : lastLine;

        for (size_t line = startLine; line <= endLine && line < buffer_->getLineCount(); ++line) {
            std::string lineText = buffer_->getLine(line);

            auto begin = std::sregex_iterator(lineText.begin(), lineText.end(), re);
            auto end = std::sregex_iterator();

            for (auto it = begin; it != end; ++it) {
                if (it->length() == 0) continue;
                SearchResult result;
                result.found = true;
                result.line = line;
                result.startCol = static_cast<size_t>(it->position());
                result.endCol = result.startCol + static_cast<size_t>(it->length());
                result.matchedText = it->str();
                results.push_back(result);
                if (results.size() >= options.maxResults) return;
            }
        }
    } catch (const std::regex_error&) {
        // Invalid regex - return empty results
    }
}

SearchResult SearchEngine::findNext(const std::string& pattern, const SearchOptions& options,
                                     size_t currentLine, size_t currentCol) {
    SearchResult result;
    result.found = false;
    
    auto allResults = find(pattern, options);
    
    if (allResults.empty()) {
        return result;
    }
    
    // Find first result after current position
    for (const auto& r : allResults) {
        if (r.line > currentLine || (r.line == currentLine && r.startCol > currentCol)) {
            return r;
        }
    }
    
    // Wrap around - return first result
    if (options.wrapAround && !allResults.empty()) {
        return allResults[0];
    }
    
    return result;
}

SearchResult SearchEngine::findPrevious(const std::string& pattern, const SearchOptions& options,
                                         size_t currentLine, size_t currentCol) {
    SearchResult result;
    result.found = false;
    
    auto allResults = find(pattern, options);
    
    if (allResults.empty()) {
        return result;
    }
    
    // Find last result before current position
    for (auto it = allResults.rbegin(); it != allResults.rend(); ++it) {
        if (it->line < currentLine || (it->line == currentLine && it->startCol < currentCol)) {
            return *it;
        }
    }
    
    // Wrap around - return last result
    if (options.wrapAround && !allResults.empty()) {
        return allResults.back();
    }
    
    return result;
}

ReplaceResult SearchEngine::replace(const SearchResult& match, const std::string& replacement,
                                     bool useRegex) {
    ReplaceResult result;
    result.success = false;
    
    if (!buffer_ || !match.found) {
        return result;
    }
    
    std::string replaceText = replacement;
    
    if (useRegex) {
        // Handle regex replacement patterns like $1, $2 for capture groups
        // Simplified implementation
    }
    
    buffer_->removeRange(match.line, match.startCol, match.line, match.endCol);
    buffer_->insert(match.line, match.startCol, replaceText);

    result.newLine = match.line;
    result.newCol = match.startCol + replaceText.length();
    result.success = true;

    return result;
}

ReplaceAllResult SearchEngine::replaceAll(const std::string& pattern, const std::string& replacement,
                                           const SearchOptions& searchOptions, bool useRegex) {
    ReplaceAllResult result;
    result.count = 0;

    if (!buffer_ || pattern.empty()) {
        return result;
    }

    SearchOptions opts = searchOptions;
    if (useRegex) opts.useRegex = true;
    auto matches = find(pattern, opts);

    // Filter out any overlapping matches before replacing from bottom-right to top-left
    std::vector<SearchResult> nonOverlapping;
    for (const auto& m : matches) {
        if (!nonOverlapping.empty() &&
            nonOverlapping.back().line == m.line &&
            m.startCol < nonOverlapping.back().endCol) {
            continue;
        }
        nonOverlapping.push_back(m);
    }

    // Process in reverse order to maintain positions
    std::sort(nonOverlapping.begin(), nonOverlapping.end(),
        [](const SearchResult& a, const SearchResult& b) {
            if (a.line != b.line) return a.line > b.line;
            return a.startCol > b.startCol;
        });

    for (const auto& match : nonOverlapping) {
        ReplaceResult replaceResult = replace(match, replacement, opts.useRegex);
        if (replaceResult.success) {
            result.count++;
        }
    }

    return result;
}

std::vector<SearchResult> SearchEngine::findAllInFile(const std::string& filePath,
                                                       const std::string& pattern,
                                                       const SearchOptions& options) {
    (void)filePath;
    return find(pattern, options);
}

void SearchEngine::setBuffer(TextBuffer* buffer) {
    buffer_ = buffer;
}

bool SearchEngine::isValidRegex(const std::string& pattern) {
    try {
        std::regex re(pattern);
        return true;
    } catch (...) {
        return false;
    }
}

// SearchReplaceEngine implementation
SearchReplaceEngine::SearchReplaceEngine() = default;
SearchReplaceEngine::~SearchReplaceEngine() = default;

std::vector<SearchMatch> SearchReplaceEngine::search(const std::string& text,
                                                      const SearchOptions& options) {
    TextBuffer buf;
    buf.setText(text);
    SearchEngine engine(&buf);
    auto found = engine.find(options.pattern, options);
    std::vector<SearchMatch> matches;
    for (const auto& r : found) {
        SearchMatch m;
        m.line = r.line;
        m.column = r.startCol;
        m.matchedText = r.matchedText;
        m.lineText = buf.getLine(r.line);
        // Compute byte offsets
        size_t offset = 0;
        for (size_t i = 0; i < r.line; ++i) offset += buf.getLineLength(i) + 1;
        m.startOffset = offset + r.startCol;
        m.endOffset = offset + r.endCol;
        matches.push_back(std::move(m));
    }
    return matches;
}

std::vector<SearchMatch> SearchReplaceEngine::searchInLines(const std::vector<std::string>& lines,
                                                             const SearchOptions& options) {
    std::string combined;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) combined += '\n';
        combined += lines[i];
    }
    return search(combined, options);
}

std::optional<SearchMatch> SearchReplaceEngine::findNext(const std::string& text,
                                                          size_t fromPosition,
                                                          const SearchOptions& options) {
    auto matches = search(text, options);
    for (const auto& m : matches) {
        if (m.startOffset >= fromPosition) return m;
    }
    if (options.wrapAround && !matches.empty()) return matches.front();
    return std::nullopt;
}

std::optional<SearchMatch> SearchReplaceEngine::findPrevious(const std::string& text,
                                                              size_t fromPosition,
                                                              const SearchOptions& options) {
    auto matches = search(text, options);
    for (auto it = matches.rbegin(); it != matches.rend(); ++it) {
        if (it->startOffset < fromPosition) return *it;
    }
    if (options.wrapAround && !matches.empty()) return matches.back();
    return std::nullopt;
}

std::string SearchReplaceEngine::replace(const std::string& text,
                                          const SearchOptions& searchOptions,
                                          const ReplaceOptions& replaceOptions) {
    std::string result;
    replaceAll(text, result, searchOptions, replaceOptions);
    return result;
}

size_t SearchReplaceEngine::replaceAll(const std::string& text,
                                        std::string& result,
                                        const SearchOptions& searchOptions,
                                        const ReplaceOptions& replaceOptions) {
    TextBuffer buf;
    buf.setText(text);
    SearchEngine engine(&buf);
    auto r = engine.replaceAll(searchOptions.pattern, replaceOptions.replacement,
                               searchOptions, searchOptions.useRegex);
    result = buf.getText();
    return r.count;
}

std::vector<std::pair<size_t, size_t>> SearchReplaceEngine::getHighlightRanges(
    const std::string& text, const SearchOptions& options) {
    std::vector<std::pair<size_t, size_t>> ranges;
    for (const auto& m : search(text, options)) {
        ranges.emplace_back(m.startOffset, m.endOffset);
    }
    return ranges;
}

size_t SearchReplaceEngine::countMatches(const std::string& text, const SearchOptions& options) {
    return search(text, options).size();
}

std::regex SearchReplaceEngine::buildRegex(const SearchOptions& options) {
    std::regex_constants::syntax_option_type flags = std::regex::ECMAScript;
    if (!options.caseSensitive) flags |= std::regex::icase;
    return std::regex(options.pattern, flags);
}

std::string SearchReplaceEngine::applyReplacement(const std::string& match,
                                                   const std::vector<std::string>& captures,
                                                   const ReplaceOptions& options) {
    (void)captures;
    if (options.preserveCase) return preserveCase(match, options.replacement);
    return options.replacement;
}

std::string SearchReplaceEngine::preserveCase(const std::string& source,
                                               const std::string& target) {
    if (source.empty() || target.empty()) return target;
    bool allUpper = std::all_of(source.begin(), source.end(),
        [](unsigned char c) { return !std::isalpha(c) || std::isupper(c); });
    if (allUpper) {
        std::string res = target;
        std::transform(res.begin(), res.end(), res.begin(),
            [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return res;
    }
    if (std::isupper(static_cast<unsigned char>(source[0]))) {
        std::string res = target;
        res[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(res[0])));
        return res;
    }
    return target;
}

bool SearchReplaceEngine::matchesWholeWord(const std::string& text, size_t start, size_t end) {
    if (start > 0 && isWordCharSR(text[start - 1])) return false;
    if (end < text.size() && isWordCharSR(text[end])) return false;
    return true;
}

// FindReplaceController implementation
FindReplaceController::FindReplaceController() = default;
FindReplaceController::~FindReplaceController() = default;

void FindReplaceController::findNext() {
    if (matches_.empty()) return;
    currentMatchIndex_ = (currentMatchIndex_ + 1) % matches_.size();
    notifyMatchChanged();
}

void FindReplaceController::findPrevious() {
    if (matches_.empty()) return;
    currentMatchIndex_ = (currentMatchIndex_ + matches_.size() - 1) % matches_.size();
    notifyMatchChanged();
}

void FindReplaceController::selectAll() {}
void FindReplaceController::replace() {}
void FindReplaceController::replaceAll() {}

void FindReplaceController::setMatchChangedCallback(MatchChangedCallback callback) {
    matchChangedCallback_ = std::move(callback);
}

void FindReplaceController::updateMatches(const std::string& text) {
    SearchReplaceEngine engine;
    options_.pattern = searchText_;
    matches_ = engine.search(text, options_);
    currentMatchIndex_ = 0;
    notifyMatchChanged();
}

void FindReplaceController::notifyMatchChanged() {
    if (matchChangedCallback_) {
        matchChangedCallback_(currentMatchIndex_, matches_.size());
    }
}

} // namespace NexusForge::Editor
