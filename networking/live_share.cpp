// networking/live_share.cpp
// Live Share Implementation

#include "live_share.hpp"
#include <algorithm>
#include <sstream>

namespace NexusForge::Networking {

LiveShare::LiveShare() : connected_(false), isHost_(false) {
}

LiveShare::~LiveShare() {
    disconnect();
}

bool LiveShare::hostSession(const std::string& workspacePath) {
    if (connected_) {
        disconnect();
    }
    
    workspacePath_ = workspacePath;
    isHost_ = true;
    
    // Generate session ID
    sessionId_ = generateSessionId();
    
    // In production, would connect to Live Share relay server
    // and create a new session
    
    connected_ = true;
    
    if (onSessionStarted) {
        onSessionStarted(sessionId_);
    }
    
    return true;
}

bool LiveShare::joinSession(const std::string& sessionId, const std::string& inviteCode) {
    if (connected_) {
        disconnect();
    }
    
    sessionId_ = sessionId;
    inviteCode_ = inviteCode;
    isHost_ = false;
    
    // In production, would connect to relay server
    // and join existing session
    
    connected_ = true;
    
    if (onSessionJoined) {
        onSessionJoined(sessionId_);
    }
    
    return true;
}

void LiveShare::disconnect() {
    if (!connected_) return;
    
    // Notify participants
    if (isHost_) {
        broadcastSessionEnd();
    }
    
    // Close connections
    participants_.clear();
    sharedDocuments_.clear();
    
    connected_ = false;
    
    if (onSessionEnded) {
        onSessionEnded();
    }
}

bool LiveShare::shareDocument(const std::string& documentId, const std::string& path) {
    if (!connected_) return false;
    
    SharedDocument doc;
    doc.id = documentId;
    doc.path = path;
    doc.content = loadDocumentContent(path);
    doc.version = 0;
    
    sharedDocuments_[documentId] = doc;
    
    // Notify participants
    broadcastDocumentShared(doc);
    
    return true;
}

void LiveShare::unshareDocument(const std::string& documentId) {
    auto it = sharedDocuments_.find(documentId);
    if (it == sharedDocuments_.end()) return;
    
    sharedDocuments_.erase(it);
    
    // Notify participants
    broadcastDocumentUnshared(documentId);
}

void LiveShare::updateDocument(const std::string& documentId, 
                                const TextChange& change) {
    auto it = sharedDocuments_.find(documentId);
    if (it == sharedDocuments_.end()) return;
    
    SharedDocument& doc = it->second;
    doc.version++;
    
    // Apply change locally
    applyChange(doc.content, change);
    
    // Broadcast change to participants
    broadcastDocumentChange(documentId, change, doc.version);
}

void LiveShare::requestDocument(const std::string& documentId) {
    if (isHost_) {
        auto it = sharedDocuments_.find(documentId);
        if (it != sharedDocuments_.end()) {
            sendDocumentContent(it->second);
        }
    } else {
        // Request from host
        sendDocumentRequest(documentId);
    }
}

bool LiveShare::inviteParticipant(const std::string& email) {
    if (!isHost_) return false;
    
    std::string inviteCode = generateInviteCode();
    
    ParticipantInvite invite;
    invite.email = email;
    invite.code = inviteCode;
    invite.sentAt = std::chrono::steady_clock::now();
    
    invites_.push_back(invite);
    
    // In production, would send email invitation
    
    if (onInviteSent) {
        onInviteSent(email, inviteCode);
    }
    
    return true;
}

bool LiveShare::kickParticipant(const std::string& participantId) {
    if (!isHost_) return false;
    
    auto it = participants_.find(participantId);
    if (it == participants_.end()) return false;
    
    // Remove participant
    participants_.erase(it);
    
    // Notify remaining participants
    broadcastParticipantRemoved(participantId);
    
    return true;
}

void LiveShare::setReadOnly(const std::string& participantId, bool readOnly) {
    auto it = participants_.find(participantId);
    if (it == participants_.end()) return;
    
    it->second.readOnly = readOnly;
    
    // Notify participant
    sendReadOnlyUpdate(participantId, readOnly);
}

void LiveShare::followParticipant(const std::string& participantId) {
    following_ = participantId;
    
    if (onFollowChanged) {
        onFollowChanged(participantId);
    }
}

void LiveShare::stopFollowing() {
    following_.clear();
    
    if (onFollowChanged) {
        onFollowChanged("");
    }
}

void LiveShare::sendChatMessage(const std::string& message) {
    ChatMessage msg;
    msg.senderId = localParticipantId_;
    msg.senderName = localParticipantName_;
    msg.content = message;
    msg.timestamp = std::chrono::steady_clock::now();
    
    chatHistory_.push_back(msg);
    
    // Broadcast to all participants
    broadcastChatMessage(msg);
}

const std::vector<ChatMessage>& LiveShare::getChatHistory() const {
    return chatHistory_;
}

const std::unordered_map<std::string, LiveShare::Participant>& LiveShare::getParticipants() const {
    return participants_;
}

const std::unordered_map<std::string, LiveShare::SharedDocument>& LiveShare::getSharedDocuments() const {
    return sharedDocuments_;
}

std::string LiveShare::getSessionId() const {
    return sessionId_;
}

bool LiveShare::isConnected() const {
    return connected_;
}

bool LiveShare::isHost() const {
    return isHost_;
}

std::string LiveShare::generateSessionId() const {
    // Generate unique session ID
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    std::stringstream ss;
    ss << "session_" << timestamp;
    
    return ss.str();
}

std::string LiveShare::generateInviteCode() const {
    const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string code;
    
    for (int i = 0; i < 8; ++i) {
        code += chars[rand() % 36];
    }
    
    return code;
}

std::string LiveShare::loadDocumentContent(const std::string& path) {
    // Load document content from file system
    // In production, would read from workspace
    return "";
}

void LiveShare::applyChange(std::string& content, const TextChange& change) {
    size_t pos = std::min(change.position, content.size());
    size_t maxLen = content.size() - pos;
    size_t len = std::min(change.length, maxLen);

    if (change.type == TextChangeType::Insert) {
        content.insert(pos, change.text);
    } else if (change.type == TextChangeType::Delete) {
        content.erase(pos, len);
    } else if (change.type == TextChangeType::Replace) {
        content.replace(pos, len, change.text);
    }
}

void LiveShare::broadcastDocumentShared(const SharedDocument& doc) {
    for (auto& [id, participant] : participants_) {
        if (participant.onDocumentShared) {
            participant.onDocumentShared(doc);
        }
    }
}

void LiveShare::broadcastDocumentUnshared(const std::string& documentId) {
    for (auto& [id, participant] : participants_) {
        if (participant.onDocumentUnshared) {
            participant.onDocumentUnshared(documentId);
        }
    }
}

void LiveShare::broadcastDocumentChange(const std::string& documentId,
                                         const TextChange& change,
                                         int version) {
    for (auto& [id, participant] : participants_) {
        if (participant.onDocumentChange) {
            participant.onDocumentChange(documentId, change, version);
        }
    }
}

void LiveShare::broadcastChatMessage(const ChatMessage& message) {
    for (auto& [id, participant] : participants_) {
        if (participant.onChatMessage) {
            participant.onChatMessage(message);
        }
    }
}

void LiveShare::broadcastParticipantRemoved(const std::string& participantId) {
    for (auto& [id, participant] : participants_) {
        if (participant.onParticipantRemoved) {
            participant.onParticipantRemoved(participantId);
        }
    }
}

void LiveShare::broadcastSessionEnd() {
    for (auto& [id, participant] : participants_) {
        if (participant.onSessionEnded) {
            participant.onSessionEnded();
        }
    }
}

void LiveShare::sendDocumentContent(const SharedDocument& doc) {
    // Send document content to requesting participant
}

void LiveShare::sendDocumentRequest(const std::string& documentId) {
    // Request document from host
}

void LiveShare::sendReadOnlyUpdate(const std::string& participantId, bool readOnly) {
    // Send read-only status update
}

// Terminal sharing
void LiveShare::shareTerminal(const std::string& terminalId) {
    if (!connected_ || !isHost_) return;
    
    SharedTerminal terminal;
    terminal.id = terminalId;
    terminal.output = "";
    
    sharedTerminals_[terminalId] = terminal;
    
    broadcastTerminalShared(terminal);
}

void LiveShare::sendTerminalOutput(const std::string& terminalId, 
                                    const std::string& output) {
    auto it = sharedTerminals_.find(terminalId);
    if (it == sharedTerminals_.end()) return;
    
    it->second.output += output;
    
    broadcastTerminalOutput(terminalId, output);
}

void LiveShare::sendTerminalInput(const std::string& terminalId,
                                   const std::string& input) {
    if (isHost_) {
        // Execute input in local terminal
    } else {
        // Send input to host
        broadcastTerminalInput(terminalId, input);
    }
}

void LiveShare::broadcastTerminalShared(const SharedTerminal& terminal) {
    for (auto& [id, participant] : participants_) {
        if (participant.onTerminalShared) {
            participant.onTerminalShared(terminal);
        }
    }
}

void LiveShare::broadcastTerminalOutput(const std::string& terminalId,
                                         const std::string& output) {
    for (auto& [id, participant] : participants_) {
        if (participant.onTerminalOutput) {
            participant.onTerminalOutput(terminalId, output);
        }
    }
}

void LiveShare::broadcastTerminalInput(const std::string& terminalId,
                                        const std::string& input) {
    (void)terminalId;
    (void)input;
}

// LiveShareSession implementation
LiveShareSession::LiveShareSession() = default;
LiveShareSession::~LiveShareSession() { stopHosting(); leaveSession(); }

bool LiveShareSession::startHosting(const std::string& sessionName) {
    sessionId_ = sessionName;
    hosting_ = true;
    return true;
}

void LiveShareSession::stopHosting() { hosting_ = false; }

bool LiveShareSession::joinSession(const std::string& sessionId, const std::string& password) {
    (void)password;
    sessionId_ = sessionId;
    joined_ = true;
    return true;
}

void LiveShareSession::leaveSession() { joined_ = false; }

Participant* LiveShareSession::getParticipant(const std::string& id) {
    for (auto& p : participants_) if (p.id == id) return &p;
    return nullptr;
}

Participant* LiveShareSession::getHost() {
    for (auto& p : participants_) if (p.isHost) return &p;
    return nullptr;
}

void LiveShareSession::shareDocument(const std::string& path, const std::string& content) {
    sendOperation("share", path + ":" + content);
}
void LiveShareSession::unshareDocument(const std::string& path) { sendOperation("unshare", path); }
void LiveShareSession::updateDocument(const std::string& path, const std::string& content) {
    sendOperation("update", path + ":" + content);
}
void LiveShareSession::updateCursor(size_t position) { sendOperation("cursor", std::to_string(position)); }
void LiveShareSession::broadcastCursor(const std::string& participantId, size_t position) {
    if (auto* p = getParticipant(participantId)) p->cursorPosition = position;
}
void LiveShareSession::sendMessage(const std::string& message) { sendOperation("chat", message); }
void LiveShareSession::sendSystemMessage(const std::string& message) { sendOperation("system", message); }
void LiveShareSession::onParticipantJoined(ParticipantCallback callback) { (void)callback; }
void LiveShareSession::onParticipantLeft(ParticipantCallback callback) { (void)callback; }
void LiveShareSession::onMessageReceived(MessageCallback callback) { (void)callback; }
void LiveShareSession::onDocumentShared(DocumentCallback callback) { (void)callback; }
void LiveShareSession::onDocumentUpdated(DocumentCallback callback) { (void)callback; }
void LiveShareSession::handleIncomingMessage(const std::string& message) { (void)message; }
void LiveShareSession::sendOperation(const std::string& type, const std::string& data) {
    if (connection_ && connection_->isConnected()) {
        connection_->send(type + ":" + data);
    }
}

// LiveShareServer implementation
LiveShareServer::LiveShareServer() = default;
LiveShareServer::~LiveShareServer() { stop(); }
bool LiveShareServer::start(int port) { port_ = port; running_ = true; return true; }
void LiveShareServer::stop() { running_ = false; sessions_.clear(); }
bool LiveShareServer::isRunning() const { return running_; }
std::vector<CollaborationSession> LiveShareServer::getActiveSessions() const { return sessions_; }
CollaborationSession* LiveShareServer::getSession(const std::string& id) {
    for (auto& s : sessions_) if (s.id == id) return &s;
    return nullptr;
}
CollaborationSession* LiveShareServer::createSession(const std::string& name, const std::string& host,
                                                      const std::string& description) {
    CollaborationSession s;
    s.id = "session_" + std::to_string(sessions_.size() + 1);
    s.name = name;
    s.host = host;
    s.description = description;
    sessions_.push_back(std::move(s));
    return &sessions_.back();
}
void LiveShareServer::closeSession(const std::string& id) {
    sessions_.erase(std::remove_if(sessions_.begin(), sessions_.end(),
        [&](const CollaborationSession& s) { return s.id == id; }), sessions_.end());
}

} // namespace NexusForge::Networking
