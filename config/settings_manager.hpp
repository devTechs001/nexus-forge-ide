// config/settings_manager.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <optional>
#include <functional>
#include <any>

#include "../ai/models/model_manager.hpp"

namespace NexusForge::Config {

// Setting value types
using SettingValue = std::variant<
    bool,
    int,
    float,
    double,
    std::string,
    std::vector<std::string>,
    std::unordered_map<std::string, std::string>
>;

// Setting scope
enum class SettingScope {
    Default,      // Built-in defaults
    User,         // User-level settings
    Workspace,    // Workspace-level settings
    Folder        // Folder-level settings
};

// Setting definition
struct SettingDefinition {
    std::string key;
    std::string title;
    std::string description;
    std::string category;
    std::string type;  // boolean, number, string, array, object, enum
    SettingValue defaultValue;
    std::optional<std::vector<std::string>> enumValues;
    std::optional<double> minimum;
    std::optional<double> maximum;
    std::optional<int> minLength;
    std::optional<int> maxLength;
    std::optional<std::string> pattern;
    bool deprecated = false;
    std::string deprecationMessage;
    std::vector<std::string> tags;
};

// Settings categories
struct SettingsCategory {
    std::string id;
    std::string title;
    std::string icon;
    std::vector<std::string> settingKeys;
    std::vector<SettingsCategory> subcategories;
};

// Editor settings
struct EditorSettings {
    // Font
    std::string fontFamily = "JetBrains Mono, Consolas, monospace";
    float fontSize = 14.0f;
    float fontWeight = 400;
    float lineHeight = 1.5f;
    float letterSpacing = 0.0f;

    // Display
    bool lineNumbers = true;
    std::string lineNumbersType = "on";  // on, off, relative, interval
    bool minimap = true;
    float minimapScale = 1.0f;
    std::string minimapSide = "right";
    bool renderWhitespace = false;
    std::string renderWhitespaceMode = "selection";  // none, boundary, selection, trailing, all
    bool bracketPairColorization = true;
    bool indentGuides = true;
    bool highlightActiveLine = true;
    bool highlightActiveLineNumber = true;
    bool stickyScroll = false;
    int stickyScrollMaxLineCount = 5;

    // Cursor
    std::string cursorStyle = "line";  // line, block, underline, line-thin, block-outline, underline-thin
    float cursorBlinking = 530;  // ms
    bool cursorSmoothCaretAnimation = true;
    int cursorWidth = 2;

    // Scrolling
    bool smoothScrolling = true;
    int scrollBeyondLastLine = 5;
    bool scrollBeyondLastColumn = true;
    std::string mouseWheelScrollSensitivity = "normal";

    // Word wrap
    std::string wordWrap = "off";  // off, on, wordWrapColumn, bounded
    int wordWrapColumn = 80;

    // Formatting
    bool formatOnSave = false;
    bool formatOnPaste = false;
    bool formatOnType = false;
    int tabSize = 4;
    bool insertSpaces = true;
    bool detectIndentation = true;
    bool trimAutoWhitespace = true;
    bool trimFinalNewlines = false;
    bool insertFinalNewline = true;

    // Auto features
    bool autoIndent = true;
    bool autoClosingBrackets = true;
    bool autoClosingQuotes = true;
    bool autoSurround = true;
    bool autoClosingTags = true;
    bool linkedEditing = true;

    // Suggestions
    bool quickSuggestions = true;
    int quickSuggestionsDelay = 10;
    bool suggestOnTriggerCharacters = true;
    bool acceptSuggestionOnEnter = true;
    bool acceptSuggestionOnCommitCharacter = true;
    std::string snippetSuggestions = "inline";  // top, bottom, inline, none
    bool tabCompletion = false;
    bool wordBasedSuggestions = true;

    // Find
    bool findSeedSearchStringFromSelection = true;
    bool findAutoFindInSelection = false;
    bool findGlobalFindClipboard = false;
    bool findAddExtraSpaceOnTop = true;
    bool findLoop = true;

    // Code lens
    bool codeLens = true;
    std::string codeLensFontFamily = "";
    float codeLensFontSize = 0;  // 0 = use editor font size

