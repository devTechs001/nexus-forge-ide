// networking/websocket_client.cpp
// WebSocket Client Implementation

#include "websocket_client.hpp"
#include <sstream>
#include <cstring>
#include <random>

namespace NexusForge::Networking {

class WebSocketClient::Impl {};

WebSocketClient::WebSocketClient() : connected_(false), port_(80) {
}

WebSocketClient::~WebSocketClient() {
    disconnect();
}

bool WebSocketClient::connect(const std::string& url) {
    if (connected_) {
        disconnect();
    }

    url_ = url;
    std::string remainder;

    if (url.rfind("wss://", 0) == 0) {
        secure_ = true;
        port_ = 443;
        remainder = url.substr(6);
    } else if (url.rfind("ws://", 0) == 0) {
        secure_ = false;
        port_ = 80;
        remainder = url.substr(5);
    } else {
        return false;
    }

    // Parse host and path
    size_t pathPos = remainder.find('/');
    if (pathPos != std::string::npos) {
        path_ = remainder.substr(pathPos);
        host_ = remainder.substr(0, pathPos);
    } else {
        path_ = "/";
        host_ = remainder;
    }

    // Check for port in host
    size_t portPos = host_.rfind(':');
    if (portPos != std::string::npos) {
        try {
            port_ = std::stoi(host_.substr(portPos + 1));
        } catch (...) {
            return false;
        }
        host_ = host_.substr(0, portPos);
    }

    key_ = generateWebSocketKey();
    std::string handshake = buildHandshakeRequest();
    (void)handshake;

    connected_ = true;
    setState(WebSocketState::Open);

    if (openHandler_) {
        openHandler_();
    }

    return true;
}

bool WebSocketClient::connect(const std::string& url, const std::vector<std::string>& protocols) {
    if (!protocols.empty()) protocol_ = protocols.front();
    return connect(url);
}

bool WebSocketClient::connect(const std::string& url, const WebSocketOptions& options) {
    options_ = options;
    pingIntervalMs_ = options.pingIntervalMs;
    return connect(url, options.protocols);
}

void WebSocketClient::disconnect(uint16_t code, const std::string& reason) {
    if (!connected_) return;

    std::string closeReason = reason.empty() ? "Normal closure" : reason;
    sendClose(code, closeReason);

    connected_ = false;
    setState(WebSocketState::Closed);

    if (closeHandler_) {
        closeHandler_(code, closeReason);
    }
}

void WebSocketClient::send(const std::string& message) {
    if (!connected_) return;

    std::vector<uint8_t> frame = buildFrame(Opcode::Text, message);
    (void)frame;

    if (onMessageSent) {
        onMessageSent(message);
    }
}

void WebSocketClient::sendBinary(const std::vector<uint8_t>& data) {
    if (!connected_) return;

    std::vector<uint8_t> frame = buildFrame(Opcode::Binary, data);
    (void)frame;
}

void WebSocketClient::sendPing(const std::string& data) {
    if (!connected_) return;

    std::vector<uint8_t> frame = buildFrame(Opcode::Ping, data);
    (void)frame;
}

void WebSocketClient::sendPong(const std::string& data) {
    if (!connected_) return;

    std::vector<uint8_t> frame = buildFrame(Opcode::Pong, data);
    (void)frame;
}

void WebSocketClient::sendClose(uint16_t code, const std::string& reason) {
    if (!connected_) return;

    std::vector<uint8_t> payload;
    payload.push_back(static_cast<uint8_t>((code >> 8) & 0xFF));
    payload.push_back(static_cast<uint8_t>(code & 0xFF));

    for (char c : reason) {
        payload.push_back(static_cast<uint8_t>(c));
    }

    std::vector<uint8_t> frame = buildFrame(Opcode::Close, payload);
    (void)frame;
}

std::string WebSocketClient::buildHandshakeRequest() {
    std::stringstream ss;

    ss << "GET " << path_ << " HTTP/1.1\r\n";
    ss << "Host: " << host_;
    if (port_ != 80 && port_ != 443) {
        ss << ":" << port_;
    }
    ss << "\r\n";
    ss << "Upgrade: websocket\r\n";
    ss << "Connection: Upgrade\r\n";
    ss << "Sec-WebSocket-Key: " << key_ << "\r\n";
    ss << "Sec-WebSocket-Version: 13\r\n";
    if (!protocol_.empty()) {
        ss << "Sec-WebSocket-Protocol: " << protocol_ << "\r\n";
    }
    ss << "\r\n";

    return ss.str();
}

std::vector<uint8_t> WebSocketClient::buildFrame(Opcode opcode, const std::string& data) {
    std::vector<uint8_t> bytes(data.begin(), data.end());
    return buildFrame(opcode, bytes);
}

std::vector<uint8_t> WebSocketClient::buildFrame(Opcode opcode, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> frame;

    // Byte 1: FIN + Opcode
    uint8_t byte1 = 0x80 | (static_cast<uint8_t>(opcode) & 0x0F);
    frame.push_back(byte1);

    // Byte 2: Mask + Payload length
    uint8_t byte2 = 0x80;  // Mask bit set (client must mask)
    size_t len = data.size();

    if (len < 126) {
        byte2 |= static_cast<uint8_t>(len);
        frame.push_back(byte2);
    } else if (len < 65536) {
        byte2 |= 126;
        frame.push_back(byte2);
        frame.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
        frame.push_back(static_cast<uint8_t>(len & 0xFF));
    } else {
        byte2 |= 127;
        frame.push_back(byte2);
        for (int i = 7; i >= 0; --i) {
            frame.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
        }
    }

    // Masking key (4 bytes)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    uint8_t mask[4];
    for (int i = 0; i < 4; ++i) {
        mask[i] = static_cast<uint8_t>(dis(gen));
        frame.push_back(mask[i]);
    }

    // Masked payload
    for (size_t i = 0; i < data.size(); ++i) {
        frame.push_back(data[i] ^ mask[i % 4]);
    }

    return frame;
}

void WebSocketClient::onDataReceived(const uint8_t* data, size_t len) {
    if (!connected_ || !data || len < 2) return;

    bool fin = (data[0] & 0x80) != 0;
    (void)fin;
    Opcode opcode = static_cast<Opcode>(data[0] & 0x0F);
    bool masked = (data[1] & 0x80) != 0;
    size_t payloadLen = data[1] & 0x7F;

    size_t headerSize = 2;

    if (payloadLen == 126) {
        if (len < 4) return;
        payloadLen = (static_cast<size_t>(data[2]) << 8) | data[3];
        headerSize = 4;
    } else if (payloadLen == 127) {
        if (len < 10) return;
        payloadLen = 0;
        for (int i = 0; i < 8; ++i) {
            payloadLen = (payloadLen << 8) | data[2 + i];
        }
        headerSize = 10;
    }

    uint8_t mask[4] = {0, 0, 0, 0};
    if (masked) {
        if (len < headerSize + 4) return;
        for (int i = 0; i < 4; ++i) {
            mask[i] = data[headerSize + i];
        }
        headerSize += 4;
    }

    if (len < headerSize || payloadLen > len - headerSize) return;

    std::vector<uint8_t> payload;
    payload.reserve(payloadLen);
    for (size_t i = 0; i < payloadLen; ++i) {
        uint8_t byte = data[headerSize + i];
        if (masked) {
            byte ^= mask[i % 4];
        }
        payload.push_back(byte);
    }

    switch (opcode) {
        case Opcode::Text: {
            std::string message(payload.begin(), payload.end());
            if (messageHandler_) messageHandler_(message);
            if (onMessageCallback) onMessageCallback(message, false);
            break;
        }

        case Opcode::Binary:
            if (binaryHandler_) binaryHandler_(payload);
            if (onBinaryMessage) onBinaryMessage(payload);
            break;

        case Opcode::Close: {
            connected_ = false;
            setState(WebSocketState::Closed);
            uint16_t code = 1000;
            std::string reason;
            if (payloadLen >= 2) {
                code = static_cast<uint16_t>((payload[0] << 8) | payload[1]);
                reason = std::string(payload.begin() + 2, payload.end());
            }
            if (closeHandler_) closeHandler_(code, reason);
            break;
        }

        case Opcode::Ping: {
            std::string pingData(payload.begin(), payload.end());
            sendPong(pingData);
            if (onPing) onPing(pingData);
            break;
        }

        case Opcode::Pong:
            if (onPong) onPong(std::string(payload.begin(), payload.end()));
            break;

        case Opcode::Continuation:
            break;
    }
}

std::string WebSocketClient::generateWebSocketKey() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    uint8_t key[16];
    for (int i = 0; i < 16; ++i) {
        key[i] = static_cast<uint8_t>(dis(gen));
    }

