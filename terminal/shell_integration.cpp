// terminal/shell_integration.cpp
// Shell Integration Implementation

#include "shell_integration.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <iostream>

namespace NexusForge::Terminal {

ShellIntegration::ShellIntegration() : shellType_(ShellType::Unknown) {
}

ShellIntegration::~ShellIntegration() = default;

bool ShellIntegration::initialize(ShellType shellType) {
    shellType_ = shellType;
    
    // Generate and inject shell integration script
    std::string script = generateIntegrationScript(shellType);
    
    return !script.empty();
}

std::string ShellIntegration::generateIntegrationScript(ShellType shellType) {
    switch (shellType) {
        case ShellType::Bash:
            return generateBashScript();
        case ShellType::Zsh:
            return generateZshScript();
        case ShellType::Fish:
            return generateFishScript();
        case ShellType::PowerShell:
            return generatePowerShellScript();
        default:
            return generateBashScript();
    }
}

std::string ShellIntegration::generateBashScript() {
    std::stringstream script;
    
    script << R"(
# NexusForge Shell Integration for Bash

# Prompt prefix/suffix for command detection
export NEXUSFORGE_PROMPT_START="\e]133;A\e\\"
export NEXUSFORGE_PROMPT_END="\e]133;B\e\\"
export NEXUSFORGE_COMMAND_START="\e]133;C\e\\"
export NEXUSFORGE_COMMAND_END="\e]133;D\e\\"

# Original prompt
export NEXUSFORGE_ORIGINAL_PS1="$PS1"
export NEXUSFORGE_ORIGINAL_PS2="$PS2"

# Enhanced prompt with shell integration
PS1="${NEXUSFORGE_PROMPT_START}${NEXUSFORGE_ORIGINAL_PS1}${NEXUSFORGE_PROMPT_END}"
PS2="${NEXUSFORGE_ORIGINAL_PS2}"

# Function to report command start
__nexusforge_command_start() {
    printf '\e]133;C\e\\'
    NEXUSFORGE_COMMAND_START_TIME=$(date +%s.%N)
}

# Function to report command end
__nexusforge_command_end() {
    local exit_code=$?
    printf '\e]133;D;%d\e\\' "$exit_code"
    
    # Report timing
    if [ -n "$NEXUSFORGE_COMMAND_START_TIME" ]; then
        local end_time=$(date +%s.%N)
        local duration=$(echo "$end_time - $NEXUSFORGE_COMMAND_START_TIME" | bc 2>/dev/null || echo "0")
        printf '\e]133;T;%s\e\\' "$duration"
    fi
    
    # Report current working directory
    printf '\e]1337;CurrentDir=%s\e\\' "$PWD"
    
    return $exit_code
}

# Set up preexec/preexec-like functionality
if [ -n "$PROMPT_COMMAND" ]; then
    PROMPT_COMMAND="__nexusforge_command_end; ${PROMPT_COMMAND}"
else
    PROMPT_COMMAND="__nexusforge_command_end"
fi

# Trap DEBUG for command start
trap '__nexusforge_command_start' DEBUG

# Function to set title
__nexusforge_set_title() {
    printf '\e]2;%s\e\\' "$1"
}

# Function to set current directory
__nexusforge_set_cwd() {
    printf '\e]1337;CurrentDir=%s\e\\' "$PWD"
}

# Initial cwd report
__nexusforge_set_cwd

# History integration
__nexusforge_save_history() {
    history -a
}

# Quick cd with history
__nexusforge_cd() {
    cd "$@" && __nexusforge_set_cwd
}

alias cd=__nexusforge_cd

# Report shell type
printf '\e]1337;ShellIntegration=1;shell=bash\e\\'
)" << std::endl;
    
    return script.str();
}

