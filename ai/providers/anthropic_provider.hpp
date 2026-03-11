// ai/providers/anthropic_provider.hpp
#pragma once

#include "../models/model_manager.hpp"
#include <string>
#include <vector>

namespace NexusForge::AI {

// Anthropic (Claude) provider implementation
class AnthropicProvider : public IAIProvider {
public:
    AnthropicProvider();
    ~AnthropicProvider() override;

    bool initialize(const ModelConfig& config) override;
    void shutdown() override;

    std::future<std::vector<CompletionResult>> complete(const CompletionRequest& request) override;
    std::future<ChatResponse> chat(const ChatRequest& request) override;
    void chatStream(const ChatRequest& request, StreamCallback callback) override;
    std::future<CodeAnalysisResult> analyzeCode(const CodeAnalysisRequest& request) override;

    bool isReady() const override { return initialized_ && !apiKey_.empty(); }
    const ModelConfig& getConfig() const override { return config_; }

    // Anthropic-specific
    void setApiKey(const std::string& key) { apiKey_ = key; }
    void setApiEndpoint(const std::string& url) { apiEndpoint_ = url; }

    // Models
    void setModel(const std::string& model) { model_ = model; }

private:
    ModelConfig config_;
    std::string apiKey_;
    std::string apiEndpoint_ = "https://api.anthropic.com/v1";
    std::string model_ = "claude-3-sonnet-20240229";
    bool initialized_ = false;

    std::string buildChatPrompt(const ChatRequest& request);
    ChatResponse parseChatResponse(const std::string& json);
    CodeAnalysisResult parseAnalysisResponse(const std::string& json);
};

} // namespace NexusForge::AI
