// ai/features/ai_chat.hpp
#pragma once

#include "../models/model_manager.hpp"
#include <string>
#include <vector>
#include <functional>

namespace NexusForge::AI {

class AIChatAssistant {
public:
    explicit AIChatAssistant(ModelManager* modelManager);
    ~AIChatAssistant();

    void startNewConversation();
    void clearConversation();
    std::string getConversationId() const;

    void sendMessage(const std::string& message);
    void sendMessageWithCode(const std::string& message,
                             const std::string& code,
                             const std::string& language);
    void sendMessageWithContext(const std::string& message,
                                const std::vector<std::string>& filePaths);

    void streamMessage(const std::string& message, StreamCallback callback);
    void cancelStream();

    const std::vector<ChatMessage>& getHistory() const;
    void loadHistory(const std::vector<ChatMessage>& history);

    void explainCode(const std::string& code, const std::string& language);
    void fixCode(const std::string& code, const std::string& language,
                 const std::string& error);
    void improveCode(const std::string& code, const std::string& language);
    void askAboutCode(const std::string& question,
                      const std::string& code,
                      const std::string& language);

    std::function<void(const std::string& response, bool done)> onResponse;
    std::function<void(const std::string& error)> onError;

private:
    ModelManager* modelManager_;
    std::string conversationId_;
    std::vector<ChatMessage> history_;
    std::string systemPrompt_;
    std::atomic<bool> streaming_{false};

    bool isStreaming() const { return streaming_; }
    void buildSystemPrompt();
    std::string generateConversationId() const;
    std::string formatCodeBlock(const std::string& code, const std::string& language);
};

} // namespace NexusForge::AI
