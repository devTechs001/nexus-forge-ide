// ai/features/ai_chat.hpp
#pragma once

#include "../models/model_manager.hpp"
#include <string>
#include <vector>
#include <functional>

namespace NexusForge::AI {

class AIChat {
public:
    explicit AIChat(ModelManager* modelManager);
    ~AIChat();

    // Conversation management
    void startNewConversation();
    void clearConversation();
    std::string getConversationId() const { return conversationId_; }

    // Sending messages
    void sendMessage(const std::string& message);
    void sendMessageWithCode(const std::string& message,
                             const std::string& code,
                             const std::string& language);
    void sendMessageWithContext(const std::string& message,
                                const std::vector<std::string>& filePaths);

    // Streaming
    void streamMessage(const std::string& message, StreamCallback callback);
    void cancelStream();

    // History
    const std::vector<ChatMessage>& getHistory() const { return history_; }

    // Events
    using ResponseCallback = std::function<void(const std::string&, bool)>;
    using ErrorCallback = std::function<void(const std::string&)>;

    void setResponseCallback(ResponseCallback callback) { onResponse_ = callback; }
    void setErrorCallback(ErrorCallback callback) { onError_ = callback; }

private:
    ModelManager* modelManager_;
    std::string conversationId_;
    std::vector<ChatMessage> history_;
    std::string currentResponse_;

    ResponseCallback onResponse_;
    ErrorCallback onError_;

    std::string buildSystemPrompt() const;
    std::string generateConversationId() const;
};

} // namespace NexusForge::AI
