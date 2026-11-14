// editor/intellisense/completion_provider.cpp
// Completion Provider Implementation

#include "completion_provider.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <set>
#include <sstream>

namespace NexusForge::Editor {

// KeywordCompletionProvider
void KeywordCompletionProvider::addKeyword(const std::string& keyword, const std::string& detail) {
    keywords_[keyword] = detail;
}

void KeywordCompletionProvider::addKeywords(const std::vector<std::string>& keywords) {
    for (const auto& kw : keywords) keywords_[kw] = "";
}

void KeywordCompletionProvider::loadKeywordsForLanguage(const std::string& language) {
    keywords_.clear();
    if (language == "cpp" || language == "c++") {
        addKeywords({"auto", "class", "const", "constexpr", "for", "if", "else",
                     "return", "struct", "template", "typename", "using", "virtual", "void", "while"});
    } else if (language == "python") {
        addKeywords({"def", "class", "if", "elif", "else", "for", "while", "import", "from", "return"});
    }
}

std::vector<CompletionItem> KeywordCompletionProvider::provideCompletions(const CompletionContext& context) {
    std::vector<CompletionItem> items;
    for (const auto& [kw, detail] : keywords_) {
        if (context.prefix.empty() || kw.find(context.prefix) == 0) {
            CompletionItem item;
            item.label = kw;
            item.kind = CompletionItemKind::Keyword;
            item.detail = detail;
            item.insertText = kw;
            items.push_back(std::move(item));
        }
    }
    return items;
}

bool KeywordCompletionProvider::shouldTrigger(const CompletionContext& context) {
    return !context.prefix.empty();
}

// SnippetCompletionProvider
void SnippetCompletionProvider::addSnippet(const Snippet& snippet) {
    snippets_.push_back(snippet);
}

void SnippetCompletionProvider::loadSnippetsForLanguage(const std::string& language) {
    (void)language;
}

std::vector<CompletionItem> SnippetCompletionProvider::provideCompletions(const CompletionContext& context) {
    std::vector<CompletionItem> items;
    for (const auto& s : snippets_) {
        if (context.prefix.empty() || s.prefix.find(context.prefix) == 0) {
            CompletionItem item;
            item.label = s.prefix;
            item.kind = CompletionItemKind::Snippet;
            item.detail = s.description;
            item.insertText = s.body;
            item.insertTextFormat = 2;
            items.push_back(std::move(item));
        }
    }
    return items;
}

bool SnippetCompletionProvider::shouldTrigger(const CompletionContext& context) {
    return !context.prefix.empty();
}

// WordCompletionProvider
void WordCompletionProvider::addWord(const std::string& word) {
    if (!word.empty()) wordFrequency_[word]++;
}

void WordCompletionProvider::addWords(const std::vector<std::string>& words) {
    for (const auto& w : words) addWord(w);
}

void WordCompletionProvider::indexDocument(const std::string& documentId, const std::string& content) {
    documents_[documentId] = content;
    std::string word;
    for (char c : content) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc) || uc == '_') {
            word.push_back(c);
        } else if (!word.empty()) {
            wordFrequency_[word]++;
            word.clear();
        }
    }
    if (!word.empty()) wordFrequency_[word]++;
}

void WordCompletionProvider::removeDocument(const std::string& documentId) {
    documents_.erase(documentId);
}

std::vector<CompletionItem> WordCompletionProvider::provideCompletions(const CompletionContext& context) {
    std::vector<CompletionItem> items;
    for (const auto& [word, freq] : wordFrequency_) {
        (void)freq;
        if (word != context.prefix && (context.prefix.empty() || word.find(context.prefix) == 0)) {
            CompletionItem item;
            item.label = word;
            item.kind = CompletionItemKind::Text;
            item.insertText = word;
            items.push_back(std::move(item));
        }
    }
    return items;
}

bool WordCompletionProvider::shouldTrigger(const CompletionContext& context) {
    return !context.prefix.empty();
}