    // Folding
    bool folding = true;
    std::string foldingStrategy = "auto";  // auto, indentation
    bool foldingHighlight = true;
    bool showFoldingControls = true;
    int foldingMaximumRegions = 5000;

    // Hover
    bool hover = true;
    int hoverDelay = 300;
    bool hoverSticky = true;

    // Diff editor
    bool diffEditorRenderSideBySide = true;
    bool diffEditorIgnoreWhitespace = false;
    bool diffEditorRenderIndicators = true;
};

// Terminal settings
struct TerminalSettings {
    std::string defaultProfile;
    std::string fontFamily = "JetBrains Mono, monospace";
    float fontSize = 14.0f;
    float lineHeight = 1.2f;
    int scrollback = 10000;
    bool copyOnSelection = false;
    bool cursorBlinking = true;
    std::string cursorStyle = "block";
    std::string shell;
    std::vector<std::string> shellArgs;
    std::unordered_map<std::string, std::string> env;
    std::string cwd;
    bool integratedGpu = true;
};

// Workbench settings
struct WorkbenchSettings {
    // Appearance
    std::string colorTheme = "NexusForge Dark";
    std::string iconTheme = "nexusforge-icons";
    std::string productIconTheme = "Default";

    // Layout
    std::string sideBarLocation = "left";
    std::string panelLocation = "bottom";
    bool activityBarVisible = true;
    bool statusBarVisible = true;
    bool menuBarVisible = true;

    // Tabs
    bool showTabs = true;
    std::string tabCloseButton = "right";
    bool tabDecorations = true;
    bool tabHighlight = true;
    std::string openEditorsVisible = "visible";
    int tabSizing = 120;

    // Editor
    bool showEditorTabs = true;
    bool enablePreview = true;
    bool enablePreviewFromQuickOpen = false;

    // Startup
    std::string startupEditor = "welcomePage";  // none, welcomePage, readme, newUntitledFile, welcomePageInEmptyWorkbench
    bool restoreWindows = true;

    // Trees
    int treeIndent = 8;
    bool treeRenderIndentGuides = true;
    bool treeExpandOnClick = true;

    // Zenmode
    bool zenModeHideTabs = true;
    bool zenModeHideStatusBar = true;
    bool zenModeHideActivityBar = true;
    bool zenModeHideLineNumbers = false;
    bool zenModeFullScreen = true;
    bool zenModeCenterLayout = true;
};

// Files settings
struct FilesSettings {
    std::string encoding = "utf8";
    std::string eol = "auto";  // \n, \r\n, auto
    bool autoSave = false;
    int autoSaveDelay = 1000;
    std::string autoSaveMode = "afterDelay";  // off, afterDelay, onFocusChange, onWindowChange
    bool hotExit = true;
    std::vector<std::string> exclude;
    std::vector<std::string> associations;
    bool trimTrailingWhitespace = false;
    bool insertFinalNewline = false;
    int maxMemoryForLargeFilesMB = 4096;
    bool watcherExclude;
    bool enableTrash = true;
    bool confirmDelete = true;
};

// Search settings
struct SearchSettings {
    std::vector<std::string> exclude;
    bool useIgnoreFiles = true;
    bool useGlobalIgnoreFiles = true;
    bool useParentIgnoreFiles = true;
    bool followSymlinks = true;
    bool smartCase = true;
    bool showLineNumbers = true;
    bool collapseResults = "auto";
    std::string mode = "sidebar";
    int maxResults = 20000;
    bool maintainFileSearchCache = true;
};

// Git settings
struct GitSettings {
    bool enabled = true;
    std::string path = "git";
    bool autoFetch = false;
    int autoFetchPeriod = 180;
    bool autofresh = true;
    bool confirmSync = true;
    bool enableSmartCommit = true;
    bool enableCommitSigning = false;
    bool pruneOnFetch = false;
    bool rebaseWhenSync = false;
    std::string branchSortOrder = "committerdate";
    bool showInlineOpenFileAction = true;
    bool showPushSuccessNotification = false;
    bool decorations = true;
};

// Keybindings
struct Keybinding {
    std::string command;
    std::string key;
    std::string mac;
    std::string linux;
    std::string win;
    std::string when;
    std::vector<std::string> args;
};

// Settings Manager
class SettingsManager {
public:
    SettingsManager();
    ~SettingsManager();

