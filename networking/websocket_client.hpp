// networking/websocket_client.hpp
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <queue>
#include <mutex>

namespace NexusForge::Networking {

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

// WebSocket message
struct WebSocketMessage {
    OpCode opcode = OpCode::Text;
    std::string data;
    bool fin = true;
    bool masked = false;
};

// WebSocket client
class WebSocketClient {
public:
    WebSocketClient();
    ~WebSocketClient();

    // Connection
    bool connect(const std::string& url, const std::vector<std::string>& protocols = {});
    void disconnect(uint16_t code = 1000, const std::string& reason = "");
    WebSocketState getState() const { return state_; }

    // Sending
    void send(const std::string& message);
    void sendBinary(const std::vector<uint8_t>& data);
    void sendPing();
    void sendPong();

    // Events
    using MessageHandler = std::function<void(const std::string&)>;
    using BinaryHandler = std::function<void(const std::vector<uint8_t>&)>;
    using OpenHandler = std::function<void()>;
    using CloseHandler = std::function<void(uint16_t, const std::string&)>;
    using ErrorHandler = std::function<void(const std::string&)>;

    void onMessage(MessageHandler handler) { messageHandler_ = handler; }
    void onBinary(BinaryHandler handler) { binaryHandler_ = handler; }
    void onOpen(OpenHandler handler) { openHandler_ = handler; }
    void onClose(CloseHandler handler) { closeHandler_ = handler; }
    void onError(ErrorHandler handler) { errorHandler_ = handler; }

    // Configuration
    void setPingInterval(int seconds) { pingInterval_ = seconds; }
    void setReconnect(bool reconnect, int maxAttempts = -1);
    void addHeader(const std::string& name, const std::string& value);

    // Info
    std::string getUrl() const { return url_; }
    std::string getProtocol() const { return protocol_; }
    size_t bufferedAmount() const { return sendQueue_.size(); }

private:
    WebSocketState state_ = WebSocketState::Closed;
    std::string url_;
    std::string protocol_;
    std::string host_;
    int port_ = 80;
    bool secure_ = false;

    MessageHandler messageHandler_;
    BinaryHandler binaryHandler_;
    OpenHandler openHandler_;
    CloseHandler closeHandler_;
    ErrorHandler errorHandler_;

    int pingInterval_ = 30;
    bool autoReconnect_ = false;
    int maxReconnectAttempts_ = -1;
    int reconnectAttempts_ = 0;

    std::vector<std::pair<std::string, std::string>> headers_;
    std::queue<WebSocketMessage> sendQueue_;
    std::mutex sendMutex_;

    // Internal
    class Impl;
    std::unique_ptr<Impl> impl_;

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
