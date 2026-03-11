// editor/features/bracket_matching.hpp
#pragma once

#include <string>
#include <vector>
#include <utility>

namespace NexusForge::Editor {

// Bracket pair
struct BracketPair {
    char open;
    char close;
    size_t openLine;
    size_t openColumn;
    size_t closeLine;
    size_t closeColumn;
    bool matched = true;
    bool isOverlapping = false;
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