std::string ShellIntegration::generateZshScript() {
    std::stringstream script;
    
    script << R"(
# NexusForge Shell Integration for Zsh

# Prompt hooks
export NEXUSFORGE_PROMPT_START="\e]133;A\e\\"
export NEXUSFORGE_PROMPT_END="\e]133;B\e\\"
export NEXUSFORGE_COMMAND_START="\e]133;C\e\\"
export NEXUSFORGE_COMMAND_END="\e]133;D\e\\"

# Store original prompts
export NEXUSFORGE_ORIGINAL_PROMPT="%{$NEXUSFORGE_PROMPT_START%}%${NEXUSFORGE_ORIGINAL_PROMPT}%{$NEXUSFORGE_PROMPT_END%}"

# Pre-exec hook
_nexusforge_preexec() {
    printf '\e]133;C\e\\'
    NEXUSFORGE_COMMAND_START_TIME=$(date +%s.%N)
}

# Pre-prompt hook
_nexusforge_precmd() {
    local exit_code=$?
    printf '\e]133;D;%d\e\\' "$exit_code"
    
    # Report timing
    if [ -n "$NEXUSFORGE_COMMAND_START_TIME" ]; then
        local end_time=$(date +%s.%N)
        local duration=$(echo "$end_time - $NEXUSFORGE_COMMAND_START_TIME" | bc 2>/dev/null || echo "0")
        printf '\e]133;T;%s\e\\' "$duration"
    fi
    
    # Report current working directory
    printf '\e]1337;CurrentDir=%s\e\\' "$PWD"
}

# Register hooks
autoload -Uz add-zsh-hook
add-zsh-hook preexec _nexusforge_preexec
add-zsh-hook precmd _nexusforge_precmd

# Initial cwd report
printf '\e]1337;CurrentDir=%s\e\\' "$PWD"

# Report shell type
printf '\e]1337;ShellIntegration=1;shell=zsh\e\\'

# Smart cd
function cd() {
    builtin cd "$@" && printf '\e]1337;CurrentDir=%s\e\\' "$PWD"
}
)" << std::endl;
    
    return script.str();
}

std::string ShellIntegration::generateFishScript() {
    std::stringstream script;
    
    script << R"(
# NexusForge Shell Integration for Fish

# Prompt hooks
set -x NEXUSFORGE_PROMPT_START "\e]133;A\e\\"
set -x NEXUSFORGE_PROMPT_END "\e]133;B\e\\"
set -x NEXUSFORGE_COMMAND_START "\e]133;C\e\\"
set -x NEXUSFORGE_COMMAND_END "\e]133;D\e\\"

# Pre-exec hook
function _nexusforge_preexec --on-event fish_preexec
    printf '\e]133;C\e\\'
    set -g NEXUSFORGE_COMMAND_START_TIME (date +%s.%N)
end

# Prompt hook
function _nexusforge_prompt --on-event fish_prompt
    set -l last_status $status
    
    printf '\e]133;D;%d\e\\' "$last_status"
    
    # Report timing
    if set -q NEXUSFORGE_COMMAND_START_TIME
        set -l end_time (date +%s.%N)
        set -l duration (math "$end_time - $NEXUSFORGE_COMMAND_START_TIME" 2>/dev/null || echo "0")
        printf '\e]133;T;%s\e\\' "$duration"
    end
    
    # Report current working directory
    printf '\e]1337;CurrentDir=%s\e\\' "$PWD"
    
    return $last_status
end

# Initial cwd report
printf '\e]1337;CurrentDir=%s\e\\' "$PWD"

# Report shell type
printf '\e]1337;ShellIntegration=1;shell=fish\e\\'
)" << std::endl;
    
    return script.str();
}

std::string ShellIntegration::generatePowerShellScript() {
    std::stringstream script;
    
    script << R"(
# NexusForge Shell Integration for PowerShell

# Store original prompt
$script:NexusForge_OriginalPrompt = $function:prompt

# New prompt function
function global:prompt {
    # Report command end
    Write-Host "`e]133;D;$LASTEXITCODE`e\\" -NoNewline
    
    # Report current directory
    Write-Host "`e]1337;CurrentDir=$PWD`e\\" -NoNewline
    
    # Call original prompt
    & $script:NexusForge_OriginalPrompt
}

# Pre-execution hook
$script:NexusForge_OriginalExecutionContext = $ExecutionContext.InvokeCommand.PreCommandAction
$ExecutionContext.InvokeCommand.PreCommandAction = {
    param($CommandName, $CommandAst, $CommandArguments)
    
    # Report command start
    Write-Host "`e]133;C`e\\" -NoNewline
    
    # Store start time
    $script:NexusForge_CommandStartTime = Get-Date
}

# Report shell type
Write-Host "`e]1337;ShellIntegration=1;shell=powershell`e\\" -NoNewline
)" << std::endl;
    
    return script.str();
}

