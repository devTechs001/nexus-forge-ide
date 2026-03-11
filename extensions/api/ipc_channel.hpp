// extensions/api/ipc_channel.hpp
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <queue>
#include <mutex>

namespace NexusForge::Extensions {

// IPC message
struct IPCMessage {
    std::string type;
    std::string channel;
    std::string data;
    std::string messageId;
    std::string replyTo;
    int64_t timestamp = 0;
};

// IPC channel for extension-host communication
class IPCChannel {
public:
    IPCChannel();
    virtual ~IPCChannel();

    // Connection
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    // Messaging
    virtual void send(const IPCMessage& message) = 0;
    virtual void send(const std::string& channel, const std::string& data) = 0;

    // Receive
    using MessageCallback = std::function<void(const IPCMessage&)>;
    virtual void setMessageCallback(MessageCallback callback) = 0;

    // Request/Response
    std::string request(const std::string& channel, const std::string& data,
                        int timeoutMs = 5000);
    
    using RequestCallback = std::function<void(const std::string&)>;
    void requestAsync(const std::string& channel, const std::string& data,
                      RequestCallback callback, int timeoutMs = 5000);

    // Events
    using ConnectedCallback = std::function<void()>;
    using DisconnectedCallback = std::function<void()>;
    using ErrorCallback = std::function<void(const std::string&)>;

    void onConnected(ConnectedCallback callback);
    void onDisconnected(DisconnectedCallback callback);
    void onError(ErrorCallback callback);

protected:
    MessageCallback messageCallback_;
    ConnectedCallback connectedCallback_;
    DisconnectedCallback disconnectedCallback_;
    ErrorCallback errorCallback_;

    std::mutex sendMutex_;
    std::queue<IPCMessage> pendingRequests_;
};

// Named pipe IPC channel (Windows)
class NamedPipeChannel : public IPCChannel {
public:
    NamedPipeChannel(const std::string& pipeName);
    ~NamedPipeChannel() override;

    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;

    void send(const IPCMessage& message) override;
    void send(const std::string& channel, const std::string& data) override;
    void setMessageCallback(MessageCallback callback) override;

private:
    std::string pipeName_;
    void* pipeHandle_ = nullptr;
    bool connected_ = false;

    void readLoop();
    void writeMessage(const IPCMessage& message);
};

// Unix domain socket channel
class UnixSocketChannel : public IPCChannel {
public:
    UnixSocketChannel(const std::string& socketPath);
    ~UnixSocketChannel() override;

    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;

    void send(const IPCMessage& message) override;
    void send(const std::string& channel, const std::string& data) override;
    void setMessageCallback(MessageCallback callback) override;

private:
    std::string socketPath_;
    int socketFd_ = -1;
    bool connected_ = false;

    void readLoop();
    void writeMessage(const IPCMessage& message);
};

// TCP socket channel
class TCPSocketChannel : public IPCChannel {
public:
    TCPSocketChannel(const std::string& host, int port);
    ~TCPSocketChannel() override;

    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;

    void send(const IPCMessage& message) override;
    void send(const std::string& channel, const std::string& data) override;
    void setMessageCallback(MessageCallback callback) override;

private:
    std::string host_;
    int port_;
    int socketFd_ = -1;
    bool connected_ = false;

    void readLoop();
    void writeMessage(const IPCMessage& message);
};

// Message protocol
namespace IPCProtocol {
    std::string serializeMessage(const IPCMessage& msg);
    IPCMessage deserializeMessage(const std::string& data);
    
    std::string createRequest(const std::string& channel, const std::string& data);
    std::string createResponse(const std::string& messageId, const std::string& data);
    std::string createError(const std::string& messageId, const std::string& error);
    std::string createEvent(const std::string& channel, const std::string& data);
}

} // namespace NexusForge::Extensions