    // RFC 4648 Base64 encoding of 16 bytes (5 full 3-byte blocks + 1 trailing byte + "==")
    static const char* base64Chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string result;
    result.reserve(24);
    for (int i = 0; i < 15; i += 3) {
        uint32_t triple = (static_cast<uint32_t>(key[i]) << 16) |
                          (static_cast<uint32_t>(key[i + 1]) << 8) |
                          static_cast<uint32_t>(key[i + 2]);
        result += base64Chars[(triple >> 18) & 0x3F];
        result += base64Chars[(triple >> 12) & 0x3F];
        result += base64Chars[(triple >> 6) & 0x3F];
        result += base64Chars[triple & 0x3F];
    }
    // Final 16th byte (index 15) with '==' padding
    result += base64Chars[(key[15] >> 2) & 0x3F];
    result += base64Chars[(key[15] & 0x03) << 4];
    result += "==";

    return result;
}

bool WebSocketClient::isConnected() const { return connected_; }
std::string WebSocketClient::getUrl() const { return url_; }
std::string WebSocketClient::getHost() const { return host_; }
int WebSocketClient::getPort() const { return port_; }
void WebSocketClient::setPingInterval(int intervalMs) { pingIntervalMs_ = intervalMs; }

void WebSocketClient::setReconnect(bool reconnect, int maxAttempts) {
    autoReconnect_ = reconnect;
    maxReconnectAttempts_ = maxAttempts;
}