    bool initialize();
    void shutdown();

    // Load/Save
    bool loadSettings();
    bool saveSettings();
    bool loadUserSettings(const std::string& path = "");
    bool loadWorkspaceSettings(const std::string& path = "");

    // Get settings objects
    EditorSettings& getEditorSettings() { return editorSettings_; }
    const EditorSettings& getEditorSettings() const { return editorSettings_; }

    TerminalSettings& getTerminalSettings() { return terminalSettings_; }
    WorkbenchSettings& getWorkbenchSettings() { return workbenchSettings_; }
    FilesSettings& getFilesSettings() { return filesSettings_; }
    SearchSettings& getSearchSettings() { return searchSettings_; }
    GitSettings& getGitSettings() { return gitSettings_; }
    AI::AISettings& getAISettings() { return aiSettings_; }

    // Generic get/set
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T{}) const;

    template<typename T>
    void set(const std::string& key, const T& value, SettingScope scope = SettingScope::User);

    bool has(const std::string& key) const;
    void remove(const std::string& key, SettingScope scope = SettingScope::User);
    void reset(const std::string& key);
    void resetAll();

    // Effective value (considering all scopes)
    SettingValue getEffectiveValue(const std::string& key) const;
    SettingScope getEffectiveScope(const std::string& key) const;

    // Categories
    const std::vector<SettingsCategory>& getCategories() const { return categories_; }
    std::vector<SettingDefinition> getSettingsInCategory(const std::string& categoryId) const;
    std::vector<SettingDefinition> searchSettings(const std::string& query) const;

    // Definitions
    void registerSetting(const SettingDefinition& definition);
    const SettingDefinition* getSettingDefinition(const std::string& key) const;

    // Keybindings
    const std::vector<Keybinding>& getKeybindings() const { return keybindings_; }
    void setKeybinding(const Keybinding& keybinding);
    void removeKeybinding(const std::string& command);
    std::string getKeybindingForCommand(const std::string& command) const;
    std::string getCommandForKeybinding(const std::string& key) const;

    // Change notifications
    using ChangeCallback = std::function<void(const std::string& key, const SettingValue& newValue)>;
    size_t addChangeListener(const std::string& keyPattern, ChangeCallback callback);
    void removeChangeListener(size_t listenerId);

    // Export/Import
    std::string exportSettings() const;
    bool importSettings(const std::string& json);

    // Paths
    static std::string getUserSettingsPath();
    static std::string getKeybindingsPath();
    static std::string getSnippetsPath();

private:
    // Structured settings
    EditorSettings editorSettings_;
    TerminalSettings terminalSettings_;
    WorkbenchSettings workbenchSettings_;
    FilesSettings filesSettings_;
    SearchSettings searchSettings_;
    GitSettings gitSettings_;
    AI::AISettings aiSettings_;

    // Generic storage
    std::unordered_map<std::string, SettingValue> defaultSettings_;
    std::unordered_map<std::string, SettingValue> userSettings_;
    std::unordered_map<std::string, SettingValue> workspaceSettings_;
    std::unordered_map<std::string, SettingValue> folderSettings_;

    // Metadata
    std::vector<SettingsCategory> categories_;
    std::unordered_map<std::string, SettingDefinition> definitions_;

    // Keybindings
    std::vector<Keybinding> keybindings_;
    std::vector<Keybinding> userKeybindings_;

    // Change listeners
    struct ChangeListener {
        size_t id;
        std::string pattern;
        ChangeCallback callback;
    };
    std::vector<ChangeListener> changeListeners_;
    size_t nextListenerId_ = 1;

    // Helpers
    void registerDefaultSettings();
    void registerDefaultKeybindings();
    void buildCategories();
    void applySettings();
    void notifyChange(const std::string& key, const SettingValue& value);
    bool matchPattern(const std::string& key, const std::string& pattern) const;

    // Serialization
    std::string serializeValue(const SettingValue& value) const;
    SettingValue deserializeValue(const std::string& json, const std::string& type) const;
};

} // namespace NexusForge::Config
