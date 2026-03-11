// editor/syntax/tree_sitter_bridge.hpp
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace NexusForge::Editor {

// Tree-sitter node
struct SyntaxNode {
    std::string type;
    std::string text;
    size_t startOffset = 0;
    size_t endOffset = 0;
    size_t startLine = 0;
    size_t startColumn = 0;
    size_t endLine = 0;
    size_t endColumn = 0;
    bool isNamed = true;
    bool hasError = false;
    std::vector<SyntaxNode> children;
};

// Tree-sitter query match
struct QueryMatch {
    size_t patternIndex;
    std::vector<SyntaxNode> captures;
    std::unordered_map<std::string, SyntaxNode> captureByName;
};

// Language support
enum class LanguageId {
    Unknown,
    C,
    Cpp,
    Python,
    JavaScript,
    TypeScript,
    Rust,
    Go,
    Java,
    CSharp,
    Ruby,
    PHP,
    Swift,
    Kotlin,
    HTML,
    CSS,
    JSON,
    XML,
    Markdown,
    SQL,
    Shell,
    YAML,
    TOML
};

// Tree-sitter bridge
class TreeSitterBridge {
public:
    TreeSitterBridge();
    ~TreeSitterBridge();

    // Initialize
    bool initialize();
    void shutdown();

    // Language
    bool loadLanguage(LanguageId language);
    bool loadLanguage(const std::string& name);
    bool loadLanguageFromFile(const std::string& path);
    void unloadLanguage();

    // Parse
    SyntaxNode parse(const std::string& source);
    SyntaxNode parseFile(const std::string& path);

    // Incremental parsing
    SyntaxNode parseWithEdits(const std::string& source,
                               const std::vector<std::tuple<size_t, size_t, std::string>>& edits);

    // Query
    bool loadQuery(const std::string& query);
    std::vector<QueryMatch> executeQuery(const SyntaxNode& node);

    // Navigation
    SyntaxNode getNodeAtOffset(const SyntaxNode& root, size_t offset);
    SyntaxNode getNodeAtPosition(const SyntaxNode& root, size_t line, size_t column);
    std::vector<SyntaxNode> getPathToNode(const SyntaxNode& root, size_t offset);

    // Symbols
    std::vector<SyntaxNode> findSymbols(const SyntaxNode& root,
                                         const std::vector<std::string>& types);
    std::string getSymbolAtOffset(const SyntaxNode& root, size_t offset);

    // Folding
    std::vector<std::pair<size_t, size_t>> getFoldRanges(const SyntaxNode& root);

    // Info
    LanguageId getCurrentLanguage() const { return currentLanguage_; }
    std::string getLanguageName() const;
    bool isLoaded() const { return languageLoaded_; }

private:
    LanguageId currentLanguage_ = LanguageId::Unknown;
    bool languageLoaded_ = false;
    void* tree_ = nullptr;
    void* parser_ = nullptr;
    void* language_ = nullptr;
    void* query_ = nullptr;

    std::string nodeTypeToString(int typeId) const;
    void freeTree();
};

// Syntax tree cache
class SyntaxTreeCache {
public:
    SyntaxTreeCache();
    ~SyntaxTreeCache();

    void setCacheSize(size_t maxEntries);
    void clear();

    SyntaxNode getOrParse(const std::string& documentId,
                          const std::string& source,
                          TreeSitterBridge& bridge);

    void invalidate(const std::string& documentId);
    void invalidateRange(const std::string& documentId,
                         size_t startLine, size_t endLine);

private:
    struct CachedTree {
        SyntaxNode root;
        std::string sourceHash;
        uint64_t lastAccess;
    };

    std::unordered_map<std::string, CachedTree> cache_;
    size_t maxEntries_ = 10;

    std::string computeHash(const std::string& source);
    void trimCache();
};

} // namespace NexusForge::Editor
