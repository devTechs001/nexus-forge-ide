// ai/models/model_manager.cpp
#include "model_manager.hpp"
#include <algorithm>
#include <iostream>

namespace NexusForge::AI {

ModelManager::ModelManager() = default;

ModelManager::~ModelManager() {
    shutdown();
}

bool ModelManager::initialize(const AISettings& settings) {
    settings_ = settings;
    
    if (!settings.enabled) {
        return true;  // AI disabled, but initialization successful
    }

    // Initialize default providers based on settings
    if (settings.defaultProvider == "openai") {
        defaultProvider_ = AIProvider::OpenAI;
    } else if (settings.defaultProvider == "anthropic") {
        defaultProvider_ = AIProvider::Anthropic;
    } else if (settings.defaultProvider == "local") {
        defaultProvider_ = AIProvider::LocalLLM;
    } else {
        defaultProvider_ = AIProvider::Anthropic;
    }

    initialized_ = true;
    return true;
}

void ModelManager::shutdown() {
    for (auto& [type, provider] : providers_) {
        if (provider) {
            provider->shutdown();
        }
    }
    providers_.clear();
    loadedModels_.clear();
    initialized_ = false;
}

void ModelManager::registerProvider(AIProvider type, std::unique_ptr<IAIProvider> provider) {
    if (provider && provider->initialize({})) {
        providers_[type] = std::move(provider);
    }
}

IAIProvider* ModelManager::getProvider(AIProvider type) {
    auto it = providers_.find(type);
    if (it != providers_.end()) {
        return it->second.get();
    }
    return nullptr;
}

IAIProvider* ModelManager::getDefaultProvider() {
    return getProvider(defaultProvider_);
}

void ModelManager::setDefaultProvider(AIProvider type) {
    defaultProvider_ = type;
}

void ModelManager::loadModel(const ModelConfig& config) {
    loadedModels_[config.modelId] = config;
}

void ModelManager::unloadModel(const std::string& modelId) {
    loadedModels_.erase(modelId);
}

std::vector<std::string> ModelManager::getLoadedModels() const {
    std::vector<std::string> models;
    for (const auto& [id, config] : loadedModels_) {
        models.push_back(id);
    }
    return models;
}

ModelConfig* ModelManager::getModelConfig(const std::string& modelId) {
    auto it = loadedModels_.find(modelId);
    if (it != loadedModels_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::future<std::vector<CompletionResult>> ModelManager::getCompletions(
    const CompletionRequest& request) {
    
    auto provider = selectProvider("completion");
    if (provider && provider->isReady()) {
        return provider->complete(request);
    }
    
    // Return empty result
    std::promise<std::vector<CompletionResult>> promise;
    promise.set_value({});
    return promise.get_future();
}

std::future<ChatResponse> ModelManager::chat(const ChatRequest& request) {
    auto provider = selectProvider("chat");
    if (provider && provider->isReady()) {
        return provider->chat(request);
    }
    
    std::promise<ChatResponse> promise;
    promise.set_value(ChatResponse{});
    return promise.get_future();
}

void ModelManager::chatStream(const ChatRequest& request, StreamCallback callback) {
    auto provider = selectProvider("chat");
    if (provider && provider->isReady()) {
        provider->chatStream(request, callback);
    }
}

std::future<CodeAnalysisResult> ModelManager::analyzeCode(
    const CodeAnalysisRequest& request) {
    
    auto provider = selectProvider("analysis");
    if (provider && provider->isReady()) {
        return provider->analyzeCode(request);
    }
    
    std::promise<CodeAnalysisResult> promise;
    promise.set_value(CodeAnalysisResult{});
    return promise.get_future();
}

std::future<std::string> ModelManager::explainCode(
    const std::string& code, const std::string& language) {
    
    CodeAnalysisRequest request;
    request.code = code;
    request.language = language;
    request.analysisType = "explain";
    
    auto future = analyzeCode(request);
    
    // Transform to string result
    auto promise = std::make_shared<std::promise<std::string>>();
    
    future.then([promise](std::future<CodeAnalysisResult> f) {
        try {
            auto result = f.get();
            promise->set_value(result.detailedAnalysis);
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    });
    
    return promise->get_future();
}

std::future<std::string> ModelManager::refactorCode(
    const std::string& code, const std::string& language,
    const std::string& instruction) {
    
    CodeAnalysisRequest request;
    request.code = code;
    request.language = language;
    request.analysisType = "refactor";
    
    auto future = analyzeCode(request);
    
    auto promise = std::make_shared<std::promise<std::string>>();
    
    future.then([promise](std::future<CodeAnalysisResult> f) {
        try {
            auto result = f.get();
            if (result.refactoredCode) {
                promise->set_value(*result.refactoredCode);
            } else {
                promise->set_value("");
            }
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    });
    
    return promise->get_future();
}

std::future<std::string> ModelManager::generateTests(
    const std::string& code, const std::string& language,
    const std::string& framework) {
    
    ChatRequest request;
    request.messages.push_back(ChatMessage::system(
        "You are an expert test generator. Generate comprehensive unit tests."
    ));
    request.messages.push_back(ChatMessage::user(
        "Generate " + framework + " tests for this " + language + " code:\n\n" + code
    ));
    
    auto future = chat(request);
    
    auto promise = std::make_shared<std::promise<std::string>>();
    
    future.then([promise](std::future<ChatResponse> f) {
        try {
            auto result = f.get();
            promise->set_value(result.content);
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    });
    
    return promise->get_future();
}

std::future<std::string> ModelManager::generateDocumentation(
    const std::string& code, const std::string& language) {
    
    ChatRequest request;
    request.messages.push_back(ChatMessage::system(
        "You are an expert technical writer. Generate clear, concise documentation."
    ));
    request.messages.push_back(ChatMessage::user(
        "Generate documentation for this " + language + " code:\n\n" + code
    ));
    
    auto future = chat(request);
    
    auto promise = std::make_shared<std::promise<std::string>>();
    
    future.then([promise](std::future<ChatResponse> f) {
        try {
            auto result = f.get();
            promise->set_value(result.content);
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    });
    
    return promise->get_future();
}

std::future<std::vector<std::string>> ModelManager::findBugs(
    const std::string& code, const std::string& language) {
    
    CodeAnalysisRequest request;
    request.code = code;
    request.language = language;
    request.analysisType = "bugs";
    
    auto future = analyzeCode(request);
    
    auto promise = std::make_shared<std::promise<std::vector<std::string>>>();
    
    future.then([promise](std::future<CodeAnalysisResult> f) {
        try {
            auto result = f.get();
            promise->set_value(result.issues);
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    });
    
    return promise->get_future();
}

std::future<std::string> ModelManager::translateCode(
    const std::string& code, const std::string& fromLanguage,
    const std::string& toLanguage) {
    
    ChatRequest request;
    request.messages.push_back(ChatMessage::system(
        "You are an expert code translator. Translate code between languages while preserving functionality."
    ));
    request.messages.push_back(ChatMessage::user(
        "Translate this " + fromLanguage + " code to " + toLanguage + ":\n\n" + code
    ));
    
    auto future = chat(request);
    
    auto promise = std::make_shared<std::promise<std::string>>();
    
    future.then([promise](std::future<ChatResponse> f) {
        try {
            auto result = f.get();
            promise->set_value(result.content);
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    });
    
    return promise->get_future();
}

std::future<std::vector<float>> ModelManager::getEmbedding(const std::string& text) {
    // Embedding implementation
    std::promise<std::vector<float>> promise;
    promise.set_value({});
    return promise.get_future();
}

std::future<std::vector<std::pair<std::string, float>>> ModelManager::semanticSearch(
    const std::string& query, const std::vector<std::string>& documents, int topK) {
    
    // Semantic search implementation
    std::promise<std::vector<std::pair<std::string, float>>> promise;
    promise.set_value({});
    return promise.get_future();
}

void ModelManager::updateSettings(const AISettings& settings) {
    settings_ = settings;
}

bool ModelManager::isReady() const {
    if (!initialized_ || !settings_.enabled) return false;
    
    auto provider = getDefaultProvider();
    return provider && provider->isReady();
}

std::string ModelManager::getStatus() const {
    if (!initialized_) return "Not initialized";
    if (!settings_.enabled) return "AI disabled";
    
    auto provider = getDefaultProvider();
    if (!provider) return "No provider configured";
    if (!provider->isReady()) return "Provider not ready";
    
    return "Ready";
}

IAIProvider* ModelManager::selectProvider(const std::string& capability) {
    // Select provider based on capability
    return getDefaultProvider();
}

std::string ModelManager::buildSystemPrompt(const std::string& task) {
    return "You are an AI assistant integrated into NexusForge IDE. "
           "Help the user with " + task + ". "
           "Be concise and provide accurate, helpful responses.";
}

} // namespace NexusForge::AI
