// config/user_data.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace NexusForge::Config {

// User data paths
class UserDataPaths {
public:
    static UserDataPaths& getInstance();

    // Directories
    std::string getAppData() const;
    std::string getUserData() const;
    std::string getUserHome() const;
    std::string getTemp() const;

    // NexusForge specific
    std::string getNexusDataDir() const;
    std::string getSettingsDir() const;
    std::string getExtensionsDir() const;
    std::string getPluginsDir() const;
    std::string getCacheDir() const;
    std::string getLogsDir() const;
    std::string getSessionsDir() const;
    std::string getSnippetsDir() const;
    std::string getKeybindingsDir() const;
    std::string getThemesDir() const;

    // Workspace
    std::string getWorkspaceDir() const;
    std::string getRecentWorkspacesFile() const;

    // State
    std::string getStateFile() const;
    std::string getGlobalStateFile() const;
    std::string getWorkspaceStateFile() const;

    // Logs
    std::string getLogFile() const;
    std::string getCrashLogFile() const;

private:
    UserDataPaths();
    std::string appData_;
    std::string userData_;
    std::string nexusData_;

    void ensureDirectories();
};

// User data manager
class UserDataManager {
public:
    static UserDataManager& getInstance();

    // Initialize
    bool initialize();
    void shutdown();

    // Settings
    bool loadUserSettings();
    bool saveUserSettings();
    std::string getUserSettingsPath() const;

    // Keybindings
    bool loadKeybindings();
    bool saveKeybindings();
    std::string getKeybindingsPath() const;

    // Snippets
    bool loadSnippets();
    bool saveSnippets();
    std::string getSnippetsPath() const;

    // State
    bool loadState();
    bool saveState();
    std::string getStatePath() const;

    // Cache
    void clearCache();
    size_t getCacheSize() const;

    // Logs
    void clearLogs();
    std::string getLogContent() const;
    void rotateLogs();

    // Backup
    bool createBackup();
    bool restoreBackup(const std::string& backupPath);
    std::vector<std::string> listBackups() const;

    // Import/Export
    bool exportSettings(const std::string& path);
    bool importSettings(const std::string& path);

    // Migration
    bool migrateFromOldVersion(const std::string& oldVersion);

private:
    UserDataManager();
    bool initialized_ = false;

    bool createDefaultSettings();
    bool createDefaultKeybindings();
};

// Settings sync
class SettingsSync {
public:
    static SettingsSync& getInstance();

    // Sync configuration
    void enableSync(bool enable);
    bool isSyncEnabled() const;

    void setSyncToken(const std::string& token);
    void setSyncServer(const std::string& server);

    // Sync items
    void syncSettings(bool sync);
    void syncKeybindings(bool sync);
    void syncSnippets(bool sync);
    void syncExtensions(bool sync);

    // Manual sync
    void syncNow();
    void pullFromServer();
    void pushToServer();

    // Status
    bool isSyncing() const;
    std::string getLastSyncTime() const;
    std::string getSyncStatus() const;

    // Conflicts
    std::vector<std::string> getConflicts() const;
    void resolveConflict(const std::string& item, bool useLocal);

private:
    SettingsSync();
    bool enabled_ = false;
    std::string token_;
    std::string server_;

    bool syncSettings_;
    bool syncKeybindings_;
    bool syncSnippets_;
    bool syncExtensions_;
};

} // namespace NexusForge::Config
