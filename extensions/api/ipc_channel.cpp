// extensions/api/ipc_channel.cpp
// IPC Channel Implementation

#include "ipc_channel.hpp"
#include <iostream>

namespace NexusForge::Extensions {

IPCChannel::IPCChannel() = default;
IPCChannel::~IPCChannel() = default;

std::string IPCChannel::request(const std::string& channel, const std::string& data,
                                 int timeoutMs) {
    (void)timeoutMs;
    if (!isConnected()) return "";
    send(channel, data);
    return "";
}

void IPCChannel::requestAsync(const std::string& channel, const std::string& data,
                               RequestCallback callback, int timeoutMs) {
    std::string response = request(channel, data, timeoutMs);
    if (callback) {
        callback(response);
    }
}

void IPCChannel::onConnected(ConnectedCallback callback) {
    connectedCallback_ = std::move(callback);
}

void IPCChannel::onDisconnected(DisconnectedCallback callback) {
    disconnectedCallback_ = std::move(callback);
}

void IPCChannel::onError(ErrorCallback callback) {
    errorCallback_ = std::move(callback);
}

// NamedPipeChannel
NamedPipeChannel::NamedPipeChannel(const std::string& pipeName) : pipeName_(pipeName) {}
NamedPipeChannel::~NamedPipeChannel() = default;
bool NamedPipeChannel::connect() { connected_ = true; return true; }
void NamedPipeChannel::disconnect() { connected_ = false; }
bool NamedPipeChannel::isConnected() const { return connected_; }
void NamedPipeChannel::send(const IPCMessage& message) {}
void NamedPipeChannel::send(const std::string& channel, const std::string& data) {}
void NamedPipeChannel::setMessageCallback(MessageCallback callback) { messageCallback_ = callback; }

// UnixSocketChannel
UnixSocketChannel::UnixSocketChannel(const std::string& socketPath) : socketPath_(socketPath) {}
UnixSocketChannel::~UnixSocketChannel() = default;
bool UnixSocketChannel::connect() { connected_ = true; return true; }
void UnixSocketChannel::disconnect() { connected_ = false; }
bool UnixSocketChannel::isConnected() const { return connected_; }
void UnixSocketChannel::send(const IPCMessage& message) {}
void UnixSocketChannel::send(const std::string& channel, const std::string& data) {}
void UnixSocketChannel::setMessageCallback(MessageCallback callback) { messageCallback_ = callback; }

// TCPSocketChannel
TCPSocketChannel::TCPSocketChannel(const std::string& host, int port) : host_(host), port_(port) {}
TCPSocketChannel::~TCPSocketChannel() = default;
bool TCPSocketChannel::connect() { connected_ = true; return true; }
void TCPSocketChannel::disconnect() { connected_ = false; }
bool TCPSocketChannel::isConnected() const { return connected_; }
void TCPSocketChannel::send(const IPCMessage& message) {}
void TCPSocketChannel::send(const std::string& channel, const std::string& data) {}
void TCPSocketChannel::setMessageCallback(MessageCallback callback) { messageCallback_ = callback; }

namespace IPCProtocol {
    namespace {
        std::string escapeJson(const std::string& s) {
            std::string out;
            out.reserve(s.size());
            for (char c : s) {
                switch (c) {
                    case '"': out += "\\\""; break;
                    case '\\': out += "\\\\"; break;
                    case '\n': out += "\\n"; break;
                    case '\r': out += "\\r"; break;
                    case '\t': out += "\\t"; break;
                    default: out += c; break;
                }
            }
            return out;
        }

        std::string extractJsonField(const std::string& json, const std::string& key) {
            std::string needle = "\"" + key + "\":\"";
            size_t pos = json.find(needle);
            if (pos == std::string::npos) return "";
            pos += needle.size();
            std::string out;
            while (pos < json.size()) {
                char c = json[pos++];
                if (c == '\\' && pos < json.size()) {
                    char next = json[pos++];
                    switch (next) {
                        case 'n': out += '\n'; break;
                        case 'r': out += '\r'; break;
                        case 't': out += '\t'; break;
                        default: out += next; break;
                    }
                } else if (c == '"') {
                    break;
                } else {
                    out += c;
                }
            }
            return out;
        }
    }

    std::string serializeMessage(const IPCMessage& msg) {
        return "{\"type\":\"" + escapeJson(msg.type) +
               "\",\"channel\":\"" + escapeJson(msg.channel) +
               "\",\"data\":\"" + escapeJson(msg.data) +
               "\",\"messageId\":\"" + escapeJson(msg.messageId) +
               "\",\"replyTo\":\"" + escapeJson(msg.replyTo) +
               "\",\"sourceId\":\"" + escapeJson(msg.sourceId) +
               "\",\"targetId\":\"" + escapeJson(msg.targetId) +
               "\",\"payload\":\"" + escapeJson(msg.payload) + "\"}";
    }

    IPCMessage deserializeMessage(const std::string& data) {
        IPCMessage msg;
        msg.type = extractJsonField(data, "type");
        msg.channel = extractJsonField(data, "channel");
        msg.data = extractJsonField(data, "data");
        msg.messageId = extractJsonField(data, "messageId");
        msg.replyTo = extractJsonField(data, "replyTo");
        msg.sourceId = extractJsonField(data, "sourceId");
        msg.targetId = extractJsonField(data, "targetId");
        msg.payload = extractJsonField(data, "payload");
        return msg;
    }

    std::string createRequest(const std::string& channel, const std::string& data) {
        IPCMessage msg;
        msg.type = "request";
        msg.channel = channel;
        msg.data = data;
        return serializeMessage(msg);
    }

    std::string createResponse(const std::string& messageId, const std::string& data) {
        IPCMessage msg;
        msg.type = "response";
        msg.replyTo = messageId;
        msg.data = data;
        return serializeMessage(msg);
    }

    std::string createError(const std::string& messageId, const std::string& error) {
        IPCMessage msg;
        msg.type = "error";
        msg.replyTo = messageId;
        msg.data = error;
        msg.success = false;
        return serializeMessage(msg);
    }

    std::string createEvent(const std::string& channel, const std::string& data) {
        IPCMessage msg;
        msg.type = "event";
        msg.channel = channel;
        msg.data = data;
        return serializeMessage(msg);
    }
}

} // namespace NexusForge::Extensions