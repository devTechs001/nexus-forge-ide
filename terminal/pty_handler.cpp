// terminal/pty_handler.cpp
// PTY Handler Implementation

#include "pty_handler.hpp"
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <fstream>
#include <thread>
#include <chrono>

#if defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#endif

namespace NexusForge::Terminal {

PTYHandler::PTYHandler()
    : masterFd_(-1), slaveFd_(-1), childPid_(-1), running_(false), reaped_(false), exitCode_(0) {
}

PTYHandler::~PTYHandler() {
    close();
}

bool PTYHandler::spawn(const std::string& shell,
                        const std::vector<std::string>& args,
                        const std::string& cwd,
                        const std::unordered_map<std::string, std::string>& env,
                        int cols, int rows) {
#if defined(__unix__) || defined(__APPLE__)
    close();

    // Open POSIX PTY master/slave pair (no extra -lutil dependency required)
    masterFd_ = posix_openpt(O_RDWR | O_NOCTTY);
    if (masterFd_ < 0) {
        return false;
    }
    if (grantpt(masterFd_) != 0 || unlockpt(masterFd_) != 0) {
        ::close(masterFd_);
        masterFd_ = -1;
        return false;
    }

    char slaveName[256] = {};
#ifdef __APPLE__
    const char* namePtr = ptsname(masterFd_);
    if (!namePtr) {
        ::close(masterFd_);
        masterFd_ = -1;
        return false;
    }
    std::strncpy(slaveName, namePtr, sizeof(slaveName) - 1);
#else
    if (ptsname_r(masterFd_, slaveName, sizeof(slaveName)) != 0) {
        ::close(masterFd_);
        masterFd_ = -1;
        return false;
    }
#endif

    slaveFd_ = ::open(slaveName, O_RDWR | O_NOCTTY);
    if (slaveFd_ < 0) {
        ::close(masterFd_);
        masterFd_ = -1;
        return false;
    }

    reaped_ = false;
    exitCode_ = 0;

    // Fork process
    childPid_ = fork();
    if (childPid_ < 0) {
        ::close(masterFd_);
        ::close(slaveFd_);
        masterFd_ = -1;
        slaveFd_ = -1;
        return false;
    }

    if (childPid_ == 0) {
        // Child process
        childProcess(shell, args, cwd, env, cols, rows);
    }

    // Parent process
    ::close(slaveFd_);
    slaveFd_ = -1;

    // Set non-blocking
    int flags = fcntl(masterFd_, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(masterFd_, F_SETFL, flags | O_NONBLOCK);
    }

    // Resize PTY
    resize(cols, rows);

    running_ = true;

    // Start read thread
    readThread_ = std::thread(&PTYHandler::readLoop, this);

    return true;
#else
    (void)shell; (void)args; (void)cwd; (void)env; (void)cols; (void)rows;
    return false;
#endif
}

void PTYHandler::childProcess(const std::string& shell,
                               const std::vector<std::string>& args,
                               const std::string& cwd,
                               const std::unordered_map<std::string, std::string>& env,
                               int cols, int rows) {
#if defined(__unix__) || defined(__APPLE__)
    (void)cols;
    (void)rows;

    // Become session leader
    setsid();

    // Set controlling terminal
    ioctl(slaveFd_, TIOCSCTTY, 0);

    // Set standard streams
    dup2(slaveFd_, STDIN_FILENO);
    dup2(slaveFd_, STDOUT_FILENO);
    dup2(slaveFd_, STDERR_FILENO);

    ::close(masterFd_);
    ::close(slaveFd_);

    // Change directory
    if (!cwd.empty()) {
        if (chdir(cwd.c_str()) != 0) {
            // Keep inherited working directory if chdir fails
        }
    }

    // Set environment
    for (const auto& [key, value] : env) {
        setenv(key.c_str(), value.c_str(), 1);
    }

    // Set default environment variables
    setenv("TERM", "xterm-256color", 1);
    setenv("COLORTERM", "truecolor", 1);

    // Build argument list
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(shell.c_str()));
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    // Execute shell
    execvp(shell.c_str(), argv.data());

