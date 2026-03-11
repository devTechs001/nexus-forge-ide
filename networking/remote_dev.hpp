// networking/remote_dev.hpp
#pragma once

#include <string>
#include <vector>
#include <functional>

namespace NexusForge::Networking {

// Remote connection config
struct RemoteConfig {
    std::string host;
    int port = 22;
    std::string username;
    std::string password;
    std::string privateKeyPath;
    std::string remotePath;
    std::string name;
    bool savePassword = false;
};

// Remote file info
struct RemoteFile {
    std::string name;
    std::string path;
    bool isDirectory = false;
    size_t size = 0;
    uint64_t modifiedTime = 0;
    std::string permissions;
    std::string owner;
    std::string group;
};

// Remote development
class RemoteDev {
public:
    RemoteDev();
    ~RemoteDev();

    // Connection
    bool connect(const RemoteConfig& config);
    void disconnect();
    bool isConnected() const { return connected_; }

    // File operations
    std::vector<RemoteFile> listDirectory(const std::string& path);
    bool downloadFile(const std::string& remotePath, const std::string& localPath);
    bool uploadFile(const std::string& localPath, const std::string& remotePath);
    bool deleteFile(const std::string& path);
    bool createDirectory(const std::string& path);
    bool renameFile(const std::string& oldPath, const std::string& newPath);

    // Execute command
    std::string executeCommand(const std::string& command);
    void executeCommandAsync(const std::string& command,
                              std::function<void(const std::string&)> callback);

    // Port forwarding
    bool startPortForward(int localPort, const std::string& remoteHost, int remotePort);
    void stopPortForward(int localPort);

    // Sync
    void startSync(const std::string& localPath, const std::string& remotePath);
    void stopSync();
    bool isSyncing() const { return syncing_; }

    // Config management
    void saveConfig(const RemoteConfig& config);
    std::vector<RemoteConfig> getSavedConfigs() const;
    bool deleteConfig(const std::string& name);

    // Events
    using ConnectionCallback = std::function<void(bool)>;
    using FileCallback = std::function<void(const std::string&)>;
    using OutputCallback = std::function<void(const std::string&)>;

    void onConnected(ConnectionCallback callback);
    void onDisconnected(ConnectionCallback callback);
    void onFileChanged(FileCallback callback);
    void onCommandOutput(OutputCallback callback);

private:
    bool connected_ = false;
    bool syncing_ = false;
    RemoteConfig config_;

    void* sshSession_ = nullptr;
    void* sftpSession_ = nullptr;

    std::vector<ConnectionCallback> connectedCallbacks_;
    std::vector<ConnectionCallback> disconnectedCallbacks_;
    std::vector<FileCallback> fileChangedCallbacks_;
    std::vector<OutputCallback> commandOutputCallbacks_;

    bool connectSSH();
    bool connectSFTP();
    void disconnectSSH();
    void disconnectSFTP();
};

} // namespace NexusForge::Networking
