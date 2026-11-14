// editor/intellisense/completion_provider.cpp
// Completion Provider Implementation

#include "completion_provider.hpp"
#include <algorithm>
#include <set>

namespace NexusForge::Editor {

CompletionProvider::CompletionProvider() : triggerCharDelay_(300) {
}

CompletionProvider::~CompletionProvider() = default;

void CompletionProvider::setLSPClient(LSPClient* client) {
    lspClient_ = client;
}

void CompletionProvider::setLanguage(const std::string& languageId) {
    languageId_ = languageId;
    loadLanguageKeywords(languageId);
}

void CompletionProvider::loadLanguageKeywords(const std::string& languageId) {
    keywords_.clear();
    
    // Common programming keywords by language
    if (languageId == "cpp" || languageId == "c" || languageId == "c++") {
        keywords_ = {
            "auto", "break", "case", "catch", "class", "const", "constexpr",
            "continue", "default", "delete", "do", "else", "enum", "explicit",
            "export", "extern", "false", "for", "friend", "goto", "if",
            "inline", "mutable", "namespace", "new", "noexcept", "nullptr",
            "operator", "private", "protected", "public", "register", "return",
            "short", "signed", "sizeof", "static", "struct", "switch",
            "template", "this", "throw", "true", "try", "typedef", "typeid",
            "typename", "union", "unsigned", "using", "virtual", "void",
            "volatile", "while", "override", "final", "decltype", "static_assert"
        };
    } else if (languageId == "python" || languageId == "py") {
        keywords_ = {
            "and", "as", "assert", "async", "await", "break", "class",
            "continue", "def", "del", "elif", "else", "except", "False",
            "finally", "for", "from", "global", "if", "import", "in", "is",
            "lambda", "None", "nonlocal", "not", "or", "pass", "raise",
            "return", "True", "try", "while", "with", "yield"
        };
    } else if (languageId == "javascript" || languageId == "js" ||
               languageId == "typescript" || languageId == "ts") {
        keywords_ = {
            "async", "await", "break", "case", "catch", "class", "const",
            "continue", "debugger", "default", "delete", "do", "else",
            "enum", "export", "extends", "false", "finally", "for", "from",
            "function", "if", "import", "in", "instanceof", "let", "new",
            "null", "return", "super", "switch", "this", "throw", "true",
            "try", "typeof", "var", "void", "while", "with", "yield"
        };
    } else if (languageId == "rust" || languageId == "rs") {
        keywords_ = {
            "as", "async", "await", "break", "const", "continue", "crate",
            "dyn", "else", "enum", "extern", "false", "fn", "for", "if",
            "impl", "in", "let", "loop", "match", "mod", "move", "mut",
            "pub", "ref", "return", "self", "Self", "static", "struct",
            "super", "trait", "true", "type", "unsafe", "use", "where",
            "while"
        };
    } else if (languageId == "go") {
        keywords_ = {
            "break", "case", "chan", "const", "continue", "default", "defer",
            "else", "fallthrough", "for", "func", "go", "goto", "if", "import",
            "interface", "map", "package", "range", "return", "select",
            "struct", "switch", "type", "var"
        };
    }
    
    // Add common snippets
    loadSnippets();
}

void CompletionProvider::loadSnippets() {
    snippets_ = {
        {"for", {"for (int ${1:i} = 0; ${1:i} < ${2:n}; ${1:i}++) {\n\t${0}\n}"}},
        {"if", {"if (${1:condition}) {\n\t${0}\n}"}},
        {"ife", {"if (${1:condition}) {\n\t${2}\n} else {\n\t${0}\n}"}},
        {"while", {"while (${1:condition}) {\n\t${0}\n}"}},
        {"func", {"void ${1:functionName}(${2:args}) {\n\t${0}\n}"}},
        {"main", {"int main(int argc, char* argv[]) {\n\t${0}\n\treturn 0;\n}"}},
        {"class", {"class ${1:ClassName} {\npublic:\n\t${1:ClassName}();\n\t~${1:ClassName}();\n\nprivate:\n\t${0}\n};"}},
        {"printf", {"printf(\"${1:format}\\n\"${2});"}},
        {"cout", {"std::cout << ${1:value} << std::endl;"}},
    };
}

void CompletionProvider::triggerCompletion(const std::string& documentUri,
                                            size_t line, size_t column,
                                            const std::string& prefix) {
    currentPrefix_ = prefix;
    
    // Debounce
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastTriggerTime_).count();
    