    // If exec fails
    _exit(127);
#else
    (void)shell; (void)args; (void)cwd; (void)env; (void)cols; (void)rows;
#endif
}

void PTYHandler::close() {
    running_ = false;

    if (readThread_.joinable()) {
        readThread_.join();
    }

#if defined(__unix__) || defined(__APPLE__)
    if (childPid_ > 0) {
        if (!reaped_.load()) {
            ::kill(childPid_, SIGTERM);
            int status = 0;
            if (waitpid(childPid_, &status, 0) == childPid_) {
                reaped_ = true;
                if (WIFEXITED(status)) {
                    exitCode_ = WEXITSTATUS(status);
                } else if (WIFSIGNALED(status)) {
                    exitCode_ = 128 + WTERMSIG(status);
                }
            }
        }
        childPid_ = -1;
    }

    if (masterFd_ >= 0) {
        ::close(masterFd_);
        masterFd_ = -1;
    }

    if (slaveFd_ >= 0) {
        ::close(slaveFd_);
        slaveFd_ = -1;
    }
#endif
}

void PTYHandler::write(const std::string& data) {
#if defined(__unix__) || defined(__APPLE__)
    if (!running_.load() || masterFd_ < 0 || data.empty()) return;

    size_t totalWritten = 0;
    while (totalWritten < data.size()) {
        ssize_t n = ::write(masterFd_, data.data() + totalWritten, data.size() - totalWritten);
        if (n > 0) {
            totalWritten += static_cast<size_t>(n);
        } else if (n < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }
#else
    (void)data;
#endif
}

void PTYHandler::resize(int cols, int rows) {
#if defined(__unix__) || defined(__APPLE__)
    if (masterFd_ < 0 || cols <= 0 || rows <= 0) return;

    struct winsize ws{};
    ws.ws_col = static_cast<unsigned short>(cols);
    ws.ws_row = static_cast<unsigned short>(rows);
    ws.ws_xpixel = static_cast<unsigned short>(cols * 8);
    ws.ws_ypixel = static_cast<unsigned short>(rows * 16);

    ioctl(masterFd_, TIOCSWINSZ, &ws);

    if (childPid_ > 0 && !reaped_.load()) {
        ::kill(childPid_, SIGWINCH);
    }
#else
    (void)cols; (void)rows;
#endif
}

void PTYHandler::kill() {
#if defined(__unix__) || defined(__APPLE__)
    if (childPid_ > 0 && !reaped_.load()) {
        ::kill(childPid_, SIGKILL);
    }
#endif
}

bool PTYHandler::isRunning() const {
#if defined(__unix__) || defined(__APPLE__)
    if (childPid_ <= 0 || reaped_.load()) return false;

    int status = 0;
    pid_t result = waitpid(childPid_, &status, WNOHANG);

    if (result == 0) {
        return true;  // Still running
    }

    if (result == childPid_) {
        reaped_ = true;
        if (WIFEXITED(status)) {
            exitCode_ = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            exitCode_ = 128 + WTERMSIG(status);
        } else {
            exitCode_ = -1;
        }
    }

    return false;
#else
    return running_.load();
#endif
}

int PTYHandler::getExitCode() const {
    return exitCode_.load();
}

void PTYHandler::setOutputCallback(OutputCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    outputCallback_ = std::move(callback);
}

void PTYHandler::setErrorCallback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    errorCallback_ = std::move(callback);
}

void PTYHandler::setExitCallback(ExitCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    exitCallback_ = std::move(callback);
}

