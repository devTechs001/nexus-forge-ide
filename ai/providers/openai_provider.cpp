// ai/providers/openai_provider.cpp
// OpenAI Provider Implementation

#include "openai_provider.hpp"
#include <sstream>
#include <algorithm>
#include <cstdio>

namespace NexusForge::AI {

OpenAIProvider::OpenAIProvider() : initialized_(false) {
    capabilities_.codeCompletion = true;
    capabilities_.chat = true;
    capabilities_.codeExplanation = true;
    capabilities_.codeRefactoring = true;
    capabilities_.bugDetection = true;
    capabilities_.testGeneration = true;
    capabilities_.documentation = true;
    capabilities_.codeReview = true;
    capabilities_.maxContextLength = 128000;
    capabilities_.maxOutputLength = 4096;
    capabilities_.supportsStreaming = true;
}

OpenAIProvider::~OpenAIProvider() {
    shutdown();
}

bool OpenAIProvider::initialize(const ModelConfig& config) {
    config_ = config;
    
    if (config.apiKey.empty()) {
        return false;
    }
    
    apiEndpoint_ = config.apiEndpoint;
    if (apiEndpoint_.empty()) {
        apiEndpoint_ = "https://api.openai.com/v1";
    }
    
    model_ = config.modelId;
    if (model_.empty()) {
        model_ = "gpt-4";
    }
    
    initialized_ = true;
    return true;
}

void OpenAIProvider::shutdown() {
    initialized_ = false;
}

std::future<std::vector<CompletionResult>> OpenAIProvider::complete(const CompletionRequest& request) {
    auto promise = std::make_shared<std::promise<std::vector<CompletionResult>>>();
    
    if (!initialized_) {
        promise->set_value({});
        return promise->get_future();
    }
    
    // Build request body
    std::stringstream body;
    body << "{";
    body << "\"model\":\"" << model_ << "\",";
    body << "\"max_tokens\":" << request.maxTokens << ",";
    body << "\"temperature\":" << request.temperature << ",";
    
    // Build prompt from context
    std::string prompt = buildCompletionPrompt(request);
    body << "\"prompt\":\"" << escapeJson(prompt) << "\",";
    
    if (!request.stopSequences.empty()) {
        body << "\"stop\":[";
        for (size_t i = 0; i < request.stopSequences.size(); ++i) {
            if (i > 0) body << ",";
            body << "\"" << escapeJson(request.stopSequences[i]) << "\"";
        }
        body << "],";
    }
    
    body << "\"stream\":false";
    body << "}";
    
    // In production, would make HTTP request to OpenAI API
    // For now, return empty result
    CompletionResult result;
    result.text = "// AI completion would go here";
    result.confidence = 0.8f;
    result.displayText = "completion";
    
    promise->set_value({result});
    
    return promise->get_future();
}

std::future<ChatResponse> OpenAIProvider::chat(const ChatRequest& request) {
    auto promise = std::make_shared<std::promise<ChatResponse>>();
    
    if (!initialized_) {
        ChatResponse empty;
        promise->set_value(empty);
        return promise->get_future();
    }
    
    // Build request body
    std::stringstream body;
    body << "{";
    body << "\"model\":\"" << model_ << "\",";
    body << "\"max_tokens\":" << request.maxTokens << ",";
    body << "\"temperature\":" << request.temperature << ",";
    body << "\"messages\":[";
    
    if (!request.systemPrompt.empty()) {
        body << "{\"role\":\"system\",\"content\":\"" << escapeJson(request.systemPrompt) << "\"},";
    }
    
    for (size_t i = 0; i < request.messages.size(); ++i) {
        if (i > 0) body << ",";
        
        std::string role;
        switch (request.messages[i].role) {
            case MessageRole::System: role = "system"; break;
            case MessageRole::User: role = "user"; break;
            case MessageRole::Assistant: role = "assistant"; break;
            case MessageRole::Function: role = "function"; break;
        }
        
        body << "{\"role\":\"" << role << "\",\"content\":\"" 
             << escapeJson(request.messages[i].content) << "\"}";
    }
    
    body << "],\"stream\":false}";
    
    // In production, would make HTTP request
    ChatResponse response;
    response.content = "Chat response would come from OpenAI API";
    response.totalTokens = 100;
    
    promise->set_value(response);
    
    return promise->get_future();
}

void OpenAIProvider::chatStream(const ChatRequest& request, StreamCallback callback) {
    if (!callback) return;
    if (!initialized_) {
        callback("", true);
        return;
    }
    
    // In production, would make streaming HTTP request
    // For now, call callback with placeholder
    callback("// Streaming response placeholder", true);
}

std::future<CodeAnalysisResult> OpenAIProvider::analyzeCode(const CodeAnalysisRequest& request) {
    auto promise = std::make_shared<std::promise<CodeAnalysisResult>>();
    
    if (!initialized_) {
        CodeAnalysisResult empty;
        empty.confidence = 0;
        promise->set_value(empty);
        return promise->get_future();
    }
    
    // Build analysis prompt based on type
    std::string prompt = buildAnalysisPrompt(request);
    
    // In production, would make API call
    CodeAnalysisResult result;
    result.summary = "Code analysis summary";
    result.detailedAnalysis = "Detailed analysis would come from OpenAI";
    result.confidence = 0.85f;
    
    promise->set_value(result);
    
    return promise->get_future();
}

bool OpenAIProvider::isReady() const {
    return initialized_ && !config_.apiKey.empty();
}

const ModelConfig& OpenAIProvider::getConfig() const {
    return config_;
}

std::string OpenAIProvider::buildCompletionPrompt(const CompletionRequest& request) {
    std::stringstream prompt;
    
    // Add context from surrounding code
    prompt << "Complete the following code:\n\n";
    prompt << "```" << request.language << "\n";
    prompt << request.prompt;
    prompt << "\n```\n\n";
    
    if (!request.suffix.empty()) {
        prompt << "After the completion, the code continues:\n";
        prompt << "```" << request.language << "\n";
        prompt << request.suffix;
        prompt << "\n```\n\n";
    }
    
    prompt << "Provide only the code completion, no explanations:";
    
    return prompt.str();
}

std::string OpenAIProvider::buildAnalysisPrompt(const CodeAnalysisRequest& request) {
    std::stringstream prompt;
    
    std::string taskDescription;
    if (request.analysisType == "explain") {
        taskDescription = "Explain what this code does";
    } else if (request.analysisType == "refactor") {
        taskDescription = "Suggest improvements and refactor this code";
    } else if (request.analysisType == "review") {
        taskDescription = "Review this code for issues and best practices";
    } else if (request.analysisType == "bugs") {
        taskDescription = "Find potential bugs in this code";
    } else if (request.analysisType == "optimize") {
        taskDescription = "Suggest performance optimizations for this code";
    } else {
        taskDescription = "Analyze this code";
    }
    
    prompt << taskDescription << ":\n\n";
    prompt << "```" << request.language << "\n";
    prompt << request.code;
    prompt << "\n```";
    
    return prompt.str();
}

std::string OpenAIProvider::escapeJson(const std::string& input) {
    std::string output;
    output.reserve(input.length() * 2);
    
    for (char c : input) {
        switch (c) {
            case '"': output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\b': output += "\\b"; break;
            case '\f': output += "\\f"; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    output += buf;
                } else {
                    output += c;
                }
        }
    }
    
    return output;
}

} // namespace NexusForge::AI
