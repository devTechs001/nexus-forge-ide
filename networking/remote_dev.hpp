// networking/remote_dev.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <ctime>

namespace NexusForge::Networking {

// Remote connection config
struct RemoteConfig {
    std::string connectionString;
    std::string host;
    int port = 22;
    std::string username;
    std::string password;
    std::string privateKeyPath;
    std::string remotePath;
    std::string name;
    bool savePassword = false;
};

struct ConnectionInfo {
    std::string user;
    std::string host;
    int port = 22;
};

// Remote file info
struct RemoteFile {
    std::string name;
    std::string path;
    bool isDirectory = false;
    size_t size = 0;
    uint64_t modifiedTime = 0;
    std::time_t lastModified = 0;
    std::string permissions;
    std::string owner;
    std::string group;
};
using RemoteFileInfo = RemoteFile;

struct RemoteCommandResult {
    int exitCode = 0;
    std::string output;
    std::string error;
};

struct RemoteProcess {
    int pid = -1;
    std::string command;
    bool running = false;
};

struct RemoteProcessInfo {
    int pid = -1;
    bool running = false;
    double cpuUsage = 0.0;
    size_t memoryUsage = 0;
};

struct LocalForward {
    int localPort = 0;
    std::string remoteHost;
    int remotePort = 0;
    bool active = false;
};

struct RemoteForward {
    int remotePort = 0;
    std::string localHost;
    int localPort = 0;
    bool active = false;
};

// Full-featured RemoteDevelopment controller
class RemoteDevelopment {
public:
    using CommandCallback = std::function<void(const RemoteCommandResult&)>;

    RemoteDevelopment();
    ~RemoteDevelopment();

    bool connect(const RemoteConfig& config);
    void disconnect();
    bool isConnected() const;
    RemoteConfig getConfig() const;

    // File operations
    bool uploadFile(const std::string& localPath, const std::string& remotePath);
    bool downloadFile(const std::string& remotePath, const std::string& localPath);
    bool deleteFile(const std::string& remotePath);
    bool fileExists(const std::string& remotePath);
    RemoteFileInfo getFileInfo(const std::string& remotePath);
    std::vector<RemoteFileInfo> listDirectory(const std::string& remotePath);
    bool createDirectory(const std::string& remotePath);
    bool deleteDirectory(const std::string& remotePath);
    std::string uploadContent(const std::string& remotePath, const std::string& content);
    std::string downloadContent(const std::string& remotePath);

    // Command execution
    RemoteCommandResult executeCommand(const std::string& command);
    void executeCommandAsync(const std::string& command, CommandCallback callback);

    // Process management
    RemoteProcess startProcess(const std::string& command,
                               const std::vector<std::string>& args = {},
                               const std::string& cwd = "");
    void killProcess(int pid);
    RemoteProcessInfo getProcessInfo(int pid);
    std::vector<RemoteProcessInfo> listProcesses();

    // Port forwarding
    bool startLocalForwarding(int localPort, const std::string& remoteHost, int remotePort);
    bool startRemoteForwarding(int remotePort, const std::string& localHost, int localPort);
    void stopLocalForwarding(int localPort);
    void stopRemoteForwarding(int remotePort);
    const std::vector<LocalForward>& getLocalForwards() const;
    const std::vector<RemoteForward>& getRemoteForwards() const;

    // Environment
    std::unordered_map<std::string, std::string> getEnvironment();
    std::string getWorkingDirectory();
    bool setWorkingDirectory(const std::string& path);

    // Helpers
    static ConnectionInfo parseConnection(const std::string& connectionString);

    // Event setters
    void setOnConnected(std::function<void()> callback);
    void setOnDisconnected(std::function<void()> callback);
    void setOnFileChanged(std::function<void(const std::string&)> callback);
    void setOnError(std::function<void(const std::string&)> callback);

    std::function<void()> onConnected;
    std::function<void()> onDisconnected;
    std::function<void(const std::string&)> onFileChanged;
    std::function<void(const std::string&)> onError;

private:
    bool connected_ = false;
    RemoteConfig config_;
    std::vector<LocalForward> localForwards_;
    std::vector<RemoteForward> remoteForwards_;

    std::string getFileName(const std::string& path) const;
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
