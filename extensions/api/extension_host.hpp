// extensions/api/extension_host.hpp
#pragma once

#include "extension_api.hpp"
#include "ipc_channel.hpp"
#include "../../core/engine/nexus_core.hpp"

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace NexusForge::Extensions {

// Extension runtime info
struct ExtensionRuntime {
    std::string id;
    std::string path;
    ExtensionManifest manifest;
    bool loaded = false;
    bool activated = false;
    bool enabled = true;
    std::string errorMessage;
    std::unique_ptr<ExtensionContext> context;
};

// Extension host process manager
class ExtensionHost {
public:
    ExtensionHost(Core::NexusEngine* engine);
    ~ExtensionHost();

    // Lifecycle
    bool initialize(const std::string& extensionsPath);
    void shutdown();

    // Extension management
    bool loadExtension(const std::string& extensionPath);
    bool unloadExtension(const std::string& extensionId);
    bool enableExtension(const std::string& extensionId);
    bool disableExtension(const std::string& extensionId);

    // Extension queries
    ExtensionRuntime* getExtension(const std::string& extensionId);
    std::vector<ExtensionRuntime*> getAllExtensions();
    std::vector<ExtensionRuntime*> getLoadedExtensions();
    std::vector<ExtensionRuntime*> getActivatedExtensions();

    // Activation
    bool activateExtension(const std::string& extensionId);
    bool activateByEvent(const std::string& event, const std::string& data = "");
    bool activateByLanguage(const std::string& languageId);
    bool activateByCommand(const std::string& commandId);
    bool activateByView(const std::string& viewId);
    bool activateByStartup();

    // Command execution
    bool executeCommand(const std::string& commandId, const std::vector<std::any>& args = {});
    void registerCommand(const std::string& extensionId, const std::string& commandId,
                         std::function<std::any(const std::vector<std::any>&)> handler);

    // IPC
    void sendToExtension(const std::string& extensionId, const IPCMessage& message);
    void broadcastToAll(const IPCMessage& message);

    // Update loop
    void update(double deltaTime);

    // Statistics
    int getExtensionCount() const { return static_cast<int>(extensions_.size()); }
    int getLoadedCount() const;
    int getActivatedCount() const;

private:
    Core::NexusEngine* engine_ = nullptr;
    std::string extensionsPath_;

    std::unordered_map<std::string, ExtensionRuntime> extensions_;
    std::unordered_map<std::string, std::function<std::any(const std::vector<std::any>&)>> commands_;

    bool initialized_ = false;

    // Extension loading
    bool loadExtensionManifest(const std::string& path, ExtensionManifest& manifest);
    bool validateExtension(const ExtensionManifest& manifest);
    bool activateExtensionInternal(ExtensionRuntime& ext);

    // IPC handling
    void handleIPCMessage(const std::string& extensionId, const IPCMessage& message);
    void processIPCQueue();

    // Command registration
    void registerExtensionCommands(const std::string& extensionId, const ExtensionManifest& manifest);

    // Event subscriptions
    void subscribeToEvents();
    void onFileOpened(const std::string& path);
    void onWorkspaceChanged();
};

} // namespace NexusForge::Extensions
