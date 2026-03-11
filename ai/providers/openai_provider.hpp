// ai/providers/openai_provider.hpp
#pragma once

#include "../models/model_manager.hpp"
#include <string>
#include <vector>

namespace NexusForge::AI {

// OpenAI provider implementation
class OpenAIProvider : public IAIProvider {
public:
    OpenAIProvider();
    ~OpenAIProvider() override;

    bool initialize(const ModelConfig& config) override;
    void shutdown() override;

    std::future<std::vector<CompletionResult>> complete(const CompletionRequest& request) override;
    std::future<ChatResponse> chat(const ChatRequest& request) override;
    void chatStream(const ChatRequest& request, StreamCallback callback) override;
    std::future<CodeAnalysisResult> analyzeCode(const CodeAnalysisRequest& request) override;

    bool isReady() const override { return initialized_ && !apiKey_.empty(); }
    const ModelConfig& getConfig() const override { return config_; }

    // OpenAI-specific
    void setApiKey(const std::string& key) { apiKey_ = key; }
    void setApiEndpoint(const std::string& url) { apiEndpoint_ = url; }
    void setOrganization(const std::string& org) { organization_ = org; }

    // Models
    std::vector<std::string> getAvailableModels() const;
    void setModel(const std::string& model) { model_ = model; }

private:
    ModelConfig config_;
    std::string apiKey_;
    std::string apiEndpoint_ = "https://api.openai.com/v1";
    std::string organization_;
    std::string model_ = "gpt-4";
    bool initialized_ = false;

    std::string buildCompletionPrompt(const CompletionRequest& request);
    std::vector<CompletionResult> parseCompletionResponse(const std::string& json);
    ChatResponse parseChatResponse(const std::string& json);
    CodeAnalysisResult parseAnalysisResponse(const std::string& json);
};

} // namespace NexusForge::AI
