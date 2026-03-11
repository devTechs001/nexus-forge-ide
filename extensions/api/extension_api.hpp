// extensions/api/extension_api.hpp
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <any>
#include <variant>
#include <unordered_map>

namespace NexusForge::Extensions {

// Extension manifest
struct ExtensionManifest {
    std::string id;
    std::string name;
    std::string displayName;
    std::string version;
    std::string description;
    std::string author;
    std::string license;
    std::string repository;
    std::string homepage;
    std::string icon;

    std::vector<std::string> categories;
    std::vector<std::string> keywords;

    struct Engines {
        std::string nexusforge;  // Minimum version
    } engines;

    struct Activation {
        std::vector<std::string> onLanguage;
        std::vector<std::string> onCommand;
        std::vector<std::string> onView;
        std::vector<std::string> onUri;
        std::vector<std::string> onFileSystem;
        std::vector<std::string> workspaceContains;
        bool onStartupFinished = false;
    } activationEvents;

    struct Contributions {
        std::vector<struct CommandDef> commands;
        std::vector<struct KeybindingDef> keybindings;
        std::vector<struct MenuDef> menus;
        std::vector<struct ViewDef> views;
        std::vector<struct LanguageDef> languages;
        std::vector<struct ThemeDef> themes;
        std::vector<struct SnippetDef> snippets;
        std::vector<struct ConfigDef> configuration;
    } contributes;

    std::vector<std::string> dependencies;
    std::vector<std::string> extensionDependencies;
};

// Command definition
struct CommandDef {
    std::string command;
    std::string title;
    std::string category;
    std::string icon;
    bool enablement;
};

// Keybinding definition
struct KeybindingDef {
    std::string command;
    std::string key;
    std::string mac;
    std::string linux;
    std::string win;
    std::string when;
};

// Menu definition
struct MenuDef {
    std::string id;
    std::string group;
    int order;
    std::string when;
};

// View definition
struct ViewDef {
    std::string id;
    std::string name;
    std::string icon;
    std::string contextualTitle;
    std::string type;
    std::string when;
};

// Extension context
class ExtensionContext {
public:
    ExtensionContext(const std::string& extensionId, const std::string& extensionPath);

    // Paths
    std::string getExtensionPath() const { return extensionPath_; }
    std::string getGlobalStoragePath() const;
    std::string getWorkspaceStoragePath() const;
    std::string getLogPath() const;

    // State storage
    void setGlobalState(const std::string& key, const std::any& value);
    std::any getGlobalState(const std::string& key) const;
    void setWorkspaceState(const std::string& key, const std::any& value);
    std::any getWorkspaceState(const std::string& key) const;

    // Secrets
    void storeSecret(const std::string& key, const std::string& value);
    std::string getSecret(const std::string& key) const;
    void deleteSecret(const std::string& key);

    // Subscriptions (for cleanup)
    void subscriptions_push(std::function<void()> disposer);

    // Environment
    std::string getEnvironmentVariable(const std::string& name) const;
    bool isNewInstall() const;
    int getExtensionMode() const;  // 1: production, 2: development, 3: test

private:
    std::string extensionId_;
    std::string extensionPath_;
    std::unordered_map<std::string, std::any> globalState_;
    std::unordered_map<std::string, std::any> workspaceState_;
    std::vector<std::function<void()>> subscriptions_;
};

// Extension API namespace
namespace API {

// Window API
namespace Window {
    void showInformationMessage(const std::string& message, const std::vector<std::string>& options = {});
    void showWarningMessage(const std::string& message, const std::vector<std::string>& options = {});
    void showErrorMessage(const std::string& message, const std::vector<std::string>& options = {});

    int showQuickPick(const std::vector<std::string>& items, const std::string& placeholder = "");
    std::string showInputBox(const std::string& prompt = "", const std::string& defaultValue = "");

    void setStatusBarMessage(const std::string& message, int timeoutMs = 5000);
    void createStatusBarItem(int priority = 0);

    void createOutputChannel(const std::string& name);
    void appendToOutput(const std::string& channel, const std::string& text);
    void showOutput(const std::string& channel);

    // Progress
    void withProgress(const std::string& title, std::function<void(float)> task);
}

// Workspace API
namespace Workspace {
    std::string getRootPath();
    std::vector<std::string> getWorkspaceFolders();

    // File operations
    bool createFile(const std::string& path, const std::string& content = "");
    bool createDirectory(const std::string& path);
    bool deleteFile(const std::string& path);
    bool renameFile(const std::string& oldPath, const std::string& newPath);
    std::string readFile(const std::string& path);
    bool writeFile(const std::string& path, const std::string& content);

    // File search
    std::vector<std::string> findFiles(const std::string& pattern,
                                        const std::string& exclude = "",
                                        int maxResults = 10000);

    // Text search
    struct TextSearchResult {
        std::string uri;
        int lineNumber;
        int column;
        std::string text;
        std::string match;
    };
    std::vector<TextSearchResult> textSearch(const std::string& pattern,
                                              const std::string& options = "");