    if (elapsed < triggerCharDelay_) {
        // Debounce - schedule for later
        return;
    }
    
    lastTriggerTime_ = now;
    
    // Get completions from multiple sources
    std::vector<CompletionItem> items;
    
    // 1. Language keywords
    for (const auto& keyword : keywords_) {
        if (prefix.empty() || keyword.find(prefix) == 0) {
            CompletionItem item;
            item.label = keyword;
            item.kind = CompletionItemKind::Keyword;
            item.sortText = "1_" + keyword;
            items.push_back(item);
        }
    }
    
    // 2. Snippets
    for (const auto& [trigger, snippet] : snippets_) {
        if (prefix.empty() || trigger.find(prefix) == 0) {
            CompletionItem item;
            item.label = trigger;
            item.kind = CompletionItemKind::Snippet;
            item.insertText = snippet[0];
            item.sortText = "2_" + trigger;
            items.push_back(item);
        }
    }
    
    // 3. LSP completions
    if (lspClient_) {
        // Async LSP request would go here
    }
    
    // 4. Local symbols (from document)
    addLocalSymbols(documentUri, items, prefix);
    
    // Sort and deduplicate
    std::sort(items.begin(), items.end(), [](const CompletionItem& a, const CompletionItem& b) {
        return a.sortText < b.sortText;
    });
    
    items.erase(std::unique(items.begin(), items.end(),
        [](const CompletionItem& a, const CompletionItem& b) {
            return a.label == b.label;
        }), items.end());
    
    currentCompletions_ = items;
    
    if (onCompletionReady) {
        onCompletionReady(items);
    }
}

void CompletionProvider::addLocalSymbols(const std::string& documentUri,
                                          std::vector<CompletionItem>& items,
                                          const std::string& prefix) {
    // In production, parse the document and extract local symbols
    // For now, add some common patterns
    
    static std::set<std::string> commonSymbols = {
        "main", "printf", "cout", "cin", "endl", "string", "vector",
        "map", "set", "list", "array", "sort", "find", "begin", "end",
        "size", "length", "push_back", "insert", "erase", "clear"
    };
    
    for (const auto& symbol : commonSymbols) {
        if (prefix.empty() || symbol.find(prefix) == 0) {
            CompletionItem item;
            item.label = symbol;
            item.kind = CompletionItemKind::Function;
            item.sortText = "3_" + symbol;
            items.push_back(item);
        }
    }
}

const std::vector<CompletionItem>& CompletionProvider::getCompletions() const {
    return currentCompletions_;
}

void CompletionProvider::resolveCompletion(CompletionItem& item) {
    if (lspClient_ && item.data != nullptr) {
        // Request resolved completion from LSP
    }
}

void CompletionProvider::insertCompletion(CompletionItem& item, TextBuffer* buffer,
                                           size_t line, size_t column) {
    std::string insertText = item.insertText.empty() ? item.label : item.insertText;
    
    // Calculate range to replace
    size_t startCol = column - currentPrefix_.length();
    
    // Delete prefix
    if (!currentPrefix_.empty() && startCol < column) {
        buffer->removeText(line, startCol, line, column);
    }
    
    // Insert completion
    buffer->insertText(line, startCol, insertText);
}

std::vector<std::string> CompletionProvider::getTriggerCharacters() const {
    std::vector<std::string> chars = {".", ":", "->", "::", "@", "#"};
    
    if (lspClient_) {
        // Get trigger chars from LSP server capabilities
    }
    
    return chars;
}

bool CompletionProvider::shouldAutoTrigger(char c) const {
    auto triggers = getTriggerCharacters();
    return std::find(triggers.begin(), triggers.end(), std::string(1, c)) != triggers.end();
}

void CompletionProvider::cancel() {
    currentCompletions_.clear();
    currentPrefix_.clear();
}

bool CompletionProvider::hasCompletions() const {
    return !currentCompletions_.empty();
}

size_t CompletionProvider::getCompletionCount() const {
    return currentCompletions_.size();
}

} // namespace NexusForge::Editor
