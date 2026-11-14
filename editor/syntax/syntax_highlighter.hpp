// editor/syntax/syntax_highlighter.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <functional>
#include <memory>

namespace NexusForge::Editor {

class TextBuffer;

class SyntaxHighlighter {
public:
    enum class TokenType {
        Text,
        Keyword,
        Type,
        Function,
        Identifier,
        String,
        Number,
        Comment,
        Operator,
        Punctuation,
        Whitespace,
        Unknown
    };

    struct Token {
        TokenType type = TokenType::Text;
        size_t start = 0;
        size_t length = 0;
        size_t line = 0;
        std::string value;
    };

    SyntaxHighlighter();
    ~SyntaxHighlighter();

    void setLanguage(const std::string& languageId);
    const std::string& getLanguage() const { return languageId_; }

    void setTextBuffer(TextBuffer* buffer);
    void highlightAll();
    void highlightLine(size_t lineNum);

    std::vector<Token> tokenizeLine(const std::string& line, size_t lineNum);
    const std::vector<Token>* getTokens(size_t lineNum) const;
    void clearTokens();

    void loadGrammar(const std::string& languageId);
    void onTextChanged(size_t startLine, size_t startCol,
                       size_t endLine, size_t endCol);

    std::string tokenTypeToString(TokenType type) const;

    std::function<void(size_t startLine, size_t endLine)> onHighlightChanged;

private:
    std::string languageId_;
    TextBuffer* buffer_ = nullptr;
    std::map<size_t, std::vector<Token>> tokens_;

    TokenType classifyIdentifier(const std::string& identifier) const;
    TokenType classifyOperator(const std::string& op) const;
};

} // namespace NexusForge::Editor
