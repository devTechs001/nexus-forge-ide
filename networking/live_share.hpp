// networking/live_share.hpp
#pragma once

#include "websocket_client.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <chrono>
#include <cstdint>

namespace NexusForge::Networking {

enum class TextChangeType {
    Insert,
    Delete,
    Replace
};

struct TextChange {
    TextChangeType type = TextChangeType::Insert;
    size_t position = 0;
    size_t length = 0;
    std::string text;
};

struct ChatMessage {
    std::string senderId;
    std::string senderName;
    std::string content;
    std::chrono::steady_clock::time_point timestamp{};
};

struct ParticipantInvite {
    std::string email;
    std::string code;
    std::chrono::steady_clock::time_point sentAt{};
};

// Collaboration session
struct CollaborationSession {
    std::string id;
    std::string host;
    std::string name;
    std::string description;
    std::vector<std::string> participants;
    bool readOnly = false;
    std::string password;
    uint64_t createdTime = 0;
};

// Participant
struct Participant {
    std::string id;
    std::string name;
    std::string color;
    std::string avatar;
    bool isHost = false;
    bool readOnly = false;
    size_t cursorPosition = 0;
    std::string currentFile;
};

// Live share controller
class LiveShare {
public:
    struct SharedDocument {
        std::string id;
        std::string path;
        std::string content;
        int version = 0;
    };

    struct SharedTerminal {
        std::string id;
        std::string output;
    };

    struct Participant {
        std::string id;
        std::string name;
        bool readOnly = false;
        std::function<void(const SharedDocument&)> onDocumentShared;
        std::function<void(const std::string&)> onDocumentUnshared;
        std::function<void(const std::string&, const TextChange&, int)> onDocumentChange;
        std::function<void(const ChatMessage&)> onChatMessage;
        std::function<void(const std::string&)> onParticipantRemoved;
        std::function<void()> onSessionEnded;
        std::function<void(const SharedTerminal&)> onTerminalShared;
        std::function<void(const std::string&, const std::string&)> onTerminalOutput;
    };

    LiveShare();
    ~LiveShare();

    bool hostSession(const std::string& workspacePath);
    bool joinSession(const std::string& sessionId, const std::string& inviteCode = "");
    void disconnect();

    bool shareDocument(const std::string& documentId, const std::string& path);
    void unshareDocument(const std::string& documentId);
    void updateDocument(const std::string& documentId, const TextChange& change);
    void requestDocument(const std::string& documentId);

    bool inviteParticipant(const std::string& email);
    bool kickParticipant(const std::string& participantId);
    void setReadOnly(const std::string& participantId, bool readOnly);
    void followParticipant(const std::string& participantId);
    void stopFollowing();

    void sendChatMessage(const std::string& message);
    const std::vector<ChatMessage>& getChatHistory() const;
    const std::unordered_map<std::string, Participant>& getParticipants() const;
    const std::unordered_map<std::string, SharedDocument>& getSharedDocuments() const;

    std::string getSessionId() const;
    bool isConnected() const;
    bool isHost() const;

    // Terminal sharing
    void shareTerminal(const std::string& terminalId);
    void sendTerminalOutput(const std::string& terminalId, const std::string& output);
    void sendTerminalInput(const std::string& terminalId, const std::string& input);

    // Events
    std::function<void(const std::string&)> onSessionStarted;
    std::function<void(const std::string&)> onSessionJoined;
    std::function<void()> onSessionEnded;
    std::function<void(const std::string&, const std::string&)> onInviteSent;
    std::function<void(const std::string&)> onFollowChanged;

private:
    bool connected_ = false;
    bool isHost_ = false;
    std::string workspacePath_;
    std::string sessionId_;
    std::string inviteCode_;
    std::string following_;
    std::string localParticipantId_;
    std::string localParticipantName_;

    std::unordered_map<std::string, Participant> participants_;
    std::unordered_map<std::string, SharedDocument> sharedDocuments_;
    std::unordered_map<std::string, SharedTerminal> sharedTerminals_;
    std::vector<ParticipantInvite> invites_;
    std::vector<ChatMessage> chatHistory_;

    std::string generateSessionId() const;
    std::string generateInviteCode() const;
    std::string loadDocumentContent(const std::string& path);
    void applyChange(std::string& content, const TextChange& change);
    void broadcastDocumentShared(const SharedDocument& doc);
    void broadcastDocumentUnshared(const std::string& documentId);
    void broadcastDocumentChange(const std::string& documentId, const TextChange& change, int version);
    void broadcastChatMessage(const ChatMessage& message);
    void broadcastParticipantRemoved(const std::string& participantId);
    void broadcastSessionEnd();
    void sendDocumentContent(const SharedDocument& doc);
    void sendDocumentRequest(const std::string& documentId);
    void sendReadOnlyUpdate(const std::string& participantId, bool readOnly);
    void broadcastTerminalShared(const SharedTerminal& terminal);
    void broadcastTerminalOutput(const std::string& terminalId, const std::string& output);
    void broadcastTerminalInput(const std::string& terminalId, const std::string& input);
};

// Live share session
class LiveShareSession {
public:
    LiveShareSession();
    ~LiveShareSession();

    // Host
    bool startHosting(const std::string& sessionName);
    void stopHosting();
    bool isHosting() const { return hosting_; }

    // Join
    bool joinSession(const std::string& sessionId, const std::string& password = "");
    void leaveSession();
    bool isJoined() const { return joined_; }

    // Participants
    const std::vector<Participant>& getParticipants() const { return participants_; }
    Participant* getParticipant(const std::string& id);
    Participant* getHost();

    // Document sync
    void shareDocument(const std::string& path, const std::string& content);
    void unshareDocument(const std::string& path);
    void updateDocument(const std::string& path, const std::string& content);

    // Cursor sharing
    void updateCursor(size_t position);
    void broadcastCursor(const std::string& participantId, size_t position);

    // Chat
    void sendMessage(const std::string& message);
    void sendSystemMessage(const std::string& message);

    // Events
    using ParticipantCallback = std::function<void(const Participant&)>;
    using MessageCallback = std::function<void(const std::string&, const std::string&)>;
    using DocumentCallback = std::function<void(const std::string&, const std::string&)>;

    void onParticipantJoined(ParticipantCallback callback);
    void onParticipantLeft(ParticipantCallback callback);
    void onMessageReceived(MessageCallback callback);
    void onDocumentShared(DocumentCallback callback);
    void onDocumentUpdated(DocumentCallback callback);

private:
    bool hosting_ = false;
    bool joined_ = false;
    std::string sessionId_;
    std::vector<Participant> participants_;

    WebSocketClient* connection_ = nullptr;

    void handleIncomingMessage(const std::string& message);
    void sendOperation(const std::string& type, const std::string& data);
};

// Live share server
class LiveShareServer {
public:
    LiveShareServer();
    ~LiveShareServer();

    bool start(int port);
    void stop();
    bool isRunning() const;

    // Sessions
    std::vector<CollaborationSession> getActiveSessions() const;
    CollaborationSession* getSession(const std::string& id);
    CollaborationSession* createSession(const std::string& name,
                                         const std::string& host,
                                         const std::string& description = "");
    void closeSession(const std::string& id);

private:
    bool running_ = false;
    int port_ = 0;
    std::vector<CollaborationSession> sessions_;
};

} // namespace NexusForge::Networking
