// networking/websocket_client.cpp
// WebSocket Client Implementation

#include "websocket_client.hpp"
#include <sstream>
#include <cstring>
#include <random>

namespace NexusForge::Networking {

WebSocketClient::WebSocketClient() : connected_(false), port_(80) {
}

WebSocketClient::~WebSocketClient() {
    disconnect();
}

bool WebSocketClient::connect(const std::string& url) {
    if (connected_) {
        disconnect();
    }
    
    // Parse URL
    url_ = url;
    
    size_t wsPos = url.find("ws://");
    size_t wssPos = url.find("wss://");
    
    if (wssPos == 0) {
        secure_ = true;
        port_ = 443;
        url_ = url.substr(6);
    } else if (wsPos == 0) {
        secure_ = false;
        port_ = 80;
        url_ = url.substr(5);
    } else {
        return false;
    }
    
    // Parse host and path
    size_t pathPos = url_.find('/');
    if (pathPos != std::string::npos) {
        path_ = url_.substr(pathPos);
        host_ = url_.substr(0, pathPos);
    } else {
        path_ = "/";
        host_ = url_;
    }
    
    // Check for port in host
    size_t portPos = host_.rfind(':');
    if (portPos != std::string::npos) {
        port_ = std::stoi(host_.substr(portPos + 1));
        host_ = host_.substr(0, portPos);
    }
    
    // In production, would establish TCP/TLS connection
    // For now, simulate connection
    
    // Generate WebSocket key
    key_ = generateWebSocketKey();
    
    // Build handshake request
    std::string handshake = buildHandshakeRequest();
    
    // Send handshake (simulated)
    connected_ = true;
    
    if (onOpen) {
        onOpen();
    }
    
    return true;
}

void WebSocketClient::disconnect() {
    if (!connected_) return;
    
    // Send close frame
    sendClose(1000, "Normal closure");
    
    connected_ = false;
    
    if (onClose) {
        onClose(1000, "Normal closure");
    }
}

void WebSocketClient::send(const std::string& message) {
    if (!connected_) return;
    
    // Build text frame
    std::vector<uint8_t> frame = buildFrame(Opcode::Text, message);
    
    // Send frame (simulated)
    if (onMessageSent) {
        onMessageSent(message);
    }
}

void WebSocketClient::sendBinary(const std::vector<uint8_t>& data) {
    if (!connected_) return;
    
    // Build binary frame
    std::vector<uint8_t> frame = buildFrame(Opcode::Binary, data);
    
    // Send frame (simulated)
}

void WebSocketClient::sendPing(const std::string& data) {
    if (!connected_) return;
    
    std::vector<uint8_t> frame = buildFrame(Opcode::Ping, data);
    // Send ping frame
}

void WebSocketClient::sendPong(const std::string& data) {
    if (!connected_) return;
    
    std::vector<uint8_t> frame = buildFrame(Opcode::Pong, data);
    // Send pong frame
}

void WebSocketClient::sendClose(uint16_t code, const std::string& reason) {
    if (!connected_) return;
    
    std::vector<uint8_t> payload;
    payload.push_back((code >> 8) & 0xFF);
    payload.push_back(code & 0xFF);
    
    for (char c : reason) {
        payload.push_back(static_cast<uint8_t>(c));
    }
    
    std::vector<uint8_t> frame = buildFrame(Opcode::Close, payload);
    // Send close frame
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
    ss << "Sec-WebSocket-Protocol: chat\r\n";
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
    uint8_t byte1 = 0x80;  // FIN bit set
    byte1 |= (static_cast<uint8_t>(opcode) & 0x0F);
    frame.push_back(byte1);
    
    // Byte 2: Mask + Payload length
    uint8_t byte2 = 0x80;  // Mask bit set (client must mask)
    size_t len = data.size();
    
    if (len < 126) {
        byte2 |= len;
        frame.push_back(byte2);
    } else if (len < 65536) {
        byte2 |= 126;
        frame.push_back(byte2);
        frame.push_back((len >> 8) & 0xFF);
        frame.push_back(len & 0xFF);
    } else {
        byte2 |= 127;
        frame.push_back(byte2);
        for (int i = 7; i >= 0; --i) {
            frame.push_back((len >> (i * 8)) & 0xFF);
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
    if (!connected_ || len < 2) return;
    
    // Parse WebSocket frame
    bool fin = (data[0] & 0x80) != 0;
    Opcode opcode = static_cast<Opcode>(data[0] & 0x0F);
    bool masked = (data[1] & 0x80) != 0;
    size_t payloadLen = data[1] & 0x7F;
    
    size_t headerSize = 2;
    
    // Extended payload length
    if (payloadLen == 126) {
        if (len < 4) return;
        payloadLen = (data[2] << 8) | data[3];
        headerSize = 4;
    } else if (payloadLen == 127) {
        if (len < 10) return;
        payloadLen = 0;
        for (int i = 0; i < 8; ++i) {
            payloadLen = (payloadLen << 8) | data[2 + i];
        }
        headerSize = 10;
    }
    
    // Masking key
    uint8_t mask[4] = {0, 0, 0, 0};
    if (masked) {
        if (len < headerSize + 4) return;
        for (int i = 0; i < 4; ++i) {
            mask[i] = data[headerSize + i];
        }
        headerSize += 4;
    }
    
    // Payload
    if (len < headerSize + payloadLen) return;
    
    std::vector<uint8_t> payload;
    for (size_t i = 0; i < payloadLen; ++i) {
        uint8_t byte = data[headerSize + i];
        if (masked) {
            byte ^= mask[i % 4];
        }
        payload.push_back(byte);
    }
    
    // Handle opcode
    switch (opcode) {
        case Opcode::Text:
            if (onMessage) {
                std::string message(payload.begin(), payload.end());
                onMessage(message, false);
            }
            break;
            
        case Opcode::Binary:
            if (onBinaryMessage) {
                onBinaryMessage(payload);
            }
            break;
            
        case Opcode::Close:
            connected_ = false;
            if (onClose) {
                uint16_t code = 1000;
                std::string reason;
                
                if (payloadLen >= 2) {
                    code = (payload[0] << 8) | payload[1];
                    reason = std::string(payload.begin() + 2, payload.end());
                }
                
                onClose(code, reason);
            }
            break;
            
        case Opcode::Ping:
            // Send pong response
            sendPong(std::string(payload.begin(), payload.end()));
            if (onPing) {
                onPing(std::string(payload.begin(), payload.end()));
            }
            break;
            
        case Opcode::Pong:
            if (onPong) {
                onPong(std::string(payload.begin(), payload.end()));
            }
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
    
    // Base64 encode (simplified)
    static const char* base64Chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string result;
    for (int i = 0; i < 16; i += 3) {
        result += base64Chars[(key[i] >> 2) & 0x3F];
        result += base64Chars[((key[i] & 0x03) << 4) | ((key[i + 1] >> 4) & 0x0F)];
        result += base64Chars[((key[i + 1] & 0x0F) << 2) | ((key[i + 2] >> 6) & 0x03)];
        result += base64Chars[key[i + 2] & 0x3F];
    }
    
    return result;
}

bool WebSocketClient::isConnected() const {
    return connected_;
}

std::string WebSocketClient::getUrl() const {
    return url_;
}

std::string WebSocketClient::getHost() const {
    return host_;
}

int WebSocketClient::getPort() const {
    return port_;
}

void WebSocketClient::setPingInterval(int intervalMs) {
    pingIntervalMs_ = intervalMs;
}

} // namespace NexusForge::Networking