void ShellIntegration::onCommandStarted(const std::string& command) {
    currentCommand_ = command;
    commandStartTime_ = std::chrono::steady_clock::now();
    
    if (onCommandStartedCallback) {
        onCommandStartedCallback(command);
    }
}

void ShellIntegration::onCommandFinished(int exitCode, int durationMs) {
    CommandHistoryEntry entry;
    entry.command = currentCommand_;
    entry.exitCode = exitCode;
    entry.durationMs = durationMs;
    entry.timestamp = std::chrono::system_clock::now();
    entry.directory = currentDirectory_;
    
    commandHistory_.push_back(entry);
    
    // Limit history size
    if (commandHistory_.size() > maxHistorySize_) {
        commandHistory_.erase(commandHistory_.begin());
    }
    
    if (onCommandFinishedCallback) {
        onCommandFinishedCallback(entry);
    }
    
    currentCommand_.clear();
}

void ShellIntegration::onDirectoryChanged(const std::string& directory) {
    currentDirectory_ = directory;
    
    if (onDirectoryChangedCallback) {
        onDirectoryChangedCallback(directory);
    }
}

void ShellIntegration::setOutputCallback(CommandStartedCallback callback) {
    onCommandStartedCallback = std::move(callback);
}

void ShellIntegration::setCommandFinishedCallback(CommandFinishedCallback callback) {
    onCommandFinishedCallback = std::move(callback);
}

void ShellIntegration::setDirectoryChangedCallback(DirectoryChangedCallback callback) {
    onDirectoryChangedCallback = std::move(callback);
}

const std::vector<CommandHistoryEntry>& ShellIntegration::getCommandHistory() const {
    return commandHistory_;
}

std::vector<CommandHistoryEntry> ShellIntegration::searchHistory(const std::string& query) const {
    std::vector<CommandHistoryEntry> results;
    
    for (const auto& entry : commandHistory_) {
        if (entry.command.find(query) != std::string::npos ||
            entry.directory.find(query) != std::string::npos) {
            results.push_back(entry);
        }
    }
    
    return results;
}

std::vector<CommandHistoryEntry> ShellIntegration::getRecentCommands(size_t count) const {
    if (count >= commandHistory_.size()) {
        return commandHistory_;
    }
    
    return std::vector<CommandHistoryEntry>(
        commandHistory_.end() - count,
        commandHistory_.end()
    );
}

void ShellIntegration::clearHistory() {
    commandHistory_.clear();
}

bool ShellIntegration::saveHistory(const std::string& filePath) const {
    std::ofstream file(filePath);
    if (!file.is_open()) return false;
    
    for (const auto& entry : commandHistory_) {
        auto timestamp = std::chrono::system_clock::to_time_t(entry.timestamp);
        file << timestamp << "\t"
             << entry.exitCode << "\t"
             << entry.durationMs << "\t"
             << entry.directory << "\t"
             << entry.command << "\n";
    }
    
    return true;
}

bool ShellIntegration::loadHistory(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    commandHistory_.clear();

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string tsStr, exitStr, durStr, dirStr, cmdStr;
        if (!std::getline(iss, tsStr, '\t') ||
            !std::getline(iss, exitStr, '\t') ||
            !std::getline(iss, durStr, '\t') ||
            !std::getline(iss, dirStr, '\t')) {
            continue;
        }
        std::getline(iss, cmdStr);

        try {
            CommandHistoryEntry entry;
            std::time_t timestamp = static_cast<std::time_t>(std::stoll(tsStr));
            entry.exitCode = std::stoi(exitStr);
            entry.durationMs = std::stoi(durStr);
            entry.directory = dirStr;
            entry.command = cmdStr;
            entry.timestamp = std::chrono::system_clock::from_time_t(timestamp);
            commandHistory_.push_back(std::move(entry));
        } catch (...) {
            continue;
        }
    }

    return true;
}

void ShellIntegration::shutdown() {
    currentCommand_.clear();
}

std::string ShellIntegration::getShellName() const {
    switch (shellType_) {
        case ShellType::Bash: return "bash";
        case ShellType::Zsh: return "zsh";
        case ShellType::Fish: return "fish";
        case ShellType::PowerShell: return "powershell";
        case ShellType::Cmd: return "cmd";
        case ShellType::Custom: return "custom";
        default: return "unknown";
    }
}

