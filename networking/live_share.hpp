// networking/live_share.hpp
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace NexusForge::Networking {

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
    size_t cursorPosition = 0;
    std::string currentFile;
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
