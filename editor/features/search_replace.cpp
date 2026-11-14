// editor/features/search_replace.cpp
// Search and Replace Implementation

#include "search_replace.hpp"
#include "../buffer/text_buffer.hpp"
#include <algorithm>
#include <regex>

namespace NexusForge::Editor {

SearchEngine::SearchEngine(TextBuffer* buffer) : buffer_(buffer) {
}

SearchEngine::~SearchEngine() = default;

std::vector<SearchResult> SearchEngine::find(const std::string& pattern, const SearchOptions& options) {
    std::vector<SearchResult> results;
    
    if (!buffer_ || pattern.empty()) {
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
    std::string searchPattern = pattern;
    std::string lineText;
    
    if (!options.caseSensitive) {
        std::transform(searchPattern.begin(), searchPattern.end(), 
                      searchPattern.begin(), ::tolower);
    }
    
    size_t startLine = options.searchInSelection ? options.selectionStartLine : 0;
    size_t endLine = options.searchInSelection ? options.selectionEndLine : buffer_->getLineCount() - 1;
    
    for (size_t line = startLine; line <= endLine && line < buffer_->getLineCount(); ++line) {
        lineText = buffer_->getLine(line);
        std::string searchLine = lineText;
        
        if (!options.caseSensitive) {
            std::transform(searchLine.begin(), searchLine.end(), 
                          searchLine.begin(), ::tolower);
        }
        
        size_t pos = 0;
        while ((pos = searchLine.find(searchPattern, pos)) != std::string::npos) {
            SearchResult result;
            result.line = line;
            result.startCol = pos;
            result.endCol = pos + pattern.length();
            result.matchedText = lineText.substr(pos, pattern.length());
            results.push_back(result);
            
            if (pattern.empty()) break;  // Prevent infinite loop
            pos += 1;  // Move by 1 to find overlapping matches
        }
    }
}

void SearchEngine::findRegex(const std::string& pattern, const SearchOptions& options,
                              std::vector<SearchResult>& results) {
    try {
        std::regex_constants::syntax_option_type flags = std::regex::ECMAScript;
        if (!options.caseSensitive) {
            flags |= std::regex::icase;
        }
        
        std::regex re(pattern, flags);
        
        size_t startLine = options.searchInSelection ? options.selectionStartLine : 0;
        size_t endLine = options.searchInSelection ? options.selectionEndLine : buffer_->getLineCount() - 1;
        
        for (size_t line = startLine; line <= endLine && line < buffer_->getLineCount(); ++line) {
            std::string lineText = buffer_->getLine(line);
            
            auto begin = std::sregex_iterator(lineText.begin(), lineText.end(), re);
            auto end = std::sregex_iterator();
            
            for (auto it = begin; it != end; ++it) {
                SearchResult result;
                result.line = line;
                result.startCol = static_cast<size_t>(it->position());
                result.endCol = result.startCol + it->length();
                result.matchedText = it->str();
                results.push_back(result);
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
    
    buffer_->removeText(match.line, match.startCol, match.line, match.endCol);
    buffer_->insertText(match.line, match.startCol, replaceText);
    
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
    
    auto matches = find(pattern, searchOptions);
    
    // Process in reverse order to maintain positions
    std::sort(matches.begin(), matches.end(),
        [](const SearchResult& a, const SearchResult& b) {
            if (a.line != b.line) return a.line > b.line;
            return a.startCol > b.startCol;
        });
    
    for (const auto& match : matches) {
        ReplaceResult replaceResult = replace(match, replacement, useRegex);
        if (replaceResult.success) {
            result.count++;
        }
    }
    
    return result;
}

std::vector<SearchResult> SearchEngine::findAllInFile(const std::string& filePath,
                                                       const std::string& pattern,
                                                       const SearchOptions& options) {
    // In production, would search across multiple files
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

} // namespace NexusForge::Editor