void PTYHandler::readLoop() {
#if defined(__unix__) || defined(__APPLE__)
    char buffer[4096];

    while (running_.load() && masterFd_ >= 0) {
        struct pollfd pfd{};
        pfd.fd = masterFd_;
        pfd.events = POLLIN;

        int result = poll(&pfd, 1, 100);  // 100ms timeout

        if (result > 0 && (pfd.revents & (POLLIN | POLLHUP))) {
            ssize_t bytesRead = ::read(masterFd_, buffer, sizeof(buffer));

            if (bytesRead > 0) {
                OutputCallback cb;
                {
                    std::lock_guard<std::mutex> lock(callbackMutex_);
                    cb = outputCallback_;
                }
                if (cb) {
                    cb(std::string(buffer, static_cast<size_t>(bytesRead)));
                }
            } else if (bytesRead == 0 || (bytesRead < 0 && errno == EIO)) {
                // EOF / slave closed (EIO is normal on Linux PTY when child exits)
                (void)isRunning();
                running_ = false;
                ExitCallback cb;
                {
                    std::lock_guard<std::mutex> lock(callbackMutex_);
                    cb = exitCallback_;
                }
                if (cb) {
                    cb(exitCode_.load());
                }
                break;
            } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                // Error
                ErrorCallback cb;
                {
                    std::lock_guard<std::mutex> lock(callbackMutex_);
                    cb = errorCallback_;
                }
                if (cb) {
                    cb(std::strerror(errno));
                }
                running_ = false;
                break;
            }
        }

        // Check if child process is still running
        if (!isRunning()) {
            // Drain any remaining buffered bytes before signaling exit
            ssize_t bytesRead = ::read(masterFd_, buffer, sizeof(buffer));
            while (bytesRead > 0) {
                OutputCallback cb;
                {
                    std::lock_guard<std::mutex> lock(callbackMutex_);
                    cb = outputCallback_;
                }
                if (cb) {
                    cb(std::string(buffer, static_cast<size_t>(bytesRead)));
                }
                bytesRead = ::read(masterFd_, buffer, sizeof(buffer));
            }
            running_ = false;
            ExitCallback cb;
            {
                std::lock_guard<std::mutex> lock(callbackMutex_);
                cb = exitCallback_;
            }
            if (cb) {
                cb(exitCode_.load());
            }
            break;
        }
    }
#endif
}

std::string PTYHandler::getWorkingDirectory() const {
#if defined(__unix__)
    if (childPid_ <= 0) return "";

    char path[4096];
    std::string cwdPath = "/proc/" + std::to_string(childPid_) + "/cwd";

    ssize_t len = readlink(cwdPath.c_str(), path, sizeof(path) - 1);
    if (len > 0) {
        path[len] = '\0';
        return std::string(path);
    }
#endif
    return "";
}

std::string PTYHandler::getProcessTitle() const {
#if defined(__unix__)
    if (childPid_ <= 0) return "";

    char path[4096];
    std::string cmdPath = "/proc/" + std::to_string(childPid_) + "/cmdline";

    std::ifstream file(cmdPath);
    if (file.is_open()) {
        file.read(path, sizeof(path) - 1);
        std::streamsize len = file.gcount();
        if (len > 0) {
            path[len] = '\0';
            for (std::streamsize i = 0; i < len; ++i) {
                if (path[i] == '\0') path[i] = ' ';
            }
            return std::string(path);
        }
    }
#endif
    return "";
}

// UnixPtyHandler implementation delegating to PTYHandler
UnixPtyHandler::UnixPtyHandler() = default;
UnixPtyHandler::~UnixPtyHandler() { shutdown(); }

bool UnixPtyHandler::initialize(const PTYConfig& config) {
    config_ = config;
    std::string shell = config.shell.empty() ? Shell::getDefaultShell() : config.shell;
    if (dataCallback_) handler_.setOutputCallback(dataCallback_);
    if (exitCallback_) handler_.setExitCallback(exitCallback_);
    return handler_.spawn(shell, config.args, config.cwd, config.env, config.columns, config.rows);
}

void UnixPtyHandler::shutdown() { handler_.close(); }
void UnixPtyHandler::write(const std::string& data) { handler_.write(data); }
void UnixPtyHandler::resize(int columns, int rows) { handler_.resize(columns, rows); }

