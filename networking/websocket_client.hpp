// networking/websocket_client.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <queue>
#include <mutex>
#include <cstdint>

namespace NexusForge::Networking {

// WebSocket options
struct WebSocketOptions {
    std::vector<std::string> protocols;
    std::unordered_map<std::string, std::string> headers;
    int pingIntervalMs = 30000;
};

// WebSocket state
enum class WebSocketState {
    Connecting,
    Open,
    Closing,
    Closed
};

// WebSocket frame types
enum class OpCode {
    Continuation = 0,
    Text = 1,
    Binary = 2,
    Close = 8,
    Ping = 9,
    Pong = 10
};
using Opcode = OpCode;

// WebSocket message
struct WebSocketMessage {
    OpCode opcode = OpCode::Text;
    std::string data;
    bool fin = true;
    bool masked = false;
};

// WebSocket client (unified)
class WebSocketClient {
public:
    WebSocketClient();
    ~WebSocketClient();

    // Connection
    bool connect(const std::string& url);
    bool connect(const std::string& url, const std::vector<std::string>& protocols);
    bool connect(const std::string& url, const WebSocketOptions& options);
    void disconnect(uint16_t code = 1000, const std::string& reason = "");
    bool isConnected() const;
    WebSocketState getState() const { return state_; }

    // Sending
    void send(const std::string& message);
    void sendBinary(const std::vector<uint8_t>& data);
    void sendPing(const std::string& data = "");
    void sendPong(const std::string& data = "");
    void sendClose(uint16_t code = 1000, const std::string& reason = "");
    void ping() { sendPing(); }

    // Frame processing
    void onDataReceived(const uint8_t* data, size_t len);

    // Events (setter style)
    using MessageHandler = std::function<void(const std::string&)>;
    using BinaryHandler = std::function<void(const std::vector<uint8_t>&)>;
    using OpenHandler = std::function<void()>;
    using CloseHandler = std::function<void(uint16_t, const std::string&)>;
    using ErrorHandler = std::function<void(const std::string&)>;

    void onMessage(MessageHandler handler) { messageHandler_ = std::move(handler); }
    void onBinary(BinaryHandler handler) { binaryHandler_ = std::move(handler); }
    void onOpen(OpenHandler handler) { openHandler_ = std::move(handler); }
    void onConnect(OpenHandler handler) { openHandler_ = std::move(handler); }
    void onClose(CloseHandler handler) { closeHandler_ = std::move(handler); }
    void onError(ErrorHandler handler) { errorHandler_ = std::move(handler); }

    // Events (callback member style)
    std::function<void(const std::string&, bool)> onMessageCallback;
    std::function<void(const std::string&)> onMessageSent;
    std::function<void(const std::vector<uint8_t>&)> onBinaryMessage;
    std::function<void(const std::string&)> onPing;
    std::function<void(const std::string&)> onPong;

    // Configuration
    void setPingInterval(int intervalMs);
    void setReconnect(bool reconnect, int maxAttempts = -1);
    void addHeader(const std::string& name, const std::string& value);

    // Info
    std::string getUrl() const;
    std::string getHost() const;
    int getPort() const;
    std::string getProtocol() const { return protocol_; }
    size_t bufferedAmount() const { return sendQueue_.size(); }

private:
    bool connected_ = false;
    WebSocketState state_ = WebSocketState::Closed;
    std::string url_;
    std::string protocol_;
    std::string host_;
    std::string path_ = "/";
    std::string key_;
    int port_ = 80;
    bool secure_ = false;
    WebSocketOptions options_;

    MessageHandler messageHandler_;
    BinaryHandler binaryHandler_;
    OpenHandler openHandler_;
    CloseHandler closeHandler_;
    ErrorHandler errorHandler_;

    int pingIntervalMs_ = 30000;
    bool autoReconnect_ = false;
    int maxReconnectAttempts_ = -1;
    int reconnectAttempts_ = 0;

    std::vector<std::pair<std::string, std::string>> headers_;
    std::queue<WebSocketMessage> sendQueue_;
    std::mutex sendMutex_;

    // Internal
    class Impl;
    std::unique_ptr<Impl> impl_;

    std::string buildHandshakeRequest();
    std::vector<uint8_t> buildFrame(Opcode opcode, const std::string& data);
    std::vector<uint8_t> buildFrame(Opcode opcode, const std::vector<uint8_t>& data);
    std::string generateWebSocketKey();
    void setState(WebSocketState state);
    void processQueue();
    void reconnect();
};

// WebSocket server (simple)
class WebSocketServer {
public:
    WebSocketServer();
    ~WebSocketServer();

    bool start(int port);
    void stop();
    bool isRunning() const;

    using ConnectionHandler = std::function<void(int)>;
    using MessageHandler = std::function<void(int, const std::string&)>;
    using CloseHandler = std::function<void(int)>;

    void onConnection(ConnectionHandler handler);
    void onMessage(MessageHandler handler);
    void onClose(CloseHandler handler);

    void send(int clientId, const std::string& message);
    void broadcast(const std::string& message);
    void disconnect(int clientId);

private:
    bool running_ = false;
    int port_ = 0;

    ConnectionHandler connectionHandler_;
    MessageHandler messageHandler_;
    CloseHandler closeHandler_;
};

} // namespace NexusForge::Networking
