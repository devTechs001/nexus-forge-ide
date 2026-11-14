// editor/syntax/syntax_highlighter.cpp
// Syntax Highlighter Implementation

#include "syntax_highlighter.hpp"
#include "../buffer/text_buffer.hpp"
#include <algorithm>
#include <regex>

namespace NexusForge::Editor {

SyntaxHighlighter::SyntaxHighlighter() = default;

SyntaxHighlighter::~SyntaxHighlighter() = default;

void SyntaxHighlighter::setLanguage(const std::string& languageId) {
    languageId_ = languageId;
    loadGrammar(languageId);
}

void SyntaxHighlighter::setTextBuffer(TextBuffer* buffer) {
    buffer_ = buffer;
    if (buffer_) {
        highlightAll();
    }
}

void SyntaxHighlighter::highlightAll() {
    if (!buffer_) return;
    
    tokens_.clear();
    
    for (size_t line = 0; line < buffer_->getLineCount(); ++line) {
        highlightLine(line);
    }
}

void SyntaxHighlighter::highlightLine(size_t lineNum) {
    if (!buffer_ || lineNum >= buffer_->getLineCount()) return;
    
    std::string line = buffer_->getLine(lineNum);
    std::vector<Token> lineTokens = tokenizeLine(line, lineNum);
    
    tokens_[lineNum] = lineTokens;
}

std::vector<SyntaxHighlighter::Token> SyntaxHighlighter::tokenizeLine(
    const std::string& line, size_t lineNum) {
    
    std::vector<Token> tokens;
    
    if (languageId_.empty()) {
        // No language - return single token
        if (!line.empty()) {
            Token token;
            token.type = TokenType::Text;
            token.start = 0;
            token.length = line.length();
            token.line = lineNum;
            tokens.push_back(token);
        }
        return tokens;
    }
    
    // Simple regex-based tokenization
    // In production, use Tree-sitter or similar
    
    size_t pos = 0;
    
    while (pos < line.length()) {
        // Skip whitespace
        if (std::isspace(line[pos])) {
            size_t start = pos;
            while (pos < line.length() && std::isspace(line[pos])) {
                pos++;
            }
            
            Token token;
            token.type = TokenType::Whitespace;
            token.start = start;
            token.length = pos - start;
            token.line = lineNum;
            tokens.push_back(token);
            continue;
        }
        
        // Check for comments
        if (pos + 1 < line.length()) {
            std::string twoChars = line.substr(pos, 2);
            
            if (twoChars == "//" || twoChars == "/*" || twoChars == "#!") {
                size_t start = pos;
                
                if (twoChars == "//") {
                    pos = line.length();  // Rest of line
                } else if (twoChars == "/*") {
                    // Simple single-line block comment handling
                    pos += 2;
                    while (pos + 1 < line.length() && 
                           line.substr(pos, 2) != "*/") {
                        pos++;
                    }
                    pos += 2;
                } else {
                    pos = line.length();
                }
                
                Token token;
                token.type = TokenType::Comment;
                token.start = start;
                token.length = pos - start;
                token.line = lineNum;
                tokens.push_back(token);
                continue;
            }
        }
        
        // Check for strings
        if (line[pos] == '"' || line[pos] == '\'' || line[pos] == '`') {
            char quote = line[pos];
            size_t start = pos;
            pos++;
            
            while (pos < line.length() && line[pos] != quote) {
                if (line[pos] == '\\' && pos + 1 < line.length()) {
                    pos += 2;  // Skip escaped character
                } else {
                    pos++;
                }
            }
            pos++;  // Skip closing quote
            
            Token token;
            token.type = TokenType::String;
            token.start = start;
            token.length = pos - start;
            token.line = lineNum;
            tokens.push_back(token);
            continue;
        }
        
        // Check for numbers
        if (std::isdigit(line[pos]) || 
            (line[pos] == '.' && pos + 1 < line.length() && std::isdigit(line[pos + 1]))) {
            size_t start = pos;
            
            // Handle hex, binary, octal
            if (line[pos] == '0' && pos + 1 < line.length()) {
                if (line[pos + 1] == 'x' || line[pos + 1] == 'X') {
                    pos += 2;
                    while (pos < line.length() && std::isxdigit(line[pos])) {
                        pos++;
                    }
                } else if (line[pos + 1] == 'b' || line[pos + 1] == 'B') {
                    pos += 2;
                    while (pos < line.length() && (line[pos] == '0' || line[pos] == '1')) {
                        pos++;
                    }
                } else {
                    while (pos < line.length() && std::isdigit(line[pos])) {
                        pos++;
                    }
                }
            } else {
                while (pos < line.length() && 
                       (std::isdigit(line[pos]) || line[pos] == '.' ||
                        line[pos] == 'e' || line[pos] == 'E' ||
                        line[pos] == '+' || line[pos] == '-')) {
                    pos++;
                }
            }
            
            // Handle suffixes (f, l, u, etc.)
            while (pos < line.length() && 
                   (line[pos] == 'f' || line[pos] == 'F' ||
                    line[pos] == 'l' || line[pos] == 'L' ||
                    line[pos] == 'u' || line[pos] == 'U')) {
                pos++;
            }
            
            Token token;
            token.type = TokenType::Number;
            token.start = start;
            token.length = pos - start;
            token.line = lineNum;
            tokens.push_back(token);
            continue;
        }
        
        // Check for identifiers/keywords
        if (std::isalpha(line[pos]) || line[pos] == '_') {
            size_t start = pos;
            
            while (pos < line.length() && 
                   (std::isalnum(line[pos]) || line[pos] == '_')) {
                pos++;
            }
            
            std::string identifier = line.substr(start, pos - start);
            TokenType type = classifyIdentifier(identifier);
            
            Token token;
            token.type = type;
            token.start = start;
            token.length = pos - start;
            token.line = lineNum;
            token.value = identifier;
            tokens.push_back(token);
            continue;
        }
        
        // Operators and punctuation
        size_t start = pos;
        while (pos < line.length() && 
               !std::isalnum(line[pos]) && 
               !std::isspace(line[pos]) &&
               line[pos] != '"' && line[pos] != '\'' && line[pos] != '`') {
            pos++;
        }
        
        std::string op = line.substr(start, pos - start);
        TokenType type = classifyOperator(op);
        
        Token token;
        token.type = type;
        token.start = start;
        token.length = pos - start;
        token.line = lineNum;
        token.value = op;
        tokens.push_back(token);
    }
    
    return tokens;
}

SyntaxHighlighter::TokenType SyntaxHighlighter::classifyIdentifier(
    const std::string& identifier) const {
    
    // Keywords for common languages
    static const std::vector<std::string> keywords = {
        "if", "else", "for", "while", "do", "switch", "case", "break",
        "continue", "return", "goto", "default", "sizeof", "typeof",
        "class", "struct", "union", "enum", "namespace", "template",
        "public", "private", "protected", "friend", "virtual", "override",
        "const", "static", "extern", "inline", "volatile", "mutable",
        "try", "catch", "throw", "finally", "throws",
        "new", "delete", "this", "nullptr", "NULL",
        "true", "false", "bool", "auto", "register",
        "int", "char", "short", "long", "float", "double", "void",
        "signed", "unsigned", "const_cast", "dynamic_cast",
        "reinterpret_cast", "static_cast",
        "def", "class", "import", "from", "as", "lambda", "yield",
        "async", "await", "let", "var", "const", "function", "arrow",
        "fn", "mod", "pub", "use", "impl", "trait", "struct", "enum",
        "match", "ref", "mut", "move", "borrow"
    };
    
    if (std::find(keywords.begin(), keywords.end(), identifier) != keywords.end()) {
        return TokenType::Keyword;
    }
    
    // Check for built-in types
    static const std::vector<std::string> types = {
        "int8_t", "int16_t", "int32_t", "int64_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t",
        "size_t", "ptrdiff_t", "intptr_t", "uintptr_t",
        "string", "vector", "map", "set", "list", "array",
        "optional", "variant", "any", "function",
        "String", "Integer", "Boolean", "Object", "Array"
    };
    
    if (std::find(types.begin(), types.end(), identifier) != types.end()) {
        return TokenType::Type;
    }
    
    // Check for common functions
    static const std::vector<std::string> functions = {
        "printf", "sprintf", "fprintf", "scanf", "malloc", "free",
        "new", "delete", "sizeof", "alignof",
        "print", "println", "input", "len", "range", "str", "int",
        "console.log", "require", "module.exports",
        "std::cout", "std::cin", "std::endl"
    };
    
    if (std::find(functions.begin(), functions.end(), identifier) != functions.end()) {
        return TokenType::Function;
    }
    
    // Capitalized = likely a type
    if (!identifier.empty() && std::isupper(identifier[0])) {
        return TokenType::Type;
    }
    
    return TokenType::Identifier;
}

SyntaxHighlighter::TokenType SyntaxHighlighter::classifyOperator(
    const std::string& op) const {
    
    if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%" ||
        op == "++" || op == "--" || op == "+=" || op == "-=" ||
        op == "*=" || op == "/=" || op == "%=" ||
        op == "==" || op == "!=" || op == "<" || op == ">" ||
        op == "<=" || op == ">=" || op == "&&" || op == "||" ||
        op == "!" || op == "&" || op == "|" || op == "^" ||
        op == "~" || op == "<<" || op == ">>" ||
        op == "&=" || op == "|=" || op == "^=" || op == "<<=" || op == ">>=") {
        return TokenType::Operator;
    }
    
