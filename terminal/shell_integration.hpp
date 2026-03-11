// terminal/shell_integration.hpp
#pragma once

#include <string>
#include <vector>
#include <functional>

namespace NexusForge::Terminal {

// Shell type
enum class ShellType {
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

// Shell integration
class ShellIntegration {
public:
    ShellIntegration();
    ~ShellIntegration();

    // Initialize
    bool initialize(ShellType shell);
    void shutdown();

    // Shell type
    ShellType getShellType() const { return shellType_; }
    std::string getShellName() const;

    // Output parsing
    DetectedCommand parseCommand(const std::string& output);
    std::string extractCwd(const std::string& output);
    std::string extractExitCode(const std::string& output);

    // Prompt detection
    bool isPrompt(const std::string& line) const;
    std::string getPromptPattern() const;

    // Command suggestions
    std::vector<std::string> getSuggestions(const std::string& prefix);
    std::vector<std::string> getCommandHistory();

    // OSC 7 (file URLs)
    std::string createFileUrl(const std::string& path);
    std::string parseFileUrl(const std::string& url);

    // OSC 1337 (iTerm2)
    std::string createCurrentDirOsc(const std::string& path);
    std::string createPromptMarkOsc();

private:
    ShellType shellType_ = ShellType::Bash;
    std::string promptPattern_;
    std::vector<std::string> commandHistory_;

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
