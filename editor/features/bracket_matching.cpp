// editor/features/bracket_matching.cpp
// Bracket Matching Implementation

#include "bracket_matching.hpp"
#include "../buffer/text_buffer.hpp"
#include <stack>
#include <unordered_map>

namespace NexusForge::Editor {

BracketMatcher::BracketMatcher(TextBuffer* buffer) : buffer_(buffer) {
    bracketPairs_ = {
        {'(', ')'},
        {'{', '}'},
        {'[', ']'},
        {'<', '>'},
        {'\'', '\''},
        {'"', '"'}
    };
    
    openingBrackets_ = "({[<";
    closingBrackets_ = ")}]>";
}

BracketMatcher::~BracketMatcher() = default;

BracketMatch BracketMatcher::findMatch(size_t line, size_t col) {
    BracketMatch result;
    result.found = false;
    
    if (!buffer_ || line >= buffer_->getLineCount()) {
        return result;
    }
    
    std::string lineText = buffer_->getLine(line);
    
    if (col >= lineText.length()) {
        return result;
    }
    
    char ch = lineText[col];
    
    // Check if it's a bracket
    size_t openPos = openingBrackets_.find(ch);
    size_t closePos = closingBrackets_.find(ch);
    
    if (openPos != std::string::npos) {
        // Opening bracket - search forward
        result = findForwardMatch(line, col, ch, bracketPairs_[ch]);
    } else if (closePos != std::string::npos) {
        // Closing bracket - search backward
        char opening = '\0';
        for (const auto& [open, close] : bracketPairs_) {
            if (close == ch) {
                opening = open;
                break;
            }
        }
        if (opening) {
            result = findBackwardMatch(line, col, opening, ch);
        }
    }
    
    return result;
}

BracketMatch BracketMatcher::findForwardMatch(size_t startLine, size_t startCol, 
                                               char openChar, char closeChar) {
    BracketMatch result;
    result.found = false;
    result.openLine = startLine;
    result.openCol = startCol;

    if (!buffer_) return result;

    std::stack<std::pair<size_t, size_t>> bracketStack;
    bracketStack.push({startLine, startCol});

    for (size_t line = startLine; line < buffer_->getLineCount(); ++line) {
        std::string lineText = buffer_->getLine(line);
        size_t start = (line == startLine) ? startCol + 1 : 0;

        for (size_t col = start; col < lineText.length(); ++col) {
            char ch = lineText[col];

            // Skip strings (simplified)
            if (ch == '"' || ch == '\'') {
                col = skipString(lineText, col);
                continue;
            }

            if (ch == openChar) {
                bracketStack.push({line, col});
            } else if (ch == closeChar && !bracketStack.empty()) {
                bracketStack.pop();

                if (bracketStack.empty()) {
                    result.found = true;
                    result.closeLine = line;
                    result.closeCol = col;
                    return result;
                }
            }
        }
    }

    return result;
}

BracketMatch BracketMatcher::findBackwardMatch(size_t startLine, size_t startCol,
                                                char openChar, char closeChar) {
    BracketMatch result;
    result.found = false;
    result.closeLine = startLine;
    result.closeCol = startCol;
    
    if (!buffer_) return result;
    
    std::stack<std::pair<size_t, size_t>> bracketStack;
    bracketStack.push({startLine, startCol});
    
    for (int line = static_cast<int>(startLine); line >= 0; --line) {
        std::string lineText = buffer_->getLine(line);
        int end = (line == static_cast<int>(startLine)) ? static_cast<int>(startCol) - 1 
                                                         : static_cast<int>(lineText.length()) - 1;
        
        for (int col = end; col >= 0; --col) {
            char ch = lineText[col];
            
            // Skip strings and comments (simplified)
            if (ch == '"' || ch == '\'') {
                continue;
            }
            
            if (ch == closeChar) {
                bracketStack.push({static_cast<size_t>(line), static_cast<size_t>(col)});
            } else if (ch == openChar && !bracketStack.empty()) {
                bracketStack.pop();
                
                if (bracketStack.empty()) {
                    result.found = true;
                    result.openLine = static_cast<size_t>(line);
                    result.openCol = static_cast<size_t>(col);
                    return result;
                }
            }
        }
    }
    
    return result;
}

size_t BracketMatcher::skipString(const std::string& lineText, size_t startPos) {
    char quote = lineText[startPos];
    size_t pos = startPos + 1;
    
    while (pos < lineText.length()) {
        if (lineText[pos] == '\\' && pos + 1 < lineText.length()) {
            pos += 2;  // Skip escaped character
        } else if (lineText[pos] == quote) {
            return pos;
        } else {
            pos++;
        }
    }
    
    return pos;
}

std::vector<BracketPair> BracketMatcher::findAllMatchingBrackets(size_t line) {
    std::vector<BracketPair> pairs;
    
    if (!buffer_ || line >= buffer_->getLineCount()) {
        return pairs;
    }
    
    std::string lineText = buffer_->getLine(line);
    
    for (size_t col = 0; col < lineText.length(); ++col) {
        char ch = lineText[col];
        
        if (openingBrackets_.find(ch) != std::string::npos) {
            BracketMatch match = findForwardMatch(line, col, ch, bracketPairs_[ch]);
            
            if (match.found) {
                BracketPair pair;
                pair.open = ch;
                pair.close = bracketPairs_[ch];
                pair.type = ch;
                pair.openLine = line;
                pair.openColumn = col;
                pair.closeLine = match.closeLine;
                pair.closeColumn = match.closeCol;
                pair.matched = true;
                pairs.push_back(pair);
            }
        }
    }
    
    return pairs;
}

bool BracketMatcher::isOpeningBracket(char ch) const {
    return openingBrackets_.find(ch) != std::string::npos;
}

bool BracketMatcher::isClosingBracket(char ch) const {
    return closingBrackets_.find(ch) != std::string::npos;
}

char BracketMatcher::getMatchingBracket(char ch) const {
    auto it = bracketPairs_.find(ch);
    if (it != bracketPairs_.end()) {
        return it->second;
    }
    
    // Check reverse
    for (const auto& [open, close] : bracketPairs_) {
        if (close == ch) {
            return open;
        }
    }
    
    return '\0';
}

void BracketMatcher::autoInsertClosingBracket(TextBuffer* buffer, size_t line, size_t col) {
    if (!buffer || line >= buffer->getLineCount()) return;

    std::string lineText = buffer->getLine(line);

    if (col >= lineText.length()) return;

    char ch = lineText[col];

    if (isOpeningBracket(ch)) {
        char closing = getMatchingBracket(ch);
        if (closing != '\0') {
            buffer->insert(line, col + 1, std::string(1, closing));
        }
    }
}

// BracketMatchingEngine implementation
BracketMatchingEngine::BracketMatchingEngine() = default;
BracketMatchingEngine::~BracketMatchingEngine() = default;

void BracketMatchingEngine::setConfig(const BracketMatchingConfig& config) {
    config_ = config;
}

BracketPair BracketMatchingEngine::findMatchingBracket(const std::string& text,
                                                        size_t line, size_t column) {
    TextBuffer buf;
    buf.setText(text);
    BracketMatcher matcher(&buf);
    BracketMatch m = matcher.findMatch(line, column);
    BracketPair pair;
    pair.matched = m.found;
    if (m.found) {
        pair.openLine = m.openLine;
        pair.openColumn = m.openCol;
        pair.closeLine = m.closeLine;
        pair.closeColumn = m.closeCol;
        pair.open = buf.getChar(m.openLine, m.openCol);
        pair.close = buf.getChar(m.closeLine, m.closeCol);
        pair.type = pair.open;
    }
    return pair;
}

std::vector<BracketPair> BracketMatchingEngine::findAllBrackets(const std::string& text,
                                                                 size_t startLine, size_t endLine) {
    TextBuffer buf;
    buf.setText(text);
    BracketMatcher matcher(&buf);
    std::vector<BracketPair> result;
    size_t maxLine = std::min(endLine, buf.getLineCount());
    for (size_t line = startLine; line < maxLine; ++line) {
        auto linePairs = matcher.findAllMatchingBrackets(line);
        result.insert(result.end(), linePairs.begin(), linePairs.end());
    }
    return result;
}

bool BracketMatchingEngine::isOnBracket(const std::string& line, size_t column) const {
    if (column >= line.length()) return false;
    char ch = line[column];
    for (const auto& [o, c] : config_.bracketPairs) {
        if (ch == o || ch == c) return true;
    }
    return false;
}

bool BracketMatchingEngine::isNearBracket(const std::string& text, size_t line, size_t column,
                                           size_t* bracketLine, size_t* bracketColumn) const {
    TextBuffer buf;
    buf.setText(text);
    if (line >= buf.getLineCount()) return false;
    std::string lineText = buf.getLine(line);
    if (isOnBracket(lineText, column)) {
        if (bracketLine) *bracketLine = line;
        if (bracketColumn) *bracketColumn = column;
        return true;
    }
    if (column > 0 && isOnBracket(lineText, column - 1)) {
        if (bracketLine) *bracketLine = line;
        if (bracketColumn) *bracketColumn = column - 1;
        return true;
    }
    return false;
}

BracketPair BracketMatchingEngine::getSurroundingBrackets(const std::string& text,
                                                           size_t line, size_t column) {
    return findMatchingBracket(text, line, column);
}

std::pair<size_t, size_t> BracketMatchingEngine::getBracketContents(const std::string& text,
                                                                     size_t line, size_t column) {
    BracketPair pair = findMatchingBracket(text, line, column);
    if (!pair.matched) return {0, 0};
    return {pair.openColumn + 1, pair.closeColumn};
}

int BracketMatchingEngine::findMatching(const std::string& text, size_t startPos,
                                         char open, char close, bool forward) {
    if (startPos >= text.size()) return -1;
    int depth = 1;
    if (forward) {
        for (size_t i = startPos + 1; i < text.size(); ++i) {
            if (text[i] == open) depth++;
            else if (text[i] == close && --depth == 0) return static_cast<int>(i);
        }
    } else {
        for (size_t i = startPos; i > 0; --i) {
            size_t idx = i - 1;
            if (text[idx] == close) depth++;
            else if (text[idx] == open && --depth == 0) return static_cast<int>(idx);
        }
    }
    return -1;
}

bool BracketMatchingEngine::isQuoteChar(char c) const {
    return c == '\'' || c == '"' || c == '`';
}

bool BracketMatchingEngine::isEscapeChar(const std::string& text, size_t pos) const {
    if (pos == 0 || pos >= text.size()) return false;
    size_t backslashes = 0;
    for (size_t i = pos; i > 0 && text[i - 1] == '\\'; --i) {
        backslashes++;
    }
    return (backslashes % 2) == 1;
}

} // namespace NexusForge::Editor
