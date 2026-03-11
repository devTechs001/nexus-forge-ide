// debugger/debug_adapter_protocol.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <variant>
#include <optional>

namespace NexusForge::Debugger {

// DAP Message types
struct DAPMessage {
    int seq = 0;
    std::string type;  // request, response, event
    std::string command;
    std::unordered_map<std::string, std::variant<
        bool, int, double, std::string,
        std::vector<std::string>,
        std::unordered_map<std::string, std::string>
    >> body;
    bool success = true;
    std::string message;
};

// Breakpoint
struct Breakpoint {
    int id = 0;
    bool verified = false;
    std::string source;
    int line = 0;
    int column = 0;
    std::string condition;
    std::string hitCondition;
    std::string logMessage;
    bool logBreakpoint = false;
};

// Stack frame
struct StackFrame {
    int id = 0;
    std::string name;
    std::string source;
    int line = 0;
    int column = 0;
    int endLine = 0;
    int endColumn = 0;
    bool canRestart = false;
    std::string instructionPointerReference;
    std::string moduleId;
    bool presentationHint;  // normal, label, subtle
};

// Variable
struct Variable {
    std::string name;
    std::string value;
    std::string type;
    int variablesReference = 0;
    int namedVariables = 0;
    int indexedVariables = 0;
    std::string memoryReference;
    std::string evaluateName;
    std::string presentationHint;
};

// Thread
struct Thread {
    int id = 0;
    std::string name;
};

// Scope
struct Scope {
    std::string name;
    int variablesReference = 0;
    bool namedVariables = false;
    int indexedVariables = 0;
    bool expensive = false;
    std::string source;
    int line = 0;
    int column = 0;
    int endLine = 0;
    int endColumn = 0;
};

// Source
struct Source {
    std::string name;
    std::string path;
    int sourceReference = 0;
    std::string presentationHint;  // normal, emphasized, de-emphasized
    std::string origin;
    std::vector<std::string> sources;
    std::vector<int> adapterData;
    std::string checksums;
};

// Completion item
struct CompletionItem {
    std::string label;
    std::string text;
    std::string type;  // method, function, constructor, field, variable, class, interface, module, property, unit, value, enum, keyword, snippet, color, file, reference, folder, enumMember, constant, struct, event, operator, typeParameter
    int start = 0;
    int length = 0;
    std::string selectionStart;
    int selectionLength = 0;
    std::string sortText;
    std::string detail;
};

// Debug adapter session
class DebugAdapterSession {
public:
    DebugAdapterSession();
    ~DebugAdapterSession();

    // Connection
    bool connect(const std::string& adapterPath, const std::string& adapterArgs = "");
    bool connectViaTcp(const std::string& host, int port);
    void disconnect();
    bool isConnected() const { return connected_; }

    // Message handling
    using MessageCallback = std::function<void(const DAPMessage&)>;
    void setMessageCallback(MessageCallback callback) { messageCallback_ = callback; }

    // Initialize
    bool initialize(const std::string& adapterId = "nexusforge");
    bool launch(const std::unordered_map<std::string, std::string>& config);
    bool attach(int processId);

    // Execution control
    bool continueExecution(int threadId = -1);
    bool next(int threadId);
    bool stepIn(int threadId);
    bool stepOut(int threadId);
    bool stepBack(int threadId);
    bool reverseContinue(int threadId);
    bool pause(int threadId);
    bool terminate();
    bool restart();

    // Breakpoints
    std::vector<Breakpoint> setBreakpoints(const std::string& source,
                                            const std::vector<Breakpoint>& breakpoints);
    bool setFunctionBreakpoints(const std::vector<std::string>& functions);
    bool setDataBreakpoints(const std::vector<std::pair<std::string, std::string>>& data);
    bool setExceptionBreakpoints(const std::vector<std::string>& filters);
    bool clearAllBreakpoints();

    // Stack traces
    std::vector<StackFrame> getStackTrace(int threadId, int startFrame = 0, int levels = 20);

    // Scopes and variables
    std::vector<Scope> getScopes(int frameId);
    std::vector<Variable> getVariables(int variablesReference,
                                        int start = 0, int count = 0);
    std::string evaluate(const std::string& expression, int frameId = 0,
                         const std::string& context = "repl");

    // Threads
    std::vector<Thread> getThreads();

    // Source
    std::string getSource(int sourceReference);
    std::string getSourceFile(const std::string& path);

    // Completion
    std::vector<CompletionItem> getCompletions(const std::string& text,
                                                int column, int line = 0);

    // Hover
    std::string getHover(const std::string& expression, int frameId = 0);

    // Restart frame
    bool restartFrame(int frameId);

    // Goto
    bool gotoTarget(int threadId, int targetId);

    // Loaded sources
    std::vector<Source> getLoadedSources();

    // Modules
    struct Module {
        int id;
        std::string name;
        std::string path;
        bool isOptimized;
        bool isUserCode;
        std::string version;
        std::string symbolStatus;
        std::string symbolFilePath;
        std::string dateTimeStamp;
        std::string addressRange;
    };
    std::vector<Module> getModules();

