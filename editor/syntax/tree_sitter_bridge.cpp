// editor/syntax/tree_sitter_bridge.cpp
// Tree-sitter Bridge Implementation

#include "tree_sitter_bridge.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <sstream>

namespace NexusForge::Editor {

TreeSitterBridge::TreeSitterBridge()
    : currentLanguage_(LanguageId::Unknown),
      languageLoaded_(false),
      initialized_(false),
      tree_(nullptr),
      parser_(nullptr),
      language_(nullptr),
      query_(nullptr) {
}

TreeSitterBridge::~TreeSitterBridge() {
    shutdown();
}

bool TreeSitterBridge::initialize() {
#ifdef NEXUS_HAS_TREE_SITTER
    parser_ = ts_parser_new();
    initialized_ = (parser_ != nullptr);
    return initialized_;
#else
    initialized_ = true;
    return true;
#endif
}

void TreeSitterBridge::shutdown() {
    freeTree();
#ifdef NEXUS_HAS_TREE_SITTER
    if (parser_) {
        ts_parser_delete(parser_);
        parser_ = nullptr;
    }
#endif
    language_ = nullptr;
    languageLoaded_ = false;
    initialized_ = false;
}

bool TreeSitterBridge::setLanguage(const std::string& languageId) {
    return loadLanguage(languageId);
}

bool TreeSitterBridge::loadLanguage(LanguageId language) {
    currentLanguage_ = language;
    languageLoaded_ = (language != LanguageId::Unknown);
    return languageLoaded_;
}

bool TreeSitterBridge::loadLanguage(const std::string& name) {
    languageName_ = name;
    if (name == "c") currentLanguage_ = LanguageId::C;
    else if (name == "cpp" || name == "c++") currentLanguage_ = LanguageId::Cpp;
    else if (name == "python" || name == "py") currentLanguage_ = LanguageId::Python;
    else if (name == "javascript" || name == "js") currentLanguage_ = LanguageId::JavaScript;
    else if (name == "typescript" || name == "ts") currentLanguage_ = LanguageId::TypeScript;
    else if (name == "rust" || name == "rs") currentLanguage_ = LanguageId::Rust;
    else if (name == "go") currentLanguage_ = LanguageId::Go;
    else if (name == "java") currentLanguage_ = LanguageId::Java;
    else if (name == "json") currentLanguage_ = LanguageId::JSON;
    else currentLanguage_ = LanguageId::Unknown;

#ifdef NEXUS_HAS_TREE_SITTER
    if (parser_) {
        const TSLanguage* lang = getLanguageForId(name);
        if (lang) {
            language_ = lang;
            languageLoaded_ = ts_parser_set_language(parser_, language_);
            return languageLoaded_;
        }
    }
#endif
    languageLoaded_ = !name.empty();
    return languageLoaded_;
}

bool TreeSitterBridge::loadLanguageFromFile(const std::string& path) {
    return loadGrammarFromFile(path);
}

void TreeSitterBridge::unloadLanguage() {
    freeTree();
    language_ = nullptr;
    currentLanguage_ = LanguageId::Unknown;
    languageName_.clear();
    languageLoaded_ = false;
}

SyntaxNode TreeSitterBridge::parse(const std::string& source) {
    SyntaxNode root;
    root.type = "program";
    root.text = source;
    root.startOffset = 0;
    root.endOffset = source.size();
    root.startLine = 0;
    root.startColumn = 0;

    size_t line = 0;
    size_t col = 0;
    for (char c : source) {
        if (c == '\n') { line++; col = 0; }
        else { col++; }
    }
    root.endLine = line;
    root.endColumn = col;

#ifdef NEXUS_HAS_TREE_SITTER
    if (parser_ && language_) {
        freeTree();
        tree_ = ts_parser_parse_string(parser_, nullptr, source.c_str(),
                                       static_cast<uint32_t>(source.length()));
    }
#endif
    lastParsedRoot_ = root;
    return root;
}

SyntaxNode TreeSitterBridge::parseFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return {};
    std::stringstream ss;
    ss << file.rdbuf();
    return parse(ss.str());
}

SyntaxNode TreeSitterBridge::parseWithEdits(
    const std::string& source,
    const std::vector<std::tuple<size_t, size_t, std::string>>& edits) {
    (void)edits;
    return parse(source);
}

void TreeSitterBridge::parseWithOldTree(const std::string& source, const TSRange* ranges, uint32_t rangeCount) {
    (void)ranges;
    (void)rangeCount;
#ifdef NEXUS_HAS_TREE_SITTER
    if (!parser_ || !language_) return;
    if (tree_) {
        TSTree* next = ts_parser_parse_string(parser_, tree_, source.c_str(),
                                              static_cast<uint32_t>(source.length()));
        ts_tree_delete(tree_);
        tree_ = next;
    } else {
        parse(source);
    }
#else
    parse(source);
#endif
}

std::string TreeSitterBridge::getNodeType(TSNode node) const {
#ifdef NEXUS_HAS_TREE_SITTER
    if (ts_node_is_null(node)) return "";
    return ts_node_type(node);
#else
    (void)node;
    return "";
#endif
}

