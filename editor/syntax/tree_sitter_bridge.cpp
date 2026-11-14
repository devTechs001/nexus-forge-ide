// editor/syntax/tree_sitter_bridge.cpp
// Tree-sitter Bridge Implementation

#include "tree_sitter_bridge.hpp"
#include <fstream>
#include <cstring>

namespace NexusForge::Editor {

TreeSitterBridge::TreeSitterBridge() : parser_(nullptr), tree_(nullptr), language_(nullptr) {
}

TreeSitterBridge::~TreeSitterBridge() {
    shutdown();
}

bool TreeSitterBridge::initialize() {
    parser_ = ts_parser_new();
    return parser_ != nullptr;
}

void TreeSitterBridge::shutdown() {
    if (tree_) {
        ts_tree_delete(tree_);
        tree_ = nullptr;
    }
    
    if (parser_) {
        ts_parser_delete(parser_);
        parser_ = nullptr;
    }
    
    language_ = nullptr;
}

bool TreeSitterBridge::setLanguage(const std::string& languageId) {
    if (!parser_) return false;
    
    const TSLanguage* lang = getLanguageForId(languageId);
    if (!lang) return false;
    
    language_ = lang;
    return ts_parser_set_language(parser_, language_);
}

void TreeSitterBridge::parse(const std::string& source) {
    if (!parser_ || !language_) return;
    
    if (tree_) {
        ts_tree_delete(tree_);
    }
    
    tree_ = ts_parser_parse_string(parser_, nullptr, source.c_str(), source.length());
}

void TreeSitterBridge::parseWithOldTree(const std::string& source, const TSRange* ranges, uint32_t rangeCount) {
    if (!parser_ || !language_) return;
    
    if (tree_) {
        tree_ = ts_parser_parse_string(parser_, tree_, source.c_str(), source.length());
    } else {
        parse(source);
    }
}

std::string TreeSitterBridge::getNodeType(TSNode node) const {
    if (ts_node_is_null(node)) return "";
    return ts_node_type(node);
}

TSNode TreeSitterBridge::getRootNode() const {
    if (!tree_) return ts_node_new(nullptr, 0, 0, 0);
    return ts_tree_root_node(tree_);
}

std::vector<TreeSitterHighlight> TreeSitterBridge::getHighlights() const {
    std::vector<TreeSitterHighlight> highlights;
    
    if (!tree_) return highlights;
    
    TSNode root = ts_tree_root_node(tree_);
    collectHighlights(root, highlights);
    
    return highlights;
}

void TreeSitterBridge::collectHighlights(TSNode node, std::vector<TreeSitterHighlight>& highlights) const {
    if (ts_node_is_null(node)) return;
    
    // Check if this node should be highlighted
    std::string type = ts_node_type(node);
    
    if (!type.empty()) {
        TreeSitterHighlight highlight;
        highlight.type = type;
        highlight.startByte = ts_node_start_byte(node);
        highlight.endByte = ts_node_end_byte(node);
        highlight.startPoint = ts_node_start_point(node);
        highlight.endPoint = ts_node_end_point(node);
        
        // Determine highlight category
        highlight.category = categorizeNode(type);
        
        highlights.push_back(highlight);
    }
    
    // Recurse into children
    TSNodeCursor cursor = ts_tree_cursor_new(node);
    
    if (ts_tree_cursor_goto_first_child(&cursor)) {
        do {
            TSNode child = ts_tree_cursor_current_node(&cursor);
            collectHighlights(child, highlights);
        } while (ts_tree_cursor_goto_next_sibling(&cursor));
    }
    
    ts_tree_cursor_delete(&cursor);
}

TreeSitterHighlight::Category TreeSitterBridge::categorizeNode(const std::string& nodeType) const {
    // Keywords
    if (nodeType.find("keyword") != std::string::npos ||
        nodeType == "if" || nodeType == "else" || nodeType == "for" ||
        nodeType == "while" || nodeType == "return" || nodeType == "break" ||
        nodeType == "continue" || nodeType == "switch" || nodeType == "case") {
        return TreeSitterHighlight::Category::Keyword;
    }
    
    // Types
    if (nodeType.find("type") != std::string::npos ||
        nodeType == "primitive_type" || nodeType == "type_identifier" ||
        nodeType == "class_declaration" || nodeType == "struct_declaration") {
        return TreeSitterHighlight::Category::Type;
    }
    
    // Functions
    if (nodeType.find("function") != std::string::npos ||
        nodeType == "call_expression" || nodeType == "function_definition") {
        return TreeSitterHighlight::Category::Function;
    }
    
    // Strings
    if (nodeType.find("string") != std::string::npos ||
        nodeType == "string_literal" || nodeType == "character_literal") {
        return TreeSitterHighlight::Category::String;
    }
    
    // Comments
    if (nodeType.find("comment") != std::string::npos) {
        return TreeSitterHighlight::Category::Comment;
    }
    
    // Numbers
    if (nodeType.find("number") != std::string::npos ||
        nodeType == "integer_literal" || nodeType == "float_literal") {
        return TreeSitterHighlight::Category::Number;
    }
    
    // Operators
    if (nodeType.find("operator") != std::string::npos ||
        nodeType == "binary_expression" || nodeType == "unary_expression") {
        return TreeSitterHighlight::Category::Operator;
    }
    
    return TreeSitterHighlight::Category::Identifier;
}

const TSLanguage* TreeSitterBridge::getLanguageForId(const std::string& languageId) const {
    // In production, load from tree-sitter grammar files
    // This is a simplified implementation
    
    // These would be linked at compile time with actual tree-sitter grammars
    // extern "C" const TSLanguage* tree_sitter_cpp();
    // extern "C" const TSLanguage* tree_sitter_python();
    // etc.
    
    if (languageId == "cpp" || languageId == "c++" || languageId == "c") {
        // return tree_sitter_cpp();
    } else if (languageId == "python" || languageId == "py") {
        // return tree_sitter_python();
    } else if (languageId == "javascript" || languageId == "js") {
        // return tree_sitter_javascript();
    } else if (languageId == "typescript" || languageId == "ts") {
        // return tree_sitter_typescript();
    } else if (languageId == "rust" || languageId == "rs") {
        // return tree_sitter_rust();
    } else if (languageId == "go") {
        // return tree_sitter_go();
    }
    
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
    
    std::vector<char> buffer((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
    
    // In production, compile and load the grammar
    // This is a placeholder
    
    return true;
}

void TreeSitterBridge::setLogger(TSLogger logger) {
    if (parser_) {
        ts_parser_set_logger(parser_, logger);
    }
}

uint32_t TreeSitterBridge::getLanguageVersion() const {
    if (language_) {
        return ts_language_version(language_);
    }
    return 0;
}

size_t TreeSitterBridge::getSymbolCount() const {
    if (language_) {
        return ts_language_symbol_count(language_);
    }
    return 0;
}

std::string TreeSitterBridge::getSymbolName(uint32_t symbol) const {
    if (language_) {
        return ts_language_symbol_name(language_, symbol);
    }
    return "";
}

} // namespace NexusForge::Editor
