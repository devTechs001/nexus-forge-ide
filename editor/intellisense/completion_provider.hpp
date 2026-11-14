// editor/intellisense/completion_provider.hpp
#pragma once

#include "lsp_client.hpp"
#include "../buffer/text_buffer.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <chrono>

namespace NexusForge::Editor {

// Completion context
struct CompletionContext {
    std::string documentId;
    std::string filePath;
    std::string language;
    size_t line = 0;
    size_t column = 0;
    std::string prefix;
    std::string currentLine;
    char triggerCharacter = '\0';
    bool isTriggerCharacter = false;
};

// Completion result
struct CompletionResult {
    std::vector<CompletionItem> items;
    bool isIncomplete = false;
    size_t startLine = 0;
    size_t startColumn = 0;
    size_t endLine = 0;
    size_t endColumn = 0;
};

// Completion provider interface
class ICompletionProvider {
public:
    virtual ~ICompletionProvider() = default;
    
    virtual std::vector<CompletionItem> provideCompletions(const CompletionContext& context) = 0;
    virtual bool shouldTrigger(const CompletionContext& context) = 0;
    virtual int getPriority() const { return 0; }
    virtual std::string getName() const = 0;
};

// Keyword completion provider
class KeywordCompletionProvider : public ICompletionProvider {
public:
    void addKeyword(const std::string& keyword, const std::string& detail = "");
    void addKeywords(const std::vector<std::string>& keywords);
    void loadKeywordsForLanguage(const std::string& language);
    
    std::vector<CompletionItem> provideCompletions(const CompletionContext& context) override;
    bool shouldTrigger(const CompletionContext& context) override;
    int getPriority() const override { return 100; }
    std::string getName() const override { return "keyword"; }
    
private:
    std::unordered_map<std::string, std::string> keywords_;
};

// Snippet completion provider
class SnippetCompletionProvider : public ICompletionProvider {
public:
    struct Snippet {
        std::string prefix;
        std::string body;
        std::string description;
        std::string scope;
    };
    
    void addSnippet(const Snippet& snippet);
    void loadSnippetsForLanguage(const std::string& language);
    
    std::vector<CompletionItem> provideCompletions(const CompletionContext& context) override;
    bool shouldTrigger(const CompletionContext& context) override;
    int getPriority() const override { return 90; }
    std::string getName() const override { return "snippet"; }
    
private:
    std::vector<Snippet> snippets_;
};

// Word completion provider (based on document words)
class WordCompletionProvider : public ICompletionProvider {
public:
    void addWord(const std::string& word);
    void addWords(const std::vector<std::string>& words);
    void indexDocument(const std::string& documentId, const std::string& content);
    void removeDocument(const std::string& documentId);
    
    std::vector<CompletionItem> provideCompletions(const CompletionContext& context) override;
    bool shouldTrigger(const CompletionContext& context) override;
    int getPriority() const override { return 50; }
    std::string getName() const override { return "word"; }
    
private:
    std::unordered_map<std::string, std::string> documents_;
    std::unordered_map<std::string, int> wordFrequency_;
};

// Path completion provider (for file paths)
class PathCompletionProvider : public ICompletionProvider {
public:
    std::vector<CompletionItem> provideCompletions(const CompletionContext& context) override;
    bool shouldTrigger(const CompletionContext& context) override;
    int getPriority() const override { return 80; }
    std::string getName() const override { return "path"; }
    
private:
    std::vector<std::string> getDirectoryContents(const std::string& path) const;
};

// Completion manager
class CompletionManager {
public:
    CompletionManager();
    ~CompletionManager();
    
    // Provider management
    void registerProvider(std::shared_ptr<ICompletionProvider> provider);
    void unregisterProvider(const std::string& name);
    
    // Get completions
    CompletionResult getCompletions(const CompletionContext& context);
    
    // Trigger characters
    void setTriggerCharacters(const std::vector<char>& characters);
    const std::vector<char>& getTriggerCharacters() const { return triggerCharacters_; }
    
    // Configuration
    void setMaxCompletions(int max) { maxCompletions_ = max; }
    void setMinPrefixLength(int length) { minPrefixLength_ = length; }
    void setSortByRelevance(bool sort) { sortByRelevance_ = sort; }
    
private:
    std::vector<std::shared_ptr<ICompletionProvider>> providers_;
    std::vector<char> triggerCharacters_;
    int maxCompletions_ = 100;
    int minPrefixLength_ = 1;
    bool sortByRelevance_ = true;
    
    std::vector<CompletionItem> mergeAndSort(
        const std::vector<std::vector<CompletionItem>>& results,
        const CompletionContext& context);
};

// Completion item resolver
class CompletionResolver {
public:
    using ResolveCallback = std::function<void(CompletionItem)>;

    void resolve(CompletionItem& item, ResolveCallback callback);

private:
    // Resolve additional details for completion item
};

// Integrated CompletionProvider (LSP + language keywords + snippets)
class CompletionProvider {
public:
    CompletionProvider();
    ~CompletionProvider();

    void setLSPClient(LSPClient* client);
    void setLanguage(const std::string& languageId);
    void loadLanguageKeywords(const std::string& languageId);
    void loadSnippets();

    void triggerCompletion(const std::string& documentUri,
                           size_t line, size_t column,
                           const std::string& prefix);

    const std::vector<CompletionItem>& getCompletions() const;
    void resolveCompletion(CompletionItem& item);
    void insertCompletion(CompletionItem& item, TextBuffer* buffer,
                          size_t line, size_t column);

    std::vector<std::string> getTriggerCharacters() const;
    bool shouldAutoTrigger(char c) const;
    void cancel();
    bool hasCompletions() const;
    size_t getCompletionCount() const;

    std::function<void(const std::vector<CompletionItem>&)> onCompletionReady;

private:
    LSPClient* lspClient_ = nullptr;
    std::string languageId_;
    std::string currentPrefix_;
    int triggerCharDelay_ = 300;
    std::chrono::steady_clock::time_point lastTriggerTime_{};
    std::vector<std::string> keywords_;
    std::unordered_map<std::string, std::vector<std::string>> snippets_;
    std::vector<CompletionItem> currentCompletions_;

    void addLocalSymbols(const std::string& documentUri,
                         std::vector<CompletionItem>& items,
                         const std::string& prefix);
};

} // namespace NexusForge::Editor