TSNode TreeSitterBridge::getRootNode() const {
#ifdef NEXUS_HAS_TREE_SITTER
    if (!tree_) return TSNode{};
    return ts_tree_root_node(tree_);
#else
    return TSNode{};
#endif
}

std::vector<TreeSitterHighlight> TreeSitterBridge::getHighlights() const {
    std::vector<TreeSitterHighlight> highlights;
#ifdef NEXUS_HAS_TREE_SITTER
    if (!tree_) return highlights;
    TSNode root = ts_tree_root_node(tree_);
    collectHighlights(root, highlights);
#endif
    return highlights;
}

void TreeSitterBridge::collectHighlights(TSNode node, std::vector<TreeSitterHighlight>& highlights) const {
#ifdef NEXUS_HAS_TREE_SITTER
    if (ts_node_is_null(node)) return;

    std::string type = ts_node_type(node);
    if (!type.empty()) {
        TreeSitterHighlight highlight;
        highlight.type = type;
        highlight.startByte = ts_node_start_byte(node);
        highlight.endByte = ts_node_end_byte(node);
        highlight.startPoint = ts_node_start_point(node);
        highlight.endPoint = ts_node_end_point(node);
        highlight.category = categorizeNode(type);
        highlights.push_back(highlight);
    }

    TSTreeCursor cursor = ts_tree_cursor_new(node);
    if (ts_tree_cursor_goto_first_child(&cursor)) {
        do {
            TSNode child = ts_tree_cursor_current_node(&cursor);
            collectHighlights(child, highlights);
        } while (ts_tree_cursor_goto_next_sibling(&cursor));
    }
    ts_tree_cursor_delete(&cursor);
#else
    (void)node;
    (void)highlights;
#endif
}

TreeSitterHighlight::Category TreeSitterBridge::categorizeNode(const std::string& nodeType) const {
    if (nodeType.find("keyword") != std::string::npos ||
        nodeType == "if" || nodeType == "else" || nodeType == "for" ||
        nodeType == "while" || nodeType == "return" || nodeType == "break" ||
        nodeType == "continue" || nodeType == "switch" || nodeType == "case") {
        return TreeSitterHighlight::Category::Keyword;
    }
    if (nodeType.find("type") != std::string::npos ||
        nodeType == "primitive_type" || nodeType == "type_identifier" ||
        nodeType == "class_declaration" || nodeType == "struct_declaration") {
        return TreeSitterHighlight::Category::Type;
    }
    if (nodeType.find("function") != std::string::npos ||
        nodeType == "call_expression" || nodeType == "function_definition") {
        return TreeSitterHighlight::Category::Function;
    }
    if (nodeType.find("string") != std::string::npos ||
        nodeType == "string_literal" || nodeType == "character_literal") {
        return TreeSitterHighlight::Category::String;
    }
    if (nodeType.find("comment") != std::string::npos) {
        return TreeSitterHighlight::Category::Comment;
    }
    if (nodeType.find("number") != std::string::npos ||
        nodeType == "integer_literal" || nodeType == "float_literal") {
        return TreeSitterHighlight::Category::Number;
    }
    if (nodeType.find("operator") != std::string::npos ||
        nodeType == "binary_expression" || nodeType == "unary_expression") {
        return TreeSitterHighlight::Category::Operator;
    }
    return TreeSitterHighlight::Category::Identifier;
}

const TSLanguage* TreeSitterBridge::getLanguageForId(const std::string& languageId) const {
    (void)languageId;
    return nullptr;
}

std::vector<std::string> TreeSitterBridge::getAvailableLanguages() const {
    return {
        "cpp", "c", "python", "javascript", "typescript",
        "rust", "go", "java", "csharp", "ruby", "php",
        "swift", "kotlin", "scala", "r", "julia"
    };
}

bool TreeSitterBridge::loadGrammarFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    return true;
}

void TreeSitterBridge::setLogger(TSLogger logger) {
#ifdef NEXUS_HAS_TREE_SITTER
    if (parser_) {
        ts_parser_set_logger(parser_, logger);
    }
#else
    (void)logger;
#endif
}

uint32_t TreeSitterBridge::getLanguageVersion() const {
#ifdef NEXUS_HAS_TREE_SITTER
    if (language_) {
        return ts_language_version(language_);
    }
#endif
    return 0;
}

size_t TreeSitterBridge::getSymbolCount() const {
#ifdef NEXUS_HAS_TREE_SITTER
    if (language_) {
        return ts_language_symbol_count(language_);
    }
#endif
    return 0;
}

std::string TreeSitterBridge::getSymbolName(uint32_t symbol) const {
#ifdef NEXUS_HAS_TREE_SITTER
    if (language_) {
        const char* name = ts_language_symbol_name(language_, static_cast<TSSymbol>(symbol));
        return name ? name : "";
    }
#else
    (void)symbol;
#endif
    return "";
}

