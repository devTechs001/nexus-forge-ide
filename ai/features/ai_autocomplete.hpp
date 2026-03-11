// ai/features/ai_autocomplete.hpp
#pragma once

#include "../models/model_manager.hpp"
#include "../../editor/buffer/text_buffer.hpp"

#include <memory>
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <chrono>

namespace NexusForge::AI {

// Inline suggestion
struct InlineSuggestion {
    std::string text;
    std::string displayText;
    size_t startLine;
    size_t startColumn;
    size_t endLine;
    size_t endColumn;
    float confidence;
    std::string source;  // "ai", "snippet", "history"
    std::chrono::steady_clock::time_point timestamp;
};

// Context for completion
struct CompletionContext {
    // Document info
    std::string documentId;
    std::string filePath;
    std::string language;

    // Cursor position
    size_t cursorLine;
    size_t cursorColumn;

    // Surrounding code
    std::string prefix;       // Code before cursor
    std::string suffix;       // Code after cursor
    std::string currentLine;  // Full current line

    // Extended context
    std::vector<std::string> importedModules;
    std::vector<std::string> recentFiles;
    std::string projectRoot;

    // User preferences
    std::string preferredStyle;  // camelCase, snake_case, etc.
};

// Autocomplete engine
class AIAutocomplete {
public:
    AIAutocomplete(ModelManager* modelManager);
    ~AIAutocomplete();

    // Configuration
    struct Config {
        bool enabled = true;
        int debounceMs = 300;
        int maxSuggestions = 5;
        float minConfidence = 0.3f;
        int maxContextLines = 50;
        int maxSuggestionLength = 500;
        bool multiLine = true;
        std::vector<std::string> triggerCharacters = {".", ":", "(", "[", "{", " "};
        std::vector<std::string> disabledLanguages;
    };

    void setConfig(const Config& config) { config_ = config; }
    const Config& getConfig() const { return config_; }

    // Main API
    void requestCompletion(const CompletionContext& context);
    void cancelPendingRequests();

    // Results
    bool hasSuggestion() const { return !currentSuggestion_.text.empty(); }
    const InlineSuggestion& getCurrentSuggestion() const { return currentSuggestion_; }
    const std::vector<InlineSuggestion>& getAllSuggestions() const { return suggestions_; }

    // Navigation
    void nextSuggestion();
    void previousSuggestion();
    void acceptSuggestion();
    void acceptPartialSuggestion(size_t wordCount = 1);
    void dismissSuggestion();

    // Learning
    void recordAcceptance(const InlineSuggestion& suggestion);
    void recordRejection(const InlineSuggestion& suggestion);
    void recordPartialAcceptance(const InlineSuggestion& suggestion,
                                  const std::string& acceptedPart);

    // Events
    std::function<void(const InlineSuggestion&)> onSuggestionReady;
    std::function<void()> onSuggestionCleared;

private:
    ModelManager* modelManager_;
    Config config_;

    InlineSuggestion currentSuggestion_;
    std::vector<InlineSuggestion> suggestions_;
    size_t currentSuggestionIndex_ = 0;

    std::atomic<bool> requestPending_{false};
    std::atomic<uint64_t> requestId_{0};
    std::chrono::steady_clock::time_point lastRequestTime_;

    std::mutex suggestionMutex_;

    // Context building
    std::string buildPrompt(const CompletionContext& context);
    std::vector<std::string> gatherRelevantContext(const CompletionContext& context);

    // Post-processing
    std::vector<InlineSuggestion> processCompletions(
        const std::vector<CompletionResult>& results,
        const CompletionContext& context);

    std::string cleanupSuggestion(const std::string& suggestion,
                                   const CompletionContext& context);
    bool validateSuggestion(const std::string& suggestion,
                            const CompletionContext& context);
    float calculateConfidence(const CompletionResult& result,
                              const CompletionContext& context);
};

// AI Chat Assistant
class AIChatAssistant {
public:
    AIChatAssistant(ModelManager* modelManager);
    ~AIChatAssistant();

    // Conversation management
    void startNewConversation();
    void clearConversation();
    std::string getConversationId() const { return conversationId_; }

    // Messages
    void sendMessage(const std::string& message);
    void sendMessageWithCode(const std::string& message,
                             const std::string& code,
                             const std::string& language);
    void sendMessageWithContext(const std::string& message,
                                const std::vector<std::string>& filePaths);

    // Streaming
    void streamMessage(const std::string& message, StreamCallback callback);
    void cancelStream();

    // History
    const std::vector<ChatMessage>& getHistory() const { return history_; }
    void loadHistory(const std::vector<ChatMessage>& history);

    // Specialized commands
    void explainCode(const std::string& code, const std::string& language);
    void fixCode(const std::string& code, const std::string& language,
                 const std::string& error);
    void improveCode(const std::string& code, const std::string& language);
    void askAboutCode(const std::string& question,
                      const std::string& code,
                      const std::string& language);

    // Events
    std::function<void(const std::string& response, bool done)> onResponse;
    std::function<void(const std::string& error)> onError;

private:
    ModelManager* modelManager_;
    std::string conversationId_;
    std::vector<ChatMessage> history_;
    std::string systemPrompt_;
    std::atomic<bool> streaming_{false};

    void buildSystemPrompt();
    std::string formatCodeBlock(const std::string& code, const std::string& language);
};

} // namespace NexusForge::AI