// PathCompletionProvider
std::vector<CompletionItem> PathCompletionProvider::provideCompletions(const CompletionContext& context) {
    std::vector<CompletionItem> items;
    for (const auto& entry : getDirectoryContents(context.prefix)) {
        CompletionItem item;
        item.label = entry;
        item.kind = CompletionItemKind::File;
        item.insertText = entry;
        items.push_back(std::move(item));
    }
    return items;
}

bool PathCompletionProvider::shouldTrigger(const CompletionContext& context) {
    return context.triggerCharacter == '/' || context.prefix.find('/') != std::string::npos;
}

std::vector<std::string> PathCompletionProvider::getDirectoryContents(const std::string& path) const {
    std::vector<std::string> entries;
    std::error_code ec;
    if (path.empty() || !std::filesystem::is_directory(path, ec)) return entries;
    for (const auto& e : std::filesystem::directory_iterator(path, ec)) {
        entries.push_back(e.path().filename().string());
    }
    return entries;
}

// CompletionManager
CompletionManager::CompletionManager() = default;
CompletionManager::~CompletionManager() = default;

void CompletionManager::registerProvider(std::shared_ptr<ICompletionProvider> provider) {
    if (provider) providers_.push_back(std::move(provider));
}

void CompletionManager::unregisterProvider(const std::string& name) {
    providers_.erase(std::remove_if(providers_.begin(), providers_.end(),
        [&](const auto& p) { return p && p->getName() == name; }), providers_.end());
}

CompletionResult CompletionManager::getCompletions(const CompletionContext& context) {
    CompletionResult res;
    if (static_cast<int>(context.prefix.size()) < minPrefixLength_ && !context.isTriggerCharacter) {
        return res;
    }
    std::vector<std::vector<CompletionItem>> batches;
    for (const auto& p : providers_) {
        if (p && p->shouldTrigger(context)) {
            batches.push_back(p->provideCompletions(context));
        }
    }
    res.items = mergeAndSort(batches, context);
    if (maxCompletions_ > 0 && res.items.size() > static_cast<size_t>(maxCompletions_)) {
        res.items.resize(static_cast<size_t>(maxCompletions_));
        res.isIncomplete = true;
    }
    return res;
}

void CompletionManager::setTriggerCharacters(const std::vector<char>& characters) {
    triggerCharacters_ = characters;
}

std::vector<CompletionItem> CompletionManager::mergeAndSort(
    const std::vector<std::vector<CompletionItem>>& results,
    const CompletionContext& context) {
    (void)context;
    std::vector<CompletionItem> merged;
    for (const auto& batch : results) {
        merged.insert(merged.end(), batch.begin(), batch.end());
    }
    if (sortByRelevance_) {
        std::sort(merged.begin(), merged.end(), [](const CompletionItem& a, const CompletionItem& b) {
            return a.label < b.label;
        });
    }
    merged.erase(std::unique(merged.begin(), merged.end(),
        [](const CompletionItem& a, const CompletionItem& b) { return a.label == b.label; }),
        merged.end());
    return merged;
}

// CompletionResolver
void CompletionResolver::resolve(CompletionItem& item, ResolveCallback callback) {
    if (callback) callback(item);
}

// CompletionProvider
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
    
    // Debounce (skip on first trigger)
    auto now = std::chrono::steady_clock::now();
    if (lastTriggerTime_.time_since_epoch().count() != 0) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastTriggerTime_).count();

        if (elapsed < triggerCharDelay_) {
            // Debounce - schedule for later
            return;
        }
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
    if (!buffer) return;
    std::string insertText = item.insertText.empty() ? item.label : item.insertText;

    // Calculate range to replace safely without unsigned underflow
    size_t prefixLen = std::min(column, currentPrefix_.length());
    size_t startCol = column - prefixLen;

    // Delete prefix
    if (prefixLen > 0 && startCol < column) {
        buffer->removeRange(line, startCol, line, column);
    }

    // Insert completion
    buffer->insert(line, startCol, insertText);
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