    if (op == "(" || op == ")" || op == "[" || op == "]" ||
        op == "{" || op == "}" || op == "," || op == ";" ||
        op == "." || op == "->" || op == "::" || op == "?" || op == ":") {
        return TokenType::Punctuation;
    }
    
    return TokenType::Operator;
}

const std::vector<SyntaxHighlighter::Token>* SyntaxHighlighter::getTokens(
    size_t lineNum) const {
    
    auto it = tokens_.find(lineNum);
    if (it != tokens_.end()) {
        return &it->second;
    }
    return nullptr;
}

void SyntaxHighlighter::clearTokens() {
    tokens_.clear();
}

void SyntaxHighlighter::loadGrammar(const std::string& languageId) {
    // In production, load grammar from file or Tree-sitter
    languageId_ = languageId;
}

void SyntaxHighlighter::onTextChanged(size_t startLine, size_t startCol,
                                       size_t endLine, size_t endCol) {
    // Re-highlight affected lines
    for (size_t line = startLine; line <= endLine && line < buffer_->getLineCount(); ++line) {
        highlightLine(line);
    }
    
    // Also highlight a few lines after in case of multi-line constructs
    for (size_t line = endLine + 1; 
         line < std::min(endLine + 5, buffer_->getLineCount()); ++line) {
        highlightLine(line);
    }
    
    if (onHighlightChanged) {
        onHighlightChanged(startLine, endLine);
    }
}

std::string SyntaxHighlighter::tokenTypeToString(TokenType type) const {
    switch (type) {
        case TokenType::Keyword: return "keyword";
        case TokenType::Type: return "type";
        case TokenType::Function: return "function";
        case TokenType::Identifier: return "identifier";
        case TokenType::String: return "string";
        case TokenType::Number: return "number";
        case TokenType::Comment: return "comment";
        case TokenType::Operator: return "operator";
        case TokenType::Punctuation: return "punctuation";
        case TokenType::Whitespace: return "whitespace";
        default: return "text";
    }
}

} // namespace NexusForge::Editor
