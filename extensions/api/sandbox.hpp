// extensions/api/sandbox.hpp
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace NexusForge::Extensions {

// Sandbox configuration
struct SandboxConfig {
    bool enableFileSystem = true;
    bool enableNetwork = false;
    bool enableProcessSpawn = false;
    bool enableNativeCode = false;
    std::vector<std::string> allowedPaths;
    std::vector<std::string> allowedHosts;
    size_t maxMemory = 256 * 1024 * 1024;  // 256MB
    int maxCpuPercent = 50;
    int timeoutMs = 30000;
};

// Sandbox API
class SandboxAPI {
public:
    virtual ~SandboxAPI() = default;
    
    // File system
    virtual bool readFile(const std::string& path, std::string& content) = 0;
    virtual bool writeFile(const std::string& path, const std::string& content) = 0;
    virtual bool fileExists(const std::string& path) = 0;
    
    // Network
    virtual bool httpGet(const std::string& url, std::string& response) = 0;
    virtual bool httpPost(const std::string& url, const std::string& body,
                          std::string& response) = 0;
    
    // Console
    virtual void log(const std::string& message) = 0;
    virtual void warn(const std::string& message) = 0;
    virtual void error(const std::string& message) = 0;
    
    // Timers
    virtual int setTimeout(std::function<void()> callback, int delay) = 0;
    virtual int setInterval(std::function<void()> callback, int interval) = 0;
    virtual void clearTimeout(int id) = 0;
    virtual void clearInterval(int id) = 0;
    
    // Events
    virtual void on(const std::string& event, std::function<void()> callback) = 0;
    virtual void emit(const std::string& event, const std::string& data) = 0;
};

// JavaScript sandbox (using QuickJS or V8)
class JSSandbox {
public:
    JSSandbox();
    ~JSSandbox();

    // Initialize
    bool initialize(const SandboxConfig& config);
    void shutdown();

    // Script execution
    bool executeScript(const std::string& script);
    bool executeFile(const std::string& path);
    
    // Evaluate
    std::string evaluate(const std::string& expression);
    
    // Call function
    std::string callFunction(const std::string& name,
                              const std::vector<std::string>& args);

    // Set global
    void setGlobal(const std::string& name, const std::string& value);
    void setGlobalFunction(const std::string& name,
                           std::function<std::string(const std::vector<std::string>&)> func);

    // API access
    SandboxAPI* getAPI() { return api_.get(); }

    // State
    bool isRunning() const { return running_; }
    void stop();

    // Memory
    size_t getMemoryUsage() const;
    void garbageCollect();

private:
    SandboxConfig config_;
    std::unique_ptr<SandboxAPI> api_;
    bool running_ = false;
    void* runtime_ = nullptr;  // JSRuntime*
    void* context_ = nullptr;   // JSContext*

    bool createRuntime();
    void destroyRuntime();
    bool registerAPI();
};

// WebAssembly sandbox
class WasmSandbox {
public:
    WasmSandbox();
    ~WasmSandbox();

    // Initialize
    bool initialize(const SandboxConfig& config);
    void shutdown();

    // Module loading
    bool loadModule(const std::string& path);
    bool loadModuleFromMemory(const void* data, size_t size);

    // Function calls
    std::string callFunction(const std::string& name,
                              const std::vector<std::string>& args);

    // Exports
    std::vector<std::string> getExportedFunctions() const;

private:
    SandboxConfig config_;
    void* store_ = nullptr;  // wasm_store_t*
    void* module_ = nullptr; // wasm_module_t*
    void* instance_ = nullptr; // wasm_instance_t*
};

} // namespace NexusForge::Extensions
