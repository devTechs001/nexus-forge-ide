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
    bool bugDetection = true;
    bool testGeneration = true;
    bool documentation = true;
    bool codeReview = true;
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
    std::string modelPath;
    ModelCapabilities capabilities;
    float temperature = 0.7f;
    int maxTokens = 2048;
    int contextSize = 4096;
    int threads = 0;
    int gpuLayers = -1;
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
    std::string suffix;
    std::string language;
    std::string filePath;
    int cursorLine;
    int cursorColumn;
    int maxTokens = 256;
    float temperature = 0.2f;
    std::vector<std::string> stopSequences;
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

struct CodeAnalysisRequest {
    std::string code;
    std::string language;
    std::string analysisType;
};

struct CodeAnalysisResult {
    std::string summary;
    std::string detailedAnalysis;
    std::optional<std::string> refactoredCode;
    std::vector<std::string> issues;
    std::vector<std::string> suggestions;
    float confidence = 0.0f;
};

class IAIProvider {
public:
    virtual ~IAIProvider() = default;
    virtual bool initialize(const ModelConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual std::future<std::vector<CompletionResult>> complete(const CompletionRequest& request) = 0;
    virtual std::future<ChatResponse> chat(const ChatRequest& request) = 0;
    virtual void chatStream(const ChatRequest& request, StreamCallback callback) = 0;
    virtual std::future<CodeAnalysisResult> analyzeCode(const CodeAnalysisRequest& request) = 0;
    virtual const ModelConfig& getConfig() const = 0;
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
    IAIProvider* getProvider(AIProvider type) const;
    IAIProvider* getDefaultProvider() const;
    void setDefaultProvider(AIProvider type);

    void loadModel(const ModelConfig& config);
    void unloadModel(const std::string& modelId);
    std::vector<std::string> getLoadedModels() const;
    ModelConfig* getModelConfig(const std::string& modelId);

    std::future<std::vector<CompletionResult>> getCompletions(const CompletionRequest& request);
    std::future<ChatResponse> chat(const ChatRequest& request);
    void chatStream(const ChatRequest& request, StreamCallback callback);

    std::future<CodeAnalysisResult> analyzeCode(const CodeAnalysisRequest& request);

    std::future<std::string> explainCode(const std::string& code, const std::string& language);
    std::future<std::string> refactorCode(const std::string& code, const std::string& language,
                                          const std::string& instruction);
    std::future<std::string> generateTests(const std::string& code, const std::string& language,
                                           const std::string& framework);
    std::future<std::string> generateDocumentation(const std::string& code, const std::string& language);
    std::future<std::vector<std::string>> findBugs(const std::string& code, const std::string& language);
    std::future<std::string> translateCode(const std::string& code, const std::string& fromLanguage,
                                           const std::string& toLanguage);

    std::future<std::vector<float>> getEmbedding(const std::string& text);
    std::future<std::vector<std::pair<std::string, float>>> semanticSearch(
        const std::string& query, const std::vector<std::string>& documents, int topK);

    void updateSettings(const AISettings& settings);
    const AISettings& getSettings() const { return settings_; }
    bool isReady() const;
    std::string getStatus() const;

private:
    IAIProvider* selectProvider(const std::string& capability);
    std::string buildSystemPrompt(const std::string& task);
    AISettings settings_;
    std::unordered_map<AIProvider, std::unique_ptr<IAIProvider>> providers_;
    std::unordered_map<std::string, ModelConfig> loadedModels_;
    AIProvider defaultProvider_ = AIProvider::Anthropic;
    bool initialized_ = false;
};

} // namespace NexusForge::AI
