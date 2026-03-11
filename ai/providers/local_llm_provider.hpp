// ai/providers/local_llm_provider.hpp
#pragma once

#include "../models/model_manager.hpp"
#include <string>
#include <vector>

namespace NexusForge::AI {

// Local LLM provider (Ollama, LM Studio, etc.)
class LocalLLMProvider : public IAIProvider {
public:
    LocalLLMProvider();
    ~LocalLLMProvider() override;

    bool initialize(const ModelConfig& config) override;
    void shutdown() override;

    std::future<std::vector<CompletionResult>> complete(const CompletionRequest& request) override;
    std::future<ChatResponse> chat(const ChatRequest& request) override;
    void chatStream(const ChatRequest& request, StreamCallback callback) override;
    std::future<CodeAnalysisResult> analyzeCode(const CodeAnalysisRequest& request) override;

    bool isReady() const override { return initialized_; }
    const ModelConfig& getConfig() const override { return config_; }

    // Local LLM-specific
    void setServerUrl(const std::string& url) { serverUrl_ = url; }
    void setModelPath(const std::string& path) { modelPath_ = path; }
    void setContextSize(size_t size) { contextSize_ = size; }
    void setGPULayers(int layers) { gpuLayers_ = layers; }

    // Model management
    bool loadModel();
    void unloadModel();
    bool isModelLoaded() const { return modelLoaded_; }

private:
    ModelConfig config_;
    std::string serverUrl_ = "http://localhost:11434";  // Ollama default
    std::string modelPath_;
    std::string model_;
    size_t contextSize_ = 4096;
    int gpuLayers_ = -1;  // Auto
    bool initialized_ = false;
    bool modelLoaded_ = false;

    std::string buildPrompt(const CompletionRequest& request);
    ChatResponse parseResponse(const std::string& json);
};

} // namespace NexusForge::AI
