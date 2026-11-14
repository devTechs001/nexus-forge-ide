// terminal/pty_handler.cpp
// PTY Handler Implementation

#include "pty_handler.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <chrono>

namespace NexusForge::Terminal {

PTYHandler::PTYHandler() : masterFd_(-1), slaveFd_(-1), childPid_(-1), running_(false) {
}

PTYHandler::~PTYHandler() {
    close();
}

bool PTYHandler::spawn(const std::string& shell,
                        const std::vector<std::string>& args,
                        const std::string& cwd,
                        const std::unordered_map<std::string, std::string>& env,
                        int cols, int rows) {
    // Create PTY
    if (posix_openpty(&masterFd_, &slaveFd_, nullptr, nullptr, nullptr) != 0) {
        return false;
    }
    
    // Fork process
    childPid_ = fork();
    
    if (childPid_ < 0) {
        ::close(masterFd_);
        ::close(slaveFd_);
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
    fcntl(masterFd_, F_SETFL, flags | O_NONBLOCK);
    
    // Resize PTY
    resize(cols, rows);
    
    running_ = true;
    
    // Start read thread
    readThread_ = std::thread(&PTYHandler::readLoop, this);
    
    return true;
}

void PTYHandler::childProcess(const std::string& shell,
                               const std::vector<std::string>& args,
                               const std::string& cwd,
                               const std::unordered_map<std::string, std::string>& env,
                               int cols, int rows) {
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
        chdir(cwd.c_str());
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
}

void PTYHandler::close() {
    running_ = false;
    
    if (readThread_.joinable()) {
        readThread_.join();
    }
    
    if (childPid_ > 0) {
        kill(childPid_, SIGTERM);
        waitpid(childPid_, nullptr, 0);
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
}

void PTYHandler::write(const std::string& data) {
    if (!running_ || masterFd_ < 0) return;
    
    ::write(masterFd_, data.c_str(), data.size());
}

void PTYHandler::resize(int cols, int rows) {
    if (masterFd_ < 0) return;
    
    struct winsize ws;
    ws.ws_col = cols;
    ws.ws_row = rows;
    ws.ws_xpixel = cols * 8;
    ws.ws_ypixel = rows * 16;
    
    ioctl(masterFd_, TIOCSWINSZ, &ws);
    
    if (childPid_ > 0) {
        kill(childPid_, SIGWINCH);
    }
}

void PTYHandler::kill() {
    if (childPid_ > 0) {
        kill(childPid_, SIGKILL);
    }
}

bool PTYHandler::isRunning() const {
    if (childPid_ <= 0) return false;
    
    int status;
    pid_t result = waitpid(childPid_, &status, WNOHANG);
    
    if (result == 0) {
        return true;  // Still running
    }
    
    return false;
}

int PTYHandler::getExitCode() const {
    return exitCode_;
}

void PTYHandler::setOutputCallback(OutputCallback callback) {
    outputCallback_ = std::move(callback);
}

void PTYHandler::setErrorCallback(ErrorCallback callback) {
    errorCallback_ = std::move(callback);
}

void PTYHandler::setExitCallback(ExitCallback callback) {
    exitCallback_ = std::move(callback);
}

void PTYHandler::readLoop() {
    char buffer[4096];
    
    while (running_) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(masterFd_, &readfds);
        
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;  // 100ms
        
        int result = select(masterFd_ + 1, &readfds, nullptr, nullptr, &timeout);
        
        if (result > 0 && FD_ISSET(masterFd_, &readfds)) {
            ssize_t bytesRead = ::read(masterFd_, buffer, sizeof(buffer));
            
            if (bytesRead > 0) {
                if (outputCallback_) {
                    outputCallback_(std::string(buffer, bytesRead));
                }
            } else if (bytesRead == 0) {
                // EOF - process terminated
                running_ = false;
                if (exitCallback_) {
                    exitCallback_(exitCode_);
                }
                break;
            } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                // Error
                if (errorCallback_) {
                    errorCallback_(strerror(errno));
                }
                running_ = false;
                break;
            }
        }
        
        // Check if child process is still running
        if (!isRunning()) {
            running_ = false;
            if (exitCallback_) {
                exitCallback_(exitCode_);
            }
            break;
        }
    }
}

std::string PTYHandler::getWorkingDirectory() const {
    if (childPid_ <= 0) return "";
    
    char path[4096];
    std::string cwdPath = "/proc/" + std::to_string(childPid_) + "/cwd";
    
    ssize_t len = readlink(cwdPath.c_str(), path, sizeof(path) - 1);
    if (len > 0) {
        path[len] = '\0';
        return std::string(path);
    }
    
    return "";
}

std::string PTYHandler::getProcessTitle() const {
    if (childPid_ <= 0) return "";
    
    char path[4096];
    std::string cmdPath = "/proc/" + std::to_string(childPid_) + "/cmdline";
    
    std::ifstream file(cmdPath);
    if (file.is_open()) {
        file.read(path, sizeof(path) - 1);
        ssize_t len = file.gcount();
        if (len > 0) {
            path[len] = '\0';
            // Replace null bytes with spaces
            for (int i = 0; i < len; ++i) {
                if (path[i] == '\0') path[i] = ' ';
            }
            return std::string(path);
        }
    }
    
    return "";
}

} // namespace NexusForge::Terminal