    // Watchers
    int createFileSystemWatcher(const std::string& globPattern,
                                 std::function<void(const std::string&)> onCreate,
                                 std::function<void(const std::string&)> onChange,
                                 std::function<void(const std::string&)> onDelete);
    void disposeWatcher(int watcherId);

    // Configuration
    std::any getConfiguration(const std::string& section = "");
}

// Languages API
namespace Languages {
    // Diagnostics
    void createDiagnosticCollection(const std::string& name);
    void setDiagnostics(const std::string& collection, const std::string& uri,
                        const std::vector<struct Diagnostic>& diagnostics);

    struct Diagnostic {
        int lineNumber;
        int column;
        int endLineNumber;
        int endColumn;
        std::string message;
        std::string severity;  // error, warning, info, hint
        std::string source;
        std::string code;
    };

    // Completion
    void registerCompletionProvider(const std::string& language,
                                     std::function<std::vector<std::string>(const std::string& prefix)> provider);

    // Hover
    void registerHoverProvider(const std::string& language,
                                std::function<std::string(const std::string& word)> provider);

    // Definition
    void registerDefinitionProvider(const std::string& language,
                                     std::function<std::string(const std::string& symbol)> provider);

    // Formatting
    void registerDocumentFormattingProvider(const std::string& language,
                                             std::function<std::string(const std::string& code)> provider);

    // Code actions
    void registerCodeActionProvider(const std::string& language,
                                     std::function<std::vector<std::string>(const std::string& error)> provider);

    // Folding
    void registerFoldingRangeProvider(const std::string& language,
                                       std::function<std::vector<std::pair<int, int>>(const std::string& code)> provider);

    // Signature help
    void registerSignatureHelpProvider(const std::string& language,
                                        std::function<std::string(const std::string& prefix)> provider);

    // Document symbols
    struct DocumentSymbol {
        std::string name;
        std::string detail;
        std::string kind;  // file, module, namespace, package, class, method, property, field, constructor, enum, interface, function, variable, constant, string, number, boolean, array, object, key, null, enumMember, struct, event, operator, typeParameter
        int startLine;
        int startColumn;
        int endLine;
        int endColumn;
    };
    void registerDocumentSymbolProvider(const std::string& language,
                                         std::function<std::vector<DocumentSymbol>(const std::string& code)> provider);

    // Color provider
    struct ColorInformation {
        int startLine;
        int startColumn;
        int endLine;
        int endColumn;
        std::string color;  // hex color
    };
    void registerColorProvider(const std::string& language,
                                std::function<std::vector<ColorInformation>(const std::string& code)> provider);
}

// Commands API
namespace Commands {
    void registerCommand(const std::string& command, std::function<void()> callback);
    void registerCommand(const std::string& command, std::function<void(const std::any&)> callback);
    void executeCommand(const std::string& command, const std::any& args = nullptr);
}

// Debug API
namespace Debug {
    void startDebugging(const std::string& configuration = "");
    void stopDebugging();

    struct Breakpoint {
        std::string uri;
        int lineNumber;
        std::string condition;
        std::string hitCondition;
        std::string logMessage;
    };

    void addBreakpoint(const Breakpoint& bp);
    void removeBreakpoint(const Breakpoint& bp);

    // Debug session
    bool isInDebugMode();
    std::string getCurrentDebugFile();
    int getCurrentDebugLine();
}

// Tests API
namespace Tests {
    void createTestController(const std::string& id, const std::string& label);

    struct TestItem {
        std::string id;
        std::string label;
        std::string uri;
        int startLine;
        int startColumn;
        int endLine;
        int endColumn;
        std::vector<TestItem> children;
    };

    void addTestItem(const std::string& controller, const TestItem& item);
    void runTests(const std::string& controller, const std::vector<std::string>& testIds = {});
    void debugTests(const std::string& controller, const std::vector<std::string>& testIds = {});
}

} // namespace API

// Extension interface
class IExtension {
public:
    virtual ~IExtension() = default;
    virtual void activate(ExtensionContext& context) = 0;
    virtual void deactivate() = 0;
};

// Extension loader
class ExtensionLoader {
public:
    static ExtensionLoader& getInstance();

    bool loadExtension(const std::string& path);
    bool unloadExtension(const std::string& id);
    bool enableExtension(const std::string& id);
    bool disableExtension(const std::string& id);

    IExtension* getExtension(const std::string& id);
    std::vector<std::string> getLoadedExtensions() const;
    std::vector<std::string> getEnabledExtensions() const;

    bool activateExtension(const std::string& id);
    bool activateByEvent(const std::string& event, const std::any& data = nullptr);

private:
    ExtensionLoader() = default;
    std::unordered_map<std::string, std::unique_ptr<IExtension>> extensions_;
    std::unordered_map<std::string, ExtensionManifest> manifests_;
    std::unordered_map<std::string, bool> enabled_;
};

} // namespace NexusForge::Extensions
