// ai/models/model_manager.hpp
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <future>
#include <optional>

namespace NexusForge::AI {

enum class AIProvider {
    OpenAI,
    Anthropic,
    Google,
    Mistral,
    LocalLLM,
    Custom
};

struct ModelCapabilities {
    bool codeCompletion = true;
    bool chat = true;
    bool codeExplanation = true;
    bool codeRefactoring = true;
    int maxContextLength = 8192;
    int maxOutputLength = 4096;
    bool supportsStreaming = true;
};

struct ModelConfig {
    std::string modelId;
    std::string displayName;
    AIProvider provider;
    std::string apiEndpoint;
    std::string apiKey;
    ModelCapabilities capabilities;
    float temperature = 0.7f;
    int maxTokens = 2048;
};

enum class MessageRole {
    System,
    User,
    Assistant,
    Function
};

struct ChatMessage {
    MessageRole role;
    std::string content;
    
    static ChatMessage system(const std::string& content) {
        return { MessageRole::System, content };
    }
    static ChatMessage user(const std::string& content) {
        return { MessageRole::User, content };
    }
    static ChatMessage assistant(const std::string& content) {
        return { MessageRole::Assistant, content };
    }
};

struct CompletionRequest {
    std::string prompt;
    std::string language;
    std::string filePath;
    int cursorLine;
    int cursorColumn;
    int maxTokens = 256;
    float temperature = 0.2f;
};

struct CompletionResult {
    std::string text;
    float confidence;
    std::string displayText;
};

struct ChatRequest {
    std::vector<ChatMessage> messages;
    std::string systemPrompt;
    float temperature = 0.7f;
    int maxTokens = 2048;
    bool stream = true;
};

struct ChatResponse {
    std::string content;
    std::string finishReason;
    int promptTokens;
    int completionTokens;
    int totalTokens;
};

using StreamCallback = std::function<void(const std::string& chunk, bool done)>;

class IAIProvider {
public:
    virtual ~IAIProvider() = default;
    virtual bool initialize(const ModelConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual std::future<std::vector<CompletionResult>> complete(const CompletionRequest& request) = 0;
    virtual std::future<ChatResponse> chat(const ChatRequest& request) = 0;
    virtual void chatStream(const ChatRequest& request, StreamCallback callback) = 0;
    virtual bool isReady() const = 0;
};

struct AISettings {
    bool enabled = true;
    std::string defaultProvider = "anthropic";
    std::string defaultModel = "claude-3-sonnet";
    std::unordered_map<std::string, std::string> apiKeys;
    bool enableInlineCompletion = true;
    bool enableChat = true;
    int completionDelay = 300;
    float completionTemperature = 0.2f;
    float chatTemperature = 0.7f;
};

class ModelManager {
public:
    ModelManager();
    ~ModelManager();

    bool initialize(const AISettings& settings);
    void shutdown();

    void registerProvider(AIProvider type, std::unique_ptr<IAIProvider> provider);
    IAIProvider* getProvider(AIProvider type);
    IAIProvider* getDefaultProvider();

    std::future<std::vector<CompletionResult>> getCompletions(const CompletionRequest& request);
    std::future<ChatResponse> chat(const ChatRequest& request);
    void chatStream(const ChatRequest& request, StreamCallback callback);

    std::future<std::string> explainCode(const std::string& code, const std::string& language);
    std::future<std::string> refactorCode(const std::string& code, const std::string& language,
                                          const std::string& instruction);

    void updateSettings(const AISettings& settings);
    const AISettings& getSettings() const { return settings_; }
    bool isReady() const;

private:
    AISettings settings_;
    std::unordered_map<AIProvider, std::unique_ptr<IAIProvider>> providers_;
    std::unordered_map<std::string, ModelConfig> loadedModels_;
    AIProvider defaultProvider_ = AIProvider::Anthropic;
    bool initialized_ = false;
};

} // namespace NexusForge::AI