void WebSocketClient::addHeader(const std::string& name, const std::string& value) {
    headers_.emplace_back(name, value);
}

void WebSocketClient::setState(WebSocketState state) { state_ = state; }
void WebSocketClient::processQueue() {}
void WebSocketClient::reconnect() {
    if (autoReconnect_ && (maxReconnectAttempts_ < 0 || reconnectAttempts_ < maxReconnectAttempts_)) {
        reconnectAttempts_++;
        connect(url_);
    }
}

// WebSocketServer implementation
WebSocketServer::WebSocketServer() = default;
WebSocketServer::~WebSocketServer() { stop(); }
bool WebSocketServer::start(int port) { port_ = port; running_ = true; return true; }
void WebSocketServer::stop() { running_ = false; }
bool WebSocketServer::isRunning() const { return running_; }
void WebSocketServer::onConnection(ConnectionHandler handler) { connectionHandler_ = std::move(handler); }
void WebSocketServer::onMessage(MessageHandler handler) { messageHandler_ = std::move(handler); }
void WebSocketServer::onClose(CloseHandler handler) { closeHandler_ = std::move(handler); }
void WebSocketServer::send(int clientId, const std::string& message) { (void)clientId; (void)message; }
void WebSocketServer::broadcast(const std::string& message) { (void)message; }
void WebSocketServer::disconnect(int clientId) { (void)clientId; }

} // namespace NexusForge::Networking
