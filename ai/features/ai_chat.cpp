// ai/features/ai_chat.cpp
// AI Chat Implementation

#include "ai_chat.hpp"
#include <sstream>
#include <chrono>
#include <algorithm>
#include <memory>

namespace NexusForge::AI {

AIChatAssistant::AIChatAssistant(ModelManager* modelManager) 
    : modelManager_(modelManager), streaming_(false) {
    buildSystemPrompt();
}

AIChatAssistant::~AIChatAssistant() = default;

void AIChatAssistant::startNewConversation() {
    conversationId_ = generateConversationId();
    history_.clear();
    buildSystemPrompt();
}

void AIChatAssistant::clearConversation() {
    history_.clear();
}

std::string AIChatAssistant::generateConversationId() const {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    return "conv_" + std::to_string(timestamp);
}

void AIChatAssistant::sendMessage(const std::string& message) {
    if (!modelManager_ || message.empty()) return;
    
    // Add user message to history
    ChatMessage userMsg = ChatMessage::user(message);
    history_.push_back(userMsg);
    
    // Build request
    ChatRequest request;
    request.messages = history_;
    request.systemPrompt = systemPrompt_;
    request.temperature = 0.7f;
    request.maxTokens = 2048;
    request.stream = true;
    
    // Send request
    streaming_ = true;
    auto accumulated = std::make_shared<std::string>();

    modelManager_->chatStream(request, [this, accumulated](const std::string& chunk, bool done) {
        *accumulated += chunk;
        if (onResponse) {
            onResponse(chunk, done);
        }

        if (done) {
            streaming_ = false;
            if (!accumulated->empty()) {
                history_.push_back(ChatMessage::assistant(*accumulated));
            }
        }
    });
}

void AIChatAssistant::sendMessageWithCode(const std::string& message,
                                           const std::string& code,
                                           const std::string& language) {
    std::stringstream formattedMessage;
    formattedMessage << message << "\n\n";
    formattedMessage << "```" << language << "\n";
    formattedMessage << code << "\n";
    formattedMessage << "```";
    
    sendMessage(formattedMessage.str());
}

void AIChatAssistant::sendMessageWithContext(const std::string& message,
                                              const std::vector<std::string>& filePaths) {
    std::stringstream contextMessage;
    contextMessage << "Context files:\n";
    
    for (const auto& path : filePaths) {
        contextMessage << "- " << path << "\n";
    }
    
    contextMessage << "\n" << message;
    
    sendMessage(contextMessage.str());
}

void AIChatAssistant::streamMessage(const std::string& message, StreamCallback callback) {
    if (!modelManager_ || message.empty() || !callback) return;
    
    ChatMessage userMsg = ChatMessage::user(message);
    history_.push_back(userMsg);
    
    ChatRequest request;
    request.messages = history_;
    request.systemPrompt = systemPrompt_;
    request.stream = true;
    
    streaming_ = true;
    auto accumulated = std::make_shared<std::string>();

    modelManager_->chatStream(request, [this, callback, accumulated](const std::string& chunk, bool done) {
        *accumulated += chunk;
        callback(chunk, done);

        if (done) {
            streaming_ = false;
            if (!accumulated->empty()) {
                history_.push_back(ChatMessage::assistant(*accumulated));
            }
        }
    });
}

void AIChatAssistant::cancelStream() {
    streaming_ = false;
    // In production, would cancel the ongoing request
}

void AIChatAssistant::explainCode(const std::string& code, const std::string& language) {
    std::stringstream message;
    message << "Please explain this code:\n\n";
    message << "```" << language << "\n";
    message << code << "\n";
    message << "```\n\n";
    message << "Explain:\n";
    message << "1. What does this code do?\n";
    message << "2. How does it work?\n";
    message << "3. Are there any potential issues?";
    
    sendMessage(message.str());
}

void AIChatAssistant::fixCode(const std::string& code, const std::string& language,
                               const std::string& error) {
    std::stringstream message;
    message << "Please fix this code that has an error:\n\n";
    message << "```" << language << "\n";
    message << code << "\n";
    message << "```\n\n";
    message << "Error: " << error << "\n\n";
    message << "Provide the corrected code with an explanation of the fix.";
    
    sendMessage(message.str());
}

void AIChatAssistant::improveCode(const std::string& code, const std::string& language) {
    std::stringstream message;
    message << "Please suggest improvements for this code:\n\n";
    message << "```" << language << "\n";
    message << code << "\n";
    message << "```\n\n";
    message << "Consider:\n";
    message << "- Performance optimizations\n";
    message << "- Code readability\n";
    message << "- Best practices\n";
    message << "- Potential bugs";
    
    sendMessage(message.str());
}

void AIChatAssistant::askAboutCode(const std::string& question,
                                    const std::string& code,
                                    const std::string& language) {
    std::stringstream message;
    message << "I have a question about this code:\n\n";
    message << "```" << language << "\n";
    message << code << "\n";
    message << "```\n\n";
    message << "Question: " << question;
    
    sendMessage(message.str());
}

void AIChatAssistant::buildSystemPrompt() {
    std::stringstream prompt;
    prompt << "You are an expert programming assistant integrated into NexusForge IDE.\n";
    prompt << "You help developers with:\n";
    prompt << "- Writing and explaining code\n";
    prompt << "- Debugging and fixing errors\n";
    prompt << "- Code review and improvements\n";
    prompt << "- Learning programming concepts\n\n";
    prompt << "Guidelines:\n";
    prompt << "- Provide clear, concise explanations\n";
    prompt << "- Include code examples when helpful\n";
    prompt << "- Point out potential issues or improvements\n";
    prompt << "- Adapt explanations to the user's skill level\n";
    prompt << "- Use proper formatting for code blocks\n\n";
    prompt << "You support multiple programming languages including:\n";
    prompt << "C++, C, Python, JavaScript, TypeScript, Rust, Go, Java, and more.";
    
    systemPrompt_ = prompt.str();
}

std::string AIChatAssistant::formatCodeBlock(const std::string& code, 
                                              const std::string& language) {
    return "```" + language + "\n" + code + "\n```";
}

const std::vector<ChatMessage>& AIChatAssistant::getHistory() const {
    return history_;
}

void AIChatAssistant::loadHistory(const std::vector<ChatMessage>& history) {
    history_ = history;
}

std::string AIChatAssistant::getConversationId() const {
    return conversationId_;
}

} // namespace NexusForge::AI
