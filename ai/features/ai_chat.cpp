// ai/features/ai_chat.cpp
#include "ai_chat.hpp"

namespace NexusForge::AI {

AIChat::AIChat(ModelManager* modelManager) : modelManager_(modelManager) {}

AIChat::~AIChat() = default;

void AIChat::startNewConversation() {
    conversationId_ = generateConversationId();
    history_.clear();
}

void AIChat::clearConversation() {
    history_.clear();
}

void AIChat::sendMessage(const std::string& message) {
    ChatMessage userMsg = ChatMessage::user(message);
    history_.push_back(userMsg);
    
    ChatRequest request;
    request.messages = history_;
    request.systemPrompt = buildSystemPrompt();
    
    auto future = modelManager_->chat(request);
    
    // Handle response asynchronously
    future.then([this](std::future<ChatResponse> f) {
        try {
            auto response = f.get();
            history_.push_back(ChatMessage::assistant(response.content));
            if (onResponse_) {
                onResponse_(response.content, true);
            }
        } catch (const std::exception& e) {
            if (onError_) {
                onError_(e.what());
            }
        }
    });
}

void AIChat::sendMessageWithCode(const std::string& message,
                                  const std::string& code,
                                  const std::string& language) {
    std::string formattedMessage = message + "\n\n```" + language + "\n" + code + "\n```";
    sendMessage(formattedMessage);
}

void AIChat::streamMessage(const std::string& message, StreamCallback callback) {
    ChatMessage userMsg = ChatMessage::user(message);
    history_.push_back(userMsg);
    
    ChatRequest request;
    request.messages = history_;
    request.systemPrompt = buildSystemPrompt();
    request.stream = true;
    
    modelManager_->chatStream(request, [this, callback](const std::string& chunk, bool done) {
        if (done) {
            history_.push_back(ChatMessage::assistant(currentResponse_));
        }
        callback(chunk, done);
    });
}

std::string AIChat::buildSystemPrompt() const {
    return "You are an AI programming assistant integrated into NexusForge IDE. "
           "You help users with coding tasks, explain code, debug issues, and provide "
           "best practices. Be concise but thorough. Format code with markdown code blocks.";
}

std::string AIChat::generateConversationId() const {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    return "conv_" + std::to_string(timestamp);
}

} // namespace NexusForge::AI