    // Disassembly
    struct DisassembledInstruction {
        std::string address;
        std::string instructionBytes;
        std::string instruction;
        std::string symbol;
        int location;
        int endLocation;
        std::string line;
        int column;
        int endLine;
        int endColumn;
    };
    std::vector<DisassembledInstruction> disassemble(
        const std::string& memoryReference, int instructionOffset,
        int instructionCount, int resolveSymbols = 0);

    // Read memory
    std::vector<uint8_t> readMemory(const std::string& memoryReference,
                                     int offset, int count);

    // Write memory
    bool writeMemory(const std::string& memoryReference, int offset,
                     const std::vector<uint8_t>& data);

    // Set variable
    bool setVariable(int variablesReference, const std::string& name,
                     const std::string& value);

    // Set expression
    bool setExpression(const std::string& expression, const std::string& value);

    // Exception info
    struct ExceptionInfo {
        std::string exceptionId;
        std::string description;
        std::string breakMode;  // never, always, unhandled, userUnhandled
        std::vector<std::string> details;
    };
    std::optional<ExceptionInfo> getExceptionInfo(int threadId);

    // Capabilities
    struct Capabilities {
        bool supportsConfigurationDoneRequest = false;
        bool supportsFunctionBreakpoints = false;
        bool supportsConditionalBreakpoints = false;
        bool supportsHitConditionalBreakpoints = false;
        bool supportsEvaluateForHovers = false;
        bool supportsExceptionBreakpointFilters = false;
        bool supportsExceptionOptions = false;
        bool supportsValueFormattingOptions = false;
        bool supportsExceptionInfoRequest = false;
        bool supportsExceptionOptions = false;
        bool supportsTerminateRequest = false;
        bool supportsRestartRequest = false;
        bool supportsGotoTargetsRequest = false;
        bool supportsStepInTargetsRequest = false;
        bool supportsCompletionsRequest = false;
        bool supportsModulesRequest = false;
        bool supportsRestartFrame = false;
        bool supportsReadMemoryRequest = false;
        bool supportsWriteMemoryRequest = false;
        bool supportsDisassembleRequest = false;
        bool supportsCancelRequest = false;
        bool supportsBreakpointLocationsRequest = false;
        bool supportsClipboardContext = false;
        bool supportsSteppingGranularity = false;
        bool supportsInstructionBreakpoints = false;
        bool supportsDataBreakpoints = false;
    };
    const Capabilities& getCapabilities() const { return capabilities_; }

    // Events
    using EventCallback = std::function<void(const std::string&, const DAPMessage&)>;
    void setEventCallback(EventCallback callback) { eventCallback_ = callback; }

private:
    bool connected_ = false;
    Capabilities capabilities_;
    MessageCallback messageCallback_;
    EventCallback eventCallback_;

    int sequenceNumber_ = 0;
    std::unordered_map<int, std::function<void(const DAPMessage&)>> pendingRequests_;

    // Internal
    DAPMessage sendMessage(const std::string& command,
                           const std::unordered_map<std::string, std::variant<
                               bool, int, double, std::string,
                               std::vector<std::string>,
                               std::unordered_map<std::string, std::string>
                           >>& body = {});
    DAPMessage waitForResponse(int seq);
    void processMessage(const DAPMessage& msg);
    void handleEvent(const std::string& event, const DAPMessage& msg);

    // Event handlers
    void onInitialized(const DAPMessage& msg);
    void onStopped(const DAPMessage& msg);
    void onContinued(const DAPMessage& msg);
    void onExited(const DAPMessage& msg);
    void onTerminated(const DAPMessage& msg);
    void onBreakpoint(const DAPMessage& msg);
    void onOutput(const DAPMessage& msg);
    void onThread(const DAPMessage& msg);
    void onModule(const DAPMessage& msg);
    void onLoadedSource(const DAPMessage& msg);
    void onProcess(const DAPMessage& msg);
    void onCapabilities(const DAPMessage& msg);
    void onProgressStart(const DAPMessage& msg);
    void onProgressUpdate(const DAPMessage& msg);
    void onProgressEnd(const DAPMessage& msg);
    void onInvalidated(const DAPMessage& msg);
    void onMemory(const DAPMessage& msg);
};

// Breakpoint manager
class BreakpointManager {
public:
    BreakpointManager();

    // Breakpoint management
    int addBreakpoint(const std::string& source, int line, int column = 0,
                      const std::string& condition = "", bool enabled = true);
    bool removeBreakpoint(int id);
    bool enableBreakpoint(int id, bool enabled = true);
    bool updateBreakpoint(int id, const std::string& condition = "",
                          const std::string& hitCondition = "");

    const std::unordered_map<int, Breakpoint>& getBreakpoints() const { return breakpoints_; }
    std::vector<Breakpoint> getBreakpointsForSource(const std::string& source) const;
    Breakpoint* getBreakpoint(int id);

    // Toggle
    int toggleBreakpoint(const std::string& source, int line);

    // Clear
    void clearAllBreakpoints();
    void clearBreakpointsForSource(const std::string& source);

private:
    std::unordered_map<int, Breakpoint> breakpoints_;
    int nextId_ = 1;
};

} // namespace NexusForge::Debugger
