// editor/syntax/syntax_highlighter.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <regex>

namespace NexusForge::Editor {

// Token types
enum class TokenType {
    Unknown,
    Keyword,
    Identifier,
    Type,
    Function,
    Variable,
    Parameter,
    String,
    Character,
    Number,
    Comment,
    DocComment,
    Operator,
    Delimiter,
    Whitespace,
    Preprocessor,
    Tag,
    Attribute,
    Error
};

// Token style
struct TokenStyle {
    uint32_t foregroundColor = 0xFFFFFFFF;
    uint32_t backgroundColor = 0x00000000;
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool strikethrough = false;
};

// Token
struct Token {
    TokenType type = TokenType::Unknown;
    size_t startOffset = 0;
    size_t length = 0;
    std::string text;
    TokenStyle style;

    size_t endOffset() const { return startOffset + length; }
};

// Line highlight info
struct LineHighlight {
    size_t lineNumber;
    std::vector<Token> tokens;
    bool isComplete = false;
};

// Language definition
struct LanguageDefinition {
    std::string name;
    std::string fileExtension;
    std::vector<std::string> fileExtensions;
    bool caseSensitive = true;

    // Keywords
    std::unordered_map<std::string, TokenType> keywords;
    std::vector<std::string> builtInTypes;
    std::vector<std::string> builtInFunctions;

    // Patterns
    std::vector<std::pair<std::regex, TokenType>> patterns;

    // Comments
    std::string lineCommentPrefix;
    std::string blockCommentStart;
    std::string blockCommentEnd;

    // Strings
    std::vector<char> stringDelimiters;
    char escapeCharacter = '\\';

    // Token styles
    std::unordered_map<TokenType, TokenStyle> tokenStyles;
};

// Syntax highlighter
class SyntaxHighlighter {
public:
    SyntaxHighlighter();
    ~SyntaxHighlighter();

    // Language
    void setLanguage(const std::string& languageId);
    const std::string& getLanguage() const { return currentLanguage_; }
    void loadLanguageDefinition(const LanguageDefinition& def);

    // Highlighting
    std::vector<Token> tokenize(const std::string& text);
    std::vector<Token> tokenizeLine(const std::string& line, size_t lineNumber);
    LineHighlight highlightLine(const std::string& line, size_t lineNumber);

    // Batch highlighting
    std::vector<LineHighlight> highlightRange(const std::string& text,
                                               size_t startLine, size_t endLine);

    // Incremental highlighting
    void invalidateLine(size_t lineNumber);
    void invalidateRange(size_t startLine, size_t endLine);
    std::vector<size_t> getInvalidatedLines() const;

    // Theme
    void setTheme(const std::string& themeId);
    void updateTokenStyle(TokenType type, const TokenStyle& style);
    const TokenStyle& getTokenStyle(TokenType type) const;

    // Utilities
    bool isComment(const Token& token) const;
    bool isString(const Token& token) const;
    bool isKeyword(const std::string& text) const;
    bool isIdentifier(const std::string& text) const;

    // Language info
    std::string getLineCommentPrefix() const;
    std::pair<std::string, std::string> getBlockCommentDelimiters() const;
    bool hasLineComments() const;
    bool hasBlockComments() const;

private:
    std::string currentLanguage_;
    LanguageDefinition languageDef_;
    std::unordered_map<std::string, LanguageDefinition> languageDefinitions_;
    std::unordered_map<TokenType, TokenStyle> themeStyles_;

    // Incremental highlighting state
    std::vector<size_t> invalidatedLines_;
    std::vector<LineHighlight> cachedLines_;

    Token tokenizeKeyword(const std::string& text, size_t offset);
    Token tokenizeIdentifier(const std::string& text, size_t offset);
    Token tokenizeNumber(const std::string& text, size_t offset);
    Token tokenizeString(const std::string& text, size_t offset);
    Token tokenizeComment(const std::string& text, size_t offset);
    Token tokenizeOperator(const std::string& text, size_t offset);

    void applyStyle(Token& token) const;
};

// Language registry
class LanguageRegistry {
public:
    static LanguageRegistry& getInstance();

    void registerLanguage(const LanguageDefinition& def);
    const LanguageDefinition* getLanguage(const std::string& id) const;
    const LanguageDefinition* getLanguageByExtension(const std::string& ext) const;
    std::vector<std::string> getRegisteredLanguages() const;

    void loadDefaultLanguages();

private:
    LanguageRegistry() = default;
    std::unordered_map<std::string, LanguageDefinition> languages_;
};

// Built-in language definitions
LanguageDefinition createCppLanguage();
LanguageDefinition createPythonLanguage();
LanguageDefinition createJavaScriptLanguage();
LanguageDefinition createTypeScriptLanguage();
LanguageDefinition createJavaLanguage();
LanguageDefinition createRustLanguage();
LanguageDefinition createGoLanguage();
LanguageDefinition createCSharpLanguage();
LanguageDefinition createHtmlLanguage();
LanguageDefinition createCssLanguage();
LanguageDefinition createJsonLanguage();
LanguageDefinition createMarkdownLanguage();
LanguageDefinition createXmlLanguage();
LanguageDefinition createSqlLanguage();
LanguageDefinition createShellLanguage();

} // namespace NexusForge::Editor