DetectedCommand ShellIntegration::parseCommand(const std::string& output) {
    DetectedCommand cmd;
    std::istringstream iss(output);
    std::string token;
    if (iss >> token) {
        cmd.name = token;
        cmd.fullPath = token;
        while (iss >> token) cmd.args.push_back(token);
    }
    return cmd;
}

std::string ShellIntegration::extractCwd(const std::string& output) {
    const std::string marker = "CurrentDir=";
    size_t pos = output.find(marker);
    if (pos == std::string::npos) return "";
    pos += marker.size();
    size_t end = output.find_first_of("\x07\x1b\r\n", pos);
    return end == std::string::npos ? output.substr(pos) : output.substr(pos, end - pos);
}

std::string ShellIntegration::extractExitCode(const std::string& output) {
    const std::string marker = "133;D;";
    size_t pos = output.find(marker);
    if (pos == std::string::npos) return "";
    pos += marker.size();
    size_t end = output.find_first_of(";\x07\x1b\r\n", pos);
    return end == std::string::npos ? output.substr(pos) : output.substr(pos, end - pos);
}

bool ShellIntegration::isPrompt(const std::string& line) const {
    if (line.empty()) return false;
    return line.find("$ ") != std::string::npos || line.find("# ") != std::string::npos ||
           line.find("> ") != std::string::npos;
}

std::string ShellIntegration::getPromptPattern() const { return promptPattern_; }

std::vector<std::string> ShellIntegration::getSuggestions(const std::string& prefix) {
    std::vector<std::string> suggestions;
    for (auto it = commandHistory_.rbegin(); it != commandHistory_.rend(); ++it) {
        if (it->command.rfind(prefix, 0) == 0 &&
            std::find(suggestions.begin(), suggestions.end(), it->command) == suggestions.end()) {
            suggestions.push_back(it->command);
            if (suggestions.size() >= 10) break;
        }
    }
    return suggestions;
}

std::string ShellIntegration::createFileUrl(const std::string& path) { return "file://" + path; }
std::string ShellIntegration::parseFileUrl(const std::string& url) {
    const std::string prefix = "file://";
    return url.rfind(prefix, 0) == 0 ? url.substr(prefix.size()) : url;
}
std::string ShellIntegration::createCurrentDirOsc(const std::string& path) {
    return "\x1b]1337;CurrentDir=" + path + "\x1b\\";
}
std::string ShellIntegration::createPromptMarkOsc() { return "\x1b]133;A\x1b\\"; }

void ShellIntegration::setupBash() { promptPattern_ = "$ "; }
void ShellIntegration::setupZsh() { promptPattern_ = "% "; }
void ShellIntegration::setupFish() { promptPattern_ = "> "; }
void ShellIntegration::setupPowerShell() { promptPattern_ = "PS> "; }
void ShellIntegration::setupCmd() { promptPattern_ = "> "; }

std::string ShellScriptGenerator::getInitScript(ShellType shell) {
    ShellIntegration si;
    return si.generateIntegrationScript(shell);
}
std::string ShellScriptGenerator::getPromptFunction(ShellType shell) {
    return getInitScript(shell);
}
std::string ShellScriptGenerator::getPreExecFunction(ShellType shell) {
    (void)shell;
    return "printf '\\e]133;C\\e\\\\'";
}
std::string ShellScriptGenerator::getPostExecFunction(ShellType shell) {
    (void)shell;
    return "printf '\\e]133;D;%d\\e\\\\' \"$?\"";
}

ShellType ShellIntegration::detectShell(const std::string& shellPath) {
    if (shellPath.find("bash") != std::string::npos) {
        return ShellType::Bash;
    } else if (shellPath.find("zsh") != std::string::npos) {
        return ShellType::Zsh;
    } else if (shellPath.find("fish") != std::string::npos) {
        return ShellType::Fish;
    } else if (shellPath.find("powershell") != std::string::npos ||
               shellPath.find("pwsh") != std::string::npos) {
        return ShellType::PowerShell;
    }

    return ShellType::Unknown;
}

std::string ShellIntegration::getDefaultShell() {
    const char* shell = getenv("SHELL");
    if (shell) {
        return std::string(shell);
    }

    return "/bin/bash";
}

} // namespace NexusForge::Terminal
