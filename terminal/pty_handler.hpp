// terminal/pty_handler.hpp
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

namespace NexusForge::Terminal {

// Concrete POSIX PTY handler
class PTYHandler {
public:
    using OutputCallback = std::function<void(const std::string&)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    using ExitCallback = std::function<void(int)>;

    PTYHandler();
    ~PTYHandler();

    PTYHandler(const PTYHandler&) = delete;
    PTYHandler& operator=(const PTYHandler&) = delete;

    bool spawn(const std::string& shell,
               const std::vector<std::string>& args = {},
               const std::string& cwd = "",
               const std::unordered_map<std::string, std::string>& env = {},
               int cols = 80, int rows = 24);
    void close();

    void write(const std::string& data);
    void resize(int cols, int rows);
    void kill();

    bool isRunning() const;
    int getExitCode() const;
    int getPid() const { return childPid_; }

    void setOutputCallback(OutputCallback callback);
    void setErrorCallback(ErrorCallback callback);
    void setExitCallback(ExitCallback callback);

    std::string getWorkingDirectory() const;
    std::string getProcessTitle() const;

private:
    int masterFd_ = -1;
    int slaveFd_ = -1;
    int childPid_ = -1;
    mutable std::atomic<bool> running_{false};
    mutable std::atomic<bool> reaped_{false};
    mutable std::atomic<int> exitCode_{0};

    std::thread readThread_;
    mutable std::mutex callbackMutex_;
    OutputCallback outputCallback_;
    ErrorCallback errorCallback_;
    ExitCallback exitCallback_;

    void childProcess(const std::string& shell,
                      const std::vector<std::string>& args,
                      const std::string& cwd,
                      const std::unordered_map<std::string, std::string>& env,
                      int cols, int rows);
    void readLoop();
};

// PTY configuration
struct PTYConfig {
    std::string shell;
    std::vector<std::string> args;
    std::string cwd;
    std::unordered_map<std::string, std::string> env;
    
    int columns = 80;
    int rows = 24;
    
    bool login = false;
    bool interactive = true;
    
    // Platform-specific
    std::string profile;  // For Windows Terminal profiles
};

// PTY process info
struct ProcessInfo {
    int pid = 0;
    std::string name;
    std::string cwd;
    std::string commandLine;
    int exitCode = 0;
    bool running = true;
};

// PTY handler interface
class IPtyHandler {
public:
    virtual ~IPtyHandler() = default;
    
    virtual bool initialize(const PTYConfig& config) = 0;
    virtual void shutdown() = 0;
    
    virtual void write(const std::string& data) = 0;
    virtual void resize(int columns, int rows) = 0;
    
    virtual ProcessInfo getProcessInfo() const = 0;
    virtual bool isRunning() const = 0;
    virtual int getPid() const = 0;
    virtual int getExitCode() const = 0;
    
    using DataCallback = std::function<void(const std::string&)>;
    using ExitCallback = std::function<void(int)>;
    
    virtual void setDataCallback(DataCallback callback) = 0;
    virtual void setExitCallback(ExitCallback callback) = 0;
};

// Unix PTY handler
class UnixPtyHandler : public IPtyHandler {
public:
    UnixPtyHandler();
    ~UnixPtyHandler() override;
    
    bool initialize(const PTYConfig& config) override;
    void shutdown() override;
    
    void write(const std::string& data) override;
    void resize(int columns, int rows) override;
    
    ProcessInfo getProcessInfo() const override;
    bool isRunning() const override;
    int getPid() const override;
    int getExitCode() const override;
    
    void setDataCallback(DataCallback callback) override;
    void setExitCallback(ExitCallback callback) override;
    
private:
    PTYHandler handler_;
    PTYConfig config_;
    DataCallback dataCallback_;
    ExitCallback exitCallback_;
};

// Windows PTY handler (ConPTY)
class WindowsPtyHandler : public IPtyHandler {
public:
    WindowsPtyHandler();
    ~WindowsPtyHandler() override;
    
    bool initialize(const PTYConfig& config) override;
    void shutdown() override;
    
    void write(const std::string& data) override;
    void resize(int columns, int rows) override;
    
    ProcessInfo getProcessInfo() const override;
    bool isRunning() const override;
    int getPid() const override;
    int getExitCode() const override;
    
    void setDataCallback(DataCallback callback) override;
    void setExitCallback(ExitCallback callback) override;
    
private:
    void* hInput_ = nullptr;
    void* hOutput_ = nullptr;
    void* hProcess_ = nullptr;
    void* pty_ = nullptr;
    
    int pid_ = -1;
    bool running_ = false;
    int exitCode_ = 0;
    
    PTYConfig config_;
    DataCallback dataCallback_;
    ExitCallback exitCallback_;
    
    void readLoop();
    void waitForExit();
};

// Factory function
std::unique_ptr<IPtyHandler> createPtyHandler();

// Shell utilities
namespace Shell {
    std::string getDefaultShell();
    std::vector<std::string> getDefaultShellArgs();
    std::string getUserHome();
    std::string expandEnvironmentVariables(const std::string& str);
}

} // namespace NexusForge::Terminal
