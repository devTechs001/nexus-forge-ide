// terminal/shell_integration.hpp
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace NexusForge::Terminal {

// Shell type
enum class ShellType {
    Unknown,
    Bash,
    Zsh,
    Fish,
    PowerShell,
    Cmd,
    Custom
};

// Command detection
struct DetectedCommand {
    std::string name;
    std::string fullPath;
    std::vector<std::string> args;
    int exitCode = 0;
    double duration = 0;  // seconds
    bool succeeded = true;
};

struct CommandHistoryEntry {
    std::string command;
    int exitCode = 0;
    int durationMs = 0;
    std::chrono::system_clock::time_point timestamp{};
    std::string directory;
};

// Shell integration
class ShellIntegration {
public:
    using CommandStartedCallback = std::function<void(const std::string&)>;
    using CommandFinishedCallback = std::function<void(const CommandHistoryEntry&)>;
    using DirectoryChangedCallback = std::function<void(const std::string&)>;

    ShellIntegration();
    ~ShellIntegration();

    // Initialize
    bool initialize(ShellType shell);
    void shutdown();

    // Script generation
    std::string generateIntegrationScript(ShellType shellType);
    std::string generateBashScript();
    std::string generateZshScript();
    std::string generateFishScript();
    std::string generatePowerShellScript();

    // Shell type
    ShellType getShellType() const { return shellType_; }
    std::string getShellName() const;

    // Lifecycle hooks
    void onCommandStarted(const std::string& command);
    void onCommandFinished(int exitCode, int durationMs);
    void onDirectoryChanged(const std::string& directory);

    void setOutputCallback(CommandStartedCallback callback);
    void setCommandFinishedCallback(CommandFinishedCallback callback);
    void setDirectoryChangedCallback(DirectoryChangedCallback callback);

    // Output parsing
    DetectedCommand parseCommand(const std::string& output);
    std::string extractCwd(const std::string& output);
    std::string extractExitCode(const std::string& output);

    // Prompt detection
    bool isPrompt(const std::string& line) const;
    std::string getPromptPattern() const;

    // Command history & suggestions
    const std::vector<CommandHistoryEntry>& getCommandHistory() const;
    std::vector<CommandHistoryEntry> searchHistory(const std::string& query) const;
    std::vector<CommandHistoryEntry> getRecentCommands(size_t count) const;
    void clearHistory();
    bool saveHistory(const std::string& filePath) const;
    bool loadHistory(const std::string& filePath);
    std::vector<std::string> getSuggestions(const std::string& prefix);

    // OSC 7 (file URLs)
    std::string createFileUrl(const std::string& path);
    std::string parseFileUrl(const std::string& url);

    // OSC 1337 (iTerm2)
    std::string createCurrentDirOsc(const std::string& path);
    std::string createPromptMarkOsc();

    // Static helpers
    static ShellType detectShell(const std::string& shellPath);
    static std::string getDefaultShell();

private:
    ShellType shellType_ = ShellType::Unknown;
    std::string promptPattern_;
    std::string currentCommand_;
    std::string currentDirectory_;
    std::chrono::steady_clock::time_point commandStartTime_{};
    std::vector<CommandHistoryEntry> commandHistory_;
    size_t maxHistorySize_ = 10000;

    CommandStartedCallback onCommandStartedCallback;
    CommandFinishedCallback onCommandFinishedCallback;
    DirectoryChangedCallback onDirectoryChangedCallback;

    void setupBash();
    void setupZsh();
    void setupFish();
    void setupPowerShell();
    void setupCmd();
};

// Shell script generator
class ShellScriptGenerator {
public:
    static std::string getInitScript(ShellType shell);
    static std::string getPromptFunction(ShellType shell);
    static std::string getPreExecFunction(ShellType shell);
    static std::string getPostExecFunction(ShellType shell);
};

} // namespace NexusForge::Terminal
