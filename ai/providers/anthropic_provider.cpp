// ai/providers/anthropic_provider.cpp
// Anthropic Provider Implementation

#include "anthropic_provider.hpp"
#include <sstream>

namespace NexusForge::AI {

AnthropicProvider::AnthropicProvider() : initialized_(false) {
    capabilities_.codeCompletion = true;
    capabilities_.chat = true;
    capabilities_.codeExplanation = true;
    capabilities_.codeRefactoring = true;
    capabilities_.bugDetection = true;
    capabilities_.testGeneration = true;
    capabilities_.documentation = true;
    capabilities_.codeReview = true;
    capabilities_.maxContextLength = 200000;
    capabilities_.maxOutputLength = 4096;
    capabilities_.supportsStreaming = true;
}

AnthropicProvider::~AnthropicProvider() {
    shutdown();
}

bool AnthropicProvider::initialize(const ModelConfig& config) {
    config_ = config;
    
    if (config.apiKey.empty()) {
        return false;
    }
    
    apiEndpoint_ = config.apiEndpoint;
    if (apiEndpoint_.empty()) {
        apiEndpoint_ = "https://api.anthropic.com/v1";
    }
    
    model_ = config.modelId;
    if (model_.empty()) {
        model_ = "claude-3-sonnet-20240229";
    }
    
    initialized_ = true;
    return true;
}

void AnthropicProvider::shutdown() {
    initialized_ = false;
}

std::future<std::vector<CompletionResult>> AnthropicProvider::complete(const CompletionRequest& request) {
    auto promise = std::make_shared<std::promise<std::vector<CompletionResult>>>();
    
    if (!initialized_) {
        promise->set_value({});
        return promise->get_future();
    }
    
    // Build Claude completion request
    std::stringstream body;
    body << "{";
    body << "\"model\":\"" << model_ << "\",";
    body << "\"max_tokens\":" << request.maxTokens << ",";
    
    // Claude uses messages format
    body << "\"messages\":[";
    body << "{\"role\":\"user\",\"content\":\"" << escapeJson(request.prompt) << "\"}";
    body << "],";
    
    body << "\"temperature\":" << request.temperature << ",";
    body << "\"stream\":false";
    body << "}";
    
    // In production, would make HTTP request to Anthropic API
    CompletionResult result;
    result.text = "// Claude completion would go here";
    result.confidence = 0.85f;
    result.displayText = "claude-completion";
    
    promise->set_value({result});
    
    return promise->get_future();
}

std::future<ChatResponse> AnthropicProvider::chat(const ChatRequest& request) {
    auto promise = std::make_shared<std::promise<ChatResponse>>();
    
    if (!initialized_) {
        ChatResponse empty;
        promise->set_value(empty);
        return promise->get_future();
    }
    
    // Build Claude messages request
    std::stringstream body;
    body << "{";
    body << "\"model\":\"" << model_ << "\",";
    body << "\"max_tokens\":" << request.maxTokens << ",";
    body << "\"messages\":[";
    
    // Convert messages to Claude format
    std::string systemPrompt = request.systemPrompt;
    
    for (size_t i = 0; i < request.messages.size(); ++i) {
        if (i > 0) body << ",";
        
        // Skip system messages - they go in the system field
        if (request.messages[i].role == MessageRole::System) {
            systemPrompt = request.messages[i].content;
            continue;
        }
        
        std::string role = (request.messages[i].role == MessageRole::User) ? "user" : "assistant";
        body << "{\"role\":\"" << role << "\",\"content\":\"" 
             << escapeJson(request.messages[i].content) << "\"}";
    }
    
    body << "]";
    
    if (!systemPrompt.empty()) {
        body << ",\"system\":\"" << escapeJson(systemPrompt) << "\"";
    }
    
    body << ",\"temperature\":" << request.temperature << ",";
    body << "\"stream\":false";
    body << "}";
    
    // In production, would make HTTP request
    ChatResponse response;
    response.content = "Claude chat response would come from Anthropic API";
    response.totalTokens = 150;
    
    promise->set_value(response);
    
    return promise->get_future();
}

void AnthropicProvider::chatStream(const ChatRequest& request, StreamCallback callback) {
    if (!initialized_ || !callback) {
        callback("", true);
        return;
    }
    
    // In production, would make streaming HTTP request to Anthropic
    // Claude has excellent streaming support
    callback("// Claude streaming response placeholder", true);
}

std::future<CodeAnalysisResult> AnthropicProvider::analyzeCode(const CodeAnalysisRequest& request) {
    auto promise = std::make_shared<std::promise<CodeAnalysisResult>>();
    
    if (!initialized_) {
        CodeAnalysisResult empty;
        empty.confidence = 0;
        promise->set_value(empty);
        return promise->get_future();
    }
    
    // Claude is particularly good at code analysis
    std::string prompt = buildAnalysisPrompt(request);
    
    // In production, would make API call
    CodeAnalysisResult result;
    result.summary = "Claude code analysis summary";
    result.detailedAnalysis = "Detailed analysis from Claude would appear here";
    result.confidence = 0.9f;
    
    // Add suggestions
    result.suggestions.push_back("Consider using RAII for resource management");
    result.suggestions.push_back("Add error handling for edge cases");
    
    promise->set_value(result);
    
    return promise->get_future();
}

bool AnthropicProvider::isReady() const {
    return initialized_ && !config_.apiKey.empty();
}

const ModelConfig& AnthropicProvider::getConfig() const {
    return config_;
}

std::string AnthropicProvider::buildAnalysisPrompt(const CodeAnalysisRequest& request) {
    std::stringstream prompt;
    
    // Claude works well with clear instructions
    prompt << "You are an expert code reviewer. ";
    
    if (request.analysisType == "explain") {
        prompt << "Explain the following code clearly and concisely:\n\n";
    } else if (request.analysisType == "refactor") {
        prompt << "Review and suggest improvements for the following code:\n\n";
    } else if (request.analysisType == "bugs") {
        prompt << "Find potential bugs and security issues in the following code:\n\n";
    } else if (request.analysisType == "review") {
        prompt << "Provide a thorough code review of the following code:\n\n";
    } else if (request.analysisType == "optimize") {
        prompt << "Suggest performance optimizations for the following code:\n\n";
    }
    
    prompt << "```" << request.language << "\n";
    prompt << request.code;
    prompt << "\n```";
    
    return prompt.str();
}

std::string AnthropicProvider::escapeJson(const std::string& input) {
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
