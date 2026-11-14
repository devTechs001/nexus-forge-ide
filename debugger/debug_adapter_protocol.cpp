// debugger/debug_adapter_protocol.cpp
// Debug Adapter Protocol Implementation

#include "debug_adapter_protocol.hpp"
#include <sstream>
#include <algorithm>

namespace NexusForge::Debugger {

DebugAdapterProtocol::DebugAdapterProtocol() : initialized_(false), processId_(0) {
}

DebugAdapterProtocol::~DebugAdapterProtocol() {
    disconnect();
}

bool DebugAdapterProtocol::connect(const std::string& adapterPath, int port) {
    adapterPath_ = adapterPath;
    port_ = port;
    
    // In production, would spawn debug adapter process
    // and establish socket connection
    
    connected_ = true;
    return true;
}

void DebugAdapterProtocol::disconnect() {
    if (initialized_) {
        sendDisconnect();
    }
    
    connected_ = false;
    initialized_ = false;
    
    // Close connection
}

bool DebugAdapterProtocol::initialize() {
    if (!connected_) return false;
    
    // Send initialize request
    json request;
    request["seq"] = nextSeq_++;
    request["type"] = "request";
    request["command"] = "initialize";
    request["arguments"] = {
        {"clientID", "nexusforge"},
        {"clientName", "NexusForge IDE"},
        {"adapterID", "cpp"},
        {"pathFormat", "path"},
        {"linesStartAt1", true},
        {"columnsStartAt1", true},
        {"supportsVariableType", true},
        {"supportsVariablePaging", true},
        {"supportsRunInTerminalRequest", true},
        {"locale", "en-us"}
    };
    
    sendRequest(request);
    
    initialized_ = true;
    return true;
}

bool DebugAdapterProtocol::launch(const std::string& program, 
                                   const std::vector<std::string>& args,
                                   const std::string& cwd,
                                   const std::unordered_map<std::string, std::string>& env) {
    if (!initialized_) return false;
    
    json request;
    request["seq"] = nextSeq_++;
    request["type"] = "request";
    request["command"] = "launch";
    request["arguments"] = {
        {"program", program},
        {"args", args},
        {"cwd", cwd.empty() ? "." : cwd},
        {"env", env},
        {"stopOnEntry", false},
        {"externalConsole", false},
        {"debugServerPort", 0}
    };
    
    sendRequest(request);
    
    return true;
}

bool DebugAdapterProtocol::attach(int processId) {
    if (!initialized_) return false;
    
    processId_ = processId;
    
    json request;
    request["seq"] = nextSeq_++;
    request["type"] = "request";
    request["command"] = "attach";
    request["arguments"] = {
        {"processId", processId}
    };
    
    sendRequest(request);
    
    return true;
}

void DebugAdapterProtocol::disconnect() {
    json request;
    request["seq"] = nextSeq_++;
    request["type"] = "request";
    request["command"] = "disconnect";
    request["arguments"] = {
        {"restart", false},
        {"terminateDebuggee", true}
    };
    
    sendRequest(request);
}

void DebugAdapterProtocol::terminate() {
    json request;
    request["seq"] = nextSeq_++;
    request["type"] = "request";
    request["command"] = "terminate";
    
    sendRequest(request);
}

void DebugAdapterProtocol::pause(int threadId) {
    json request;
    request["seq"] = nextSeq_++;
    request["type"] = "request";
    request["command"] = "pause";
    request["arguments"] = {
        {"threadId", threadId}
    };
    
    sendRequest(request);
}

void DebugAdapterProtocol::continueExecution(int threadId) {
    json request;
    request["seq"] = nextSeq_++;
    request["type"] = "request";
    request["command"] = "continue";
    request["arguments"] = {
        {"threadId", threadId}
    };
    
    sendRequest(request);
}

void DebugAdapterProtocol::stepInto(int threadId) {
    json request;
    request["seq"] = nextSeq_++;
    request["type"] = "request";
    request["command"] = "stepIn";
    request["arguments"] = {
        {"threadId", threadId}
    };
    
    sendRequest(request);
}

void DebugAdapterProtocol::stepOver(int threadId) {
    json request;
    request["seq"] = nextSeq_++;
    request["type"] = "request";
    request["command"] = "stepOver";
    request["arguments"] = {
        {"threadId", threadId}
    };
    
    sendRequest(request);
}

void DebugAdapterProtocol::stepOut(int threadId) {
    json request;
    request["seq"] = nextSeq_++;
    request["type"] = "request";
    request["command"] = "stepOut";
    request["arguments"] = {
        {"threadId", threadId}
    };
    
    sendRequest(request);
}

void DebugAdapterProtocol::setBreakpoint(const std::string& sourcePath, int line, 
                                          const std::string& condition, int hitCondition) {
    json request;
    request["seq"] = nextSeq_++;
    request["type"] = "request";
    request["command"] = "setBreakpoints";
    request["arguments"] = {
        {"source", {{"path", sourcePath}}},
        {"breakpoints", {
            {{"line", line}, {"condition", condition}, {"hitCondition", hitCondition}}
        }},
        {"sourceModified", false}
    };
    
    sendRequest(request);
}

void DebugAdapterProtocol::removeBreakpoint(int breakpointId) {
    // Would need to update the breakpoint list and resend
}

void DebugAdapterProtocol::getThreads() {
    json request;
    request["seq"] = nextSeq_++;
    request["type"] = "request";
    request["command"] = "threads";
    
    sendRequest(request);
}

void DebugAdapterProtocol::getStackTrace(int threadId, int startFrame, int levels) {
    json request;
    request["seq"] = nextSeq_++;
    request["type"] = "request";
    request["command"] = "stackTrace";
    request["arguments"] = {
        {"threadId", threadId},
        {"startFrame", startFrame},
        {"levels", levels}
    };
    
    sendRequest(request);
}

void DebugAdapterProtocol::getScopes(int frameId) {
    json request;
    request["seq"] = nextSeq_++;
    request["type"] = "request";
    request["command"] = "scopes";
    request["arguments"] = {
        {"frameId", frameId}
    };
    
    sendRequest(request);
}

void DebugAdapterProtocol::getVariables(int variablesReference, 
                                         int filter, int start, int count) {
    json request;
    request["seq"] = nextSeq_++;
    request["type"] = "request";
    request["command"] = "variables";
    request["arguments"] = {
        {"variablesReference", variablesReference},
        {"filter", filter == 0 ? "all" : (filter == 1 ? "indexed" : "named")},
        {"start", start},
        {"count", count}
    };
    
    sendRequest(request);
}

void DebugAdapterProtocol::evaluate(const std::string& expression, int frameId, 
                                     const std::string& context) {
    json request;
    request["seq"] = nextSeq_++;
    request["type"] = "request";
    request["command"] = "evaluate";
    request["arguments"] = {
        {"expression", expression},
        {"frameId", frameId},
        {"context", context}
    };
    
    sendRequest(request);
}

void DebugAdapterProtocol::sendRequest(const json& request) {
    if (!connected_) return;
    
    // In production, would send over socket
    // DAP uses JSON-RPC like protocol
    
    std::string message = request.dump();
    
    // Format: Content-Length: <length>\r\n\r\n<message>
    std::ostringstream header;
    header << "Content-Length: " << message.length() << "\r\n\r\n";
    
    std::string fullMessage = header.str() + message;
    
    // Send to adapter
    // socket_.write(fullMessage);
}

void DebugAdapterProtocol::onMessageReceived(const std::string& message) {
    // Parse DAP message
    try {
        json msg = json::parse(message);
        
        std::string type = msg.value("type", "");
        
        if (type == "response") {
            handleResponse(msg);
        } else if (type == "event") {
            handleEvent(msg);
        }
    } catch (...) {
        // Parse error
    }
}

void DebugAdapterProtocol::handleResponse(const json& response) {
    int seq = response.value("seq", 0);
    std::string command = response.value("command", "");
    bool success = response.value("success", false);
    
    if (!success) {
        std::string errorMsg = response.value("message", "Unknown error");
        if (onError) {
            onError(errorMsg);
        }
        return;
    }
    
    // Handle specific responses
    if (command == "threads") {
        // Parse threads response
    } else if (command == "stackTrace") {
        // Parse stack trace response
    } else if (command == "variables") {
        // Parse variables response
    }
}

void DebugAdapterProtocol::handleEvent(const json& event) {
    std::string eventName = event.value("event", "");
    json body = event.value("body", json::object());
    
    if (eventName == "stopped") {
        std::string reason = body.value("reason", "");
        int threadId = body.value("threadId", 0);
        
        if (onStopped) {
            onStopped(reason, threadId);
        }
    } else if (eventName == "continued") {
        int threadId = body.value("threadId", 0);
        
        if (onContinued) {
            onContinued(threadId);
        }
    } else if (eventName == "terminated") {
        if (onTerminated) {
            onTerminated();
        }
    } else if (eventName == "output") {
        std::string output = body.value("output", "");
        std::string category = body.value("category", "stdout");
        
        if (onOutput) {
            onOutput(output, category);
        }
    } else if (eventName == "breakpoint") {
        // Breakpoint changed event
    }
}

bool DebugAdapterProtocol::isConnected() const {
    return connected_;
}

bool DebugAdapterProtocol::isInitialized() const {
    return initialized_;
}

int DebugAdapterProtocol::getProcessId() const {
    return processId_;
}

} // namespace NexusForge::Debugger
