// editor/features/bracket_matching.hpp
#pragma once

#include <string>
#include <vector>
#include <utility>
#include <unordered_map>

namespace NexusForge::Editor {

class TextBuffer;

// Bracket match result
struct BracketMatch {
    bool found = false;
    size_t openLine = 0;
    size_t openCol = 0;
    size_t closeLine = 0;
    size_t closeCol = 0;
};

// Bracket pair
struct BracketPair {
    char open = '\0';
    char close = '\0';
    char type = '\0';
    size_t openLine = 0;
    size_t openColumn = 0;
    size_t closeLine = 0;
    size_t closeColumn = 0;
    bool matched = true;
    bool isOverlapping = false;
};

// Buffer-backed bracket matcher
class BracketMatcher {
public:
    explicit BracketMatcher(TextBuffer* buffer = nullptr);
    ~BracketMatcher();

    void setBuffer(TextBuffer* buffer) { buffer_ = buffer; }
    BracketMatch findMatch(size_t line, size_t col);
    std::vector<BracketPair> findAllMatchingBrackets(size_t line);

    bool isOpeningBracket(char ch) const;
    bool isClosingBracket(char ch) const;
    char getMatchingBracket(char ch) const;

    void autoInsertClosingBracket(TextBuffer* buffer, size_t line, size_t col);

private:
    TextBuffer* buffer_ = nullptr;
    std::unordered_map<char, char> bracketPairs_;
    std::string openingBrackets_;
    std::string closingBrackets_;

    BracketMatch findForwardMatch(size_t startLine, size_t startCol, char openChar, char closeChar);
    BracketMatch findBackwardMatch(size_t startLine, size_t startCol, char openChar, char closeChar);
    size_t skipString(const std::string& lineText, size_t startPos);
};

// Bracket matching configuration
struct BracketMatchingConfig {
    std::vector<std::pair<char, char>> bracketPairs = {
        {'(', ')'}, {'{', '}'}, {'[', ']'}, {'<', '>'}
    };
    std::vector<std::pair<std::string, std::string>> tagPairs = {
        {"<div>", "</div>"}, {"<span>", "</span>"}
    };
    bool highlightActive = true;
    bool highlightSurrounding = true;
    bool showLines = false;
    int maxDistance = 1000;  // Max lines to search
};

// Bracket matching engine
class BracketMatchingEngine {
public:
    BracketMatchingEngine();
    ~BracketMatchingEngine();

    // Configuration
    void setConfig(const BracketMatchingConfig& config);
    const BracketMatchingConfig& getConfig() const { return config_; }

    // Find matching bracket
    BracketPair findMatchingBracket(const std::string& text,
                                     size_t line, size_t column);
    
    // Find all brackets in range
    std::vector<BracketPair> findAllBrackets(const std::string& text,
                                              size_t startLine, size_t endLine);

    // Check if cursor is on/near a bracket
    bool isOnBracket(const std::string& line, size_t column) const;
    bool isNearBracket(const std::string& text, size_t line, size_t column,
                       size_t* bracketLine, size_t* bracketColumn) const;

    // Get surrounding brackets
    BracketPair getSurroundingBrackets(const std::string& text,
                                        size_t line, size_t column);

    // Select bracket contents
    std::pair<size_t, size_t> getBracketContents(const std::string& text,
                                                  size_t line, size_t column);

private:
    BracketMatchingConfig config_;
    
    int findMatching(const std::string& text, size_t startPos,
                     char open, char close, bool forward);
    bool isQuoteChar(char c) const;
    bool isEscapeChar(const std::string& text, size_t pos) const;
};

} // namespace NexusForge::Editor