bool TreeSitterBridge::loadQuery(const std::string& query) {
    (void)query;
    return true;
}

std::vector<QueryMatch> TreeSitterBridge::executeQuery(const SyntaxNode& node) {
    (void)node;
    return {};
}

SyntaxNode TreeSitterBridge::getNodeAtOffset(const SyntaxNode& root, size_t offset) {
    for (const auto& child : root.children) {
        if (offset >= child.startOffset && offset <= child.endOffset) {
            return getNodeAtOffset(child, offset);
        }
    }
    return root;
}

SyntaxNode TreeSitterBridge::getNodeAtPosition(const SyntaxNode& root, size_t line, size_t column) {
    for (const auto& child : root.children) {
        if (line >= child.startLine && line <= child.endLine) {
            return getNodeAtPosition(child, line, column);
        }
    }
    return root;
}

std::vector<SyntaxNode> TreeSitterBridge::getPathToNode(const SyntaxNode& root, size_t offset) {
    std::vector<SyntaxNode> path;
    path.push_back(root);
    for (const auto& child : root.children) {
        if (offset >= child.startOffset && offset <= child.endOffset) {
            auto sub = getPathToNode(child, offset);
            path.insert(path.end(), sub.begin(), sub.end());
            break;
        }
    }
    return path;
}

std::vector<SyntaxNode> TreeSitterBridge::findSymbols(const SyntaxNode& root,
                                                       const std::vector<std::string>& types) {
    std::vector<SyntaxNode> result;
    if (std::find(types.begin(), types.end(), root.type) != types.end()) {
        result.push_back(root);
    }
    for (const auto& child : root.children) {
        auto sub = findSymbols(child, types);
        result.insert(result.end(), sub.begin(), sub.end());
    }
    return result;
}

std::string TreeSitterBridge::getSymbolAtOffset(const SyntaxNode& root, size_t offset) {
    return getNodeAtOffset(root, offset).text;
}

std::vector<std::pair<size_t, size_t>> TreeSitterBridge::getFoldRanges(const SyntaxNode& root) {
    std::vector<std::pair<size_t, size_t>> ranges;
    if (root.endLine > root.startLine) {
        ranges.emplace_back(root.startLine, root.endLine);
    }
    for (const auto& child : root.children) {
        auto sub = getFoldRanges(child);
        ranges.insert(ranges.end(), sub.begin(), sub.end());
    }
    return ranges;
}

std::string TreeSitterBridge::getLanguageName() const {
    if (!languageName_.empty()) return languageName_;
    switch (currentLanguage_) {
        case LanguageId::C: return "c";
        case LanguageId::Cpp: return "cpp";
        case LanguageId::Python: return "python";
        case LanguageId::JavaScript: return "javascript";
        case LanguageId::TypeScript: return "typescript";
        case LanguageId::Rust: return "rust";
        case LanguageId::Go: return "go";
        default: return "unknown";
    }
}

std::string TreeSitterBridge::nodeTypeToString(int typeId) const {
    return std::to_string(typeId);
}

void TreeSitterBridge::freeTree() {
#ifdef NEXUS_HAS_TREE_SITTER
    if (tree_) {
        ts_tree_delete(tree_);
        tree_ = nullptr;
    }
#else
    tree_ = nullptr;
#endif
}

// SyntaxTreeCache implementation
SyntaxTreeCache::SyntaxTreeCache() = default;
SyntaxTreeCache::~SyntaxTreeCache() = default;

void SyntaxTreeCache::setCacheSize(size_t maxEntries) {
    maxEntries_ = maxEntries;
    trimCache();
}

void SyntaxTreeCache::clear() {
    cache_.clear();
}

SyntaxNode SyntaxTreeCache::getOrParse(const std::string& documentId,
                                        const std::string& source,
                                        TreeSitterBridge& bridge) {
    std::string hash = computeHash(source);
    auto it = cache_.find(documentId);
    uint64_t now = static_cast<uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    if (it != cache_.end() && it->second.sourceHash == hash) {
        it->second.lastAccess = now;
        return it->second.root;
    }
    SyntaxNode root = bridge.parse(source);
    cache_[documentId] = {root, hash, now};
    trimCache();
    return root;
}

void SyntaxTreeCache::invalidate(const std::string& documentId) {
    cache_.erase(documentId);
}

void SyntaxTreeCache::invalidateRange(const std::string& documentId,
                                       size_t startLine, size_t endLine) {
    (void)startLine;
    (void)endLine;
    cache_.erase(documentId);
}

std::string SyntaxTreeCache::computeHash(const std::string& source) {
    return std::to_string(std::hash<std::string>{}(source));
}

void SyntaxTreeCache::trimCache() {
    while (cache_.size() > maxEntries_ && !cache_.empty()) {
        auto oldest = cache_.begin();
        for (auto it = cache_.begin(); it != cache_.end(); ++it) {
            if (it->second.lastAccess < oldest->second.lastAccess) {
                oldest = it;
            }
        }
        cache_.erase(oldest);
    }
}

} // namespace NexusForge::Editor