ProcessInfo UnixPtyHandler::getProcessInfo() const {
    ProcessInfo info;
    info.pid = handler_.getPid();
    info.cwd = handler_.getWorkingDirectory();
    info.commandLine = handler_.getProcessTitle();
    info.name = config_.shell;
    info.exitCode = handler_.getExitCode();
    info.running = handler_.isRunning();
    return info;
}

bool UnixPtyHandler::isRunning() const { return handler_.isRunning(); }
int UnixPtyHandler::getPid() const { return handler_.getPid(); }
int UnixPtyHandler::getExitCode() const { return handler_.getExitCode(); }

void UnixPtyHandler::setDataCallback(DataCallback callback) {
    dataCallback_ = std::move(callback);
    handler_.setOutputCallback(dataCallback_);
}

void UnixPtyHandler::setExitCallback(ExitCallback callback) {
    exitCallback_ = std::move(callback);
    handler_.setExitCallback(exitCallback_);
}

// WindowsPtyHandler stub implementation for non-Windows hosts
WindowsPtyHandler::WindowsPtyHandler() = default;
WindowsPtyHandler::~WindowsPtyHandler() { shutdown(); }
bool WindowsPtyHandler::initialize(const PTYConfig& config) { config_ = config; return false; }
void WindowsPtyHandler::shutdown() { running_ = false; }
void WindowsPtyHandler::write(const std::string& data) { (void)data; }
void WindowsPtyHandler::resize(int columns, int rows) { (void)columns; (void)rows; }
ProcessInfo WindowsPtyHandler::getProcessInfo() const { return ProcessInfo{pid_, config_.shell, config_.cwd, "", exitCode_, running_}; }
bool WindowsPtyHandler::isRunning() const { return running_; }
int WindowsPtyHandler::getPid() const { return pid_; }
int WindowsPtyHandler::getExitCode() const { return exitCode_; }
void WindowsPtyHandler::setDataCallback(DataCallback callback) { dataCallback_ = std::move(callback); }
void WindowsPtyHandler::setExitCallback(ExitCallback callback) { exitCallback_ = std::move(callback); }
void WindowsPtyHandler::readLoop() {}
void WindowsPtyHandler::waitForExit() {}

std::unique_ptr<IPtyHandler> createPtyHandler() {
#ifdef _WIN32
    return std::make_unique<WindowsPtyHandler>();
#else
    return std::make_unique<UnixPtyHandler>();
#endif
}

namespace Shell {

std::string getDefaultShell() {
#ifdef _WIN32
    const char* comspec = std::getenv("COMSPEC");
    return comspec ? std::string(comspec) : "cmd.exe";
#else
    const char* shell = std::getenv("SHELL");
    return (shell && shell[0] != '\0') ? std::string(shell) : "/bin/sh";
#endif
}

std::vector<std::string> getDefaultShellArgs() {
    return {};
}

std::string getUserHome() {
    const char* home = std::getenv("HOME");
    if (home && home[0] != '\0') return std::string(home);
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile && userProfile[0] != '\0') return std::string(userProfile);
    return "/";
}

std::string expandEnvironmentVariables(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '$') {
            std::string varName;
            size_t next = i + 1;
            if (next < str.size() && str[next] == '{') {
                size_t close = str.find('}', next + 1);
                if (close != std::string::npos) {
                    varName = str.substr(next + 1, close - next - 1);
                    i = close;
                }
            } else {
                while (next < str.size() && (std::isalnum(static_cast<unsigned char>(str[next])) || str[next] == '_')) {
                    varName += str[next++];
                }
                i = next - 1;
            }
            if (!varName.empty()) {
                const char* val = std::getenv(varName.c_str());
                if (val) result += val;
            } else {
                result += '$';
            }
        } else {
            result += str[i];
        }
    }
    return result;
}

} // namespace Shell

} // namespace NexusForge::Terminal
