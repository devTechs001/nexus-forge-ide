// debugger/debug_adapter_protocol.cpp
// Debug Adapter Protocol Implementation

#include "debug_adapter_protocol.hpp"
#include <sstream>
#include <algorithm>

namespace NexusForge::Debugger {

namespace {
std::string escapeDapJson(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

std::string extractJsonString(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\":\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    size_t end = json.find('"', pos);
    if (end == std::string::npos) return "";
    return json.substr(pos, end - pos);
}

int extractJsonInt(const std::string& json, const std::string& key, int fallback = 0) {
    std::string needle = "\"" + key + "\":";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return fallback;
    try {
        return std::stoi(json.substr(pos + needle.size()));
    } catch (...) {
        return fallback;
    }
}
} // namespace

// DebugAdapterSession implementation
DebugAdapterSession::DebugAdapterSession() = default;
DebugAdapterSession::~DebugAdapterSession() { disconnect(); }

bool DebugAdapterSession::connect(const std::string& adapterPath, const std::string& adapterArgs) {
    (void)adapterPath; (void)adapterArgs;
    connected_ = true;
    return true;
}

bool DebugAdapterSession::connectViaTcp(const std::string& host, int port) {
    (void)host; (void)port;
    connected_ = true;
    return true;
}

void DebugAdapterSession::disconnect() {
    connected_ = false;
    pendingRequests_.clear();
}

bool DebugAdapterSession::initialize(const std::string& adapterId) {
    if (!connected_) return false;
    sendMessage("initialize", {{"adapterID", adapterId}});
    return true;
}

bool DebugAdapterSession::launch(const std::unordered_map<std::string, std::string>& config) {
    if (!connected_) return false;
    sendMessage("launch", {{"config", config}});
    return true;
}

bool DebugAdapterSession::attach(int processId) {
    if (!connected_) return false;
    sendMessage("attach", {{"processId", processId}});
    return true;
}

bool DebugAdapterSession::continueExecution(int threadId) {
    if (!connected_) return false;
    sendMessage("continue", {{"threadId", threadId}});
    return true;
}

bool DebugAdapterSession::next(int threadId) {
    if (!connected_) return false;
    sendMessage("next", {{"threadId", threadId}});
    return true;
}

bool DebugAdapterSession::stepIn(int threadId) {
    if (!connected_) return false;
    sendMessage("stepIn", {{"threadId", threadId}});
    return true;
}

bool DebugAdapterSession::stepOut(int threadId) {
    if (!connected_) return false;
    sendMessage("stepOut", {{"threadId", threadId}});
    return true;
}

bool DebugAdapterSession::stepBack(int threadId) {
    if (!connected_) return false;
    sendMessage("stepBack", {{"threadId", threadId}});
    return true;
}

bool DebugAdapterSession::reverseContinue(int threadId) {
    if (!connected_) return false;
    sendMessage("reverseContinue", {{"threadId", threadId}});
    return true;
}

bool DebugAdapterSession::pause(int threadId) {
    if (!connected_) return false;
    sendMessage("pause", {{"threadId", threadId}});
    return true;
}

bool DebugAdapterSession::terminate() {
    if (!connected_) return false;
    sendMessage("terminate");
    return true;
}

bool DebugAdapterSession::restart() {
    if (!connected_) return false;
    sendMessage("restart");
    return true;
}

std::vector<Breakpoint> DebugAdapterSession::setBreakpoints(
    const std::string& source, const std::vector<Breakpoint>& breakpoints) {
    (void)source;
    return breakpoints;
}

bool DebugAdapterSession::setFunctionBreakpoints(const std::vector<std::string>& functions) {
    (void)functions;
    return connected_;
}

bool DebugAdapterSession::setDataBreakpoints(const std::vector<std::pair<std::string, std::string>>& data) {
    (void)data;
    return connected_;
}

bool DebugAdapterSession::setExceptionBreakpoints(const std::vector<std::string>& filters) {
    (void)filters;
    return connected_;
}

bool DebugAdapterSession::clearAllBreakpoints() { return connected_; }

std::vector<StackFrame> DebugAdapterSession::getStackTrace(int threadId, int startFrame, int levels) {
    (void)threadId; (void)startFrame; (void)levels;
    return {};
}

std::vector<Scope> DebugAdapterSession::getScopes(int frameId) { (void)frameId; return {}; }
std::vector<Variable> DebugAdapterSession::getVariables(int variablesReference, int start, int count) {
    (void)variablesReference; (void)start; (void)count;
    return {};
}
std::string DebugAdapterSession::evaluate(const std::string& expression, int frameId, const std::string& context) {
    (void)expression; (void)frameId; (void)context;
    return "";
}
std::vector<Thread> DebugAdapterSession::getThreads() { return {}; }
std::string DebugAdapterSession::getSource(int sourceReference) { (void)sourceReference; return ""; }
std::string DebugAdapterSession::getSourceFile(const std::string& path) { (void)path; return ""; }
std::vector<CompletionItem> DebugAdapterSession::getCompletions(const std::string& text, int column, int line) {
    (void)text; (void)column; (void)line;
    return {};
}
std::string DebugAdapterSession::getHover(const std::string& expression, int frameId) {
    (void)expression; (void)frameId;
    return "";
}
bool DebugAdapterSession::restartFrame(int frameId) { (void)frameId; return connected_; }
bool DebugAdapterSession::gotoTarget(int threadId, int targetId) { (void)threadId; (void)targetId; return connected_; }
std::vector<Source> DebugAdapterSession::getLoadedSources() { return {}; }
std::vector<DebugAdapterSession::Module> DebugAdapterSession::getModules() { return {}; }
std::vector<DebugAdapterSession::DisassembledInstruction> DebugAdapterSession::disassemble(
    const std::string& memoryReference, int instructionOffset, int instructionCount, int resolveSymbols) {
    (void)memoryReference; (void)instructionOffset; (void)instructionCount; (void)resolveSymbols;
    return {};
}
std::vector<uint8_t> DebugAdapterSession::readMemory(const std::string& memoryReference, int offset, int count) {
    (void)memoryReference; (void)offset; (void)count;
    return {};
}
bool DebugAdapterSession::writeMemory(const std::string& memoryReference, int offset, const std::vector<uint8_t>& data) {
    (void)memoryReference; (void)offset; (void)data;
    return connected_;
}
bool DebugAdapterSession::setVariable(int variablesReference, const std::string& name, const std::string& value) {
    (void)variablesReference; (void)name; (void)value;
    return connected_;
}
bool DebugAdapterSession::setExpression(const std::string& expression, const std::string& value) {
    (void)expression; (void)value;
    return connected_;
}
std::optional<DebugAdapterSession::ExceptionInfo> DebugAdapterSession::getExceptionInfo(int threadId) {
    (void)threadId;
    return std::nullopt;
}

DAPMessage DebugAdapterSession::sendMessage(
    const std::string& command,
    const std::unordered_map<std::string, std::variant<
        bool, int, double, std::string,
        std::vector<std::string>,
        std::unordered_map<std::string, std::string>
    >>& body) {
    DAPMessage msg;
    msg.seq = ++sequenceNumber_;
    msg.type = "request";
    msg.command = command;
    msg.body = body;
    if (messageCallback_) messageCallback_(msg);
    return msg;
}

DAPMessage DebugAdapterSession::waitForResponse(int seq) { DAPMessage m; m.seq = seq; return m; }
void DebugAdapterSession::processMessage(const DAPMessage& msg) {
    if (msg.type == "event") handleEvent(msg.command, msg);
}
void DebugAdapterSession::handleEvent(const std::string& event, const DAPMessage& msg) {
    if (eventCallback_) eventCallback_(event, msg);
}
void DebugAdapterSession::onInitialized(const DAPMessage& msg) { (void)msg; }
void DebugAdapterSession::onStopped(const DAPMessage& msg) { (void)msg; }
void DebugAdapterSession::onContinued(const DAPMessage& msg) { (void)msg; }
void DebugAdapterSession::onExited(const DAPMessage& msg) { (void)msg; }
void DebugAdapterSession::onTerminated(const DAPMessage& msg) { (void)msg; }
void DebugAdapterSession::onBreakpoint(const DAPMessage& msg) { (void)msg; }
void DebugAdapterSession::onOutput(const DAPMessage& msg) { (void)msg; }
void DebugAdapterSession::onThread(const DAPMessage& msg) { (void)msg; }
void DebugAdapterSession::onModule(const DAPMessage& msg) { (void)msg; }
void DebugAdapterSession::onLoadedSource(const DAPMessage& msg) { (void)msg; }
void DebugAdapterSession::onProcess(const DAPMessage& msg) { (void)msg; }
void DebugAdapterSession::onCapabilities(const DAPMessage& msg) { (void)msg; }
void DebugAdapterSession::onProgressStart(const DAPMessage& msg) { (void)msg; }
void DebugAdapterSession::onProgressUpdate(const DAPMessage& msg) { (void)msg; }
void DebugAdapterSession::onProgressEnd(const DAPMessage& msg) { (void)msg; }
void DebugAdapterSession::onInvalidated(const DAPMessage& msg) { (void)msg; }
void DebugAdapterSession::onMemory(const DAPMessage& msg) { (void)msg; }

// DebugAdapterProtocol implementation
DebugAdapterProtocol::DebugAdapterProtocol()
    : port_(0), connected_(false), initialized_(false), processId_(0), nextSeq_(1) {
}

DebugAdapterProtocol::~DebugAdapterProtocol() {
    disconnect();
}

bool DebugAdapterProtocol::connect(const std::string& adapterPath, int port) {
    adapterPath_ = adapterPath;
    port_ = port;
    connected_ = true;
    return true;
}

void DebugAdapterProtocol::sendDisconnect() {
    std::ostringstream req;
    req << "{\"seq\":" << nextSeq_++
        << ",\"type\":\"request\",\"command\":\"disconnect\","
        << "\"arguments\":{\"restart\":false,\"terminateDebuggee\":true}}";
    sendRequest(req.str());
}

void DebugAdapterProtocol::disconnect() {
    if (initialized_) {
        sendDisconnect();
    }
    connected_ = false;
    initialized_ = false;
}

bool DebugAdapterProtocol::initialize() {
    if (!connected_) return false;

    std::ostringstream req;
    req << "{\"seq\":" << nextSeq_++
        << ",\"type\":\"request\",\"command\":\"initialize\","
        << "\"arguments\":{\"clientID\":\"nexusforge\",\"clientName\":\"NexusForge IDE\","
        << "\"adapterID\":\"cpp\",\"pathFormat\":\"path\",\"linesStartAt1\":true,\"columnsStartAt1\":true}}";
    sendRequest(req.str());

    initialized_ = true;
    return true;
}

bool DebugAdapterProtocol::launch(const std::string& program,
                                   const std::vector<std::string>& args,
                                   const std::string& cwd,
                                   const std::unordered_map<std::string, std::string>& env) {
    if (!initialized_) return false;
    (void)args;
    (void)env;

    std::ostringstream req;
    req << "{\"seq\":" << nextSeq_++
        << ",\"type\":\"request\",\"command\":\"launch\","
        << "\"arguments\":{\"program\":\"" << escapeDapJson(program)
        << "\",\"cwd\":\"" << escapeDapJson(cwd.empty() ? "." : cwd)
        << "\",\"stopOnEntry\":false}}";
    sendRequest(req.str());
    return true;
}

bool DebugAdapterProtocol::attach(int processId) {
    if (!initialized_) return false;
    processId_ = processId;
    std::ostringstream req;
    req << "{\"seq\":" << nextSeq_++
        << ",\"type\":\"request\",\"command\":\"attach\","
        << "\"arguments\":{\"processId\":" << processId << "}}";
    sendRequest(req.str());
    return true;
}

void DebugAdapterProtocol::terminate() {
    std::ostringstream req;
    req << "{\"seq\":" << nextSeq_++ << ",\"type\":\"request\",\"command\":\"terminate\"}";
    sendRequest(req.str());
}

void DebugAdapterProtocol::pause(int threadId) {
    std::ostringstream req;
    req << "{\"seq\":" << nextSeq_++
        << ",\"type\":\"request\",\"command\":\"pause\",\"arguments\":{\"threadId\":" << threadId << "}}";
    sendRequest(req.str());
}

void DebugAdapterProtocol::continueExecution(int threadId) {
    std::ostringstream req;
    req << "{\"seq\":" << nextSeq_++
        << ",\"type\":\"request\",\"command\":\"continue\",\"arguments\":{\"threadId\":" << threadId << "}}";
    sendRequest(req.str());
}

void DebugAdapterProtocol::stepInto(int threadId) {
    std::ostringstream req;
    req << "{\"seq\":" << nextSeq_++
        << ",\"type\":\"request\",\"command\":\"stepIn\",\"arguments\":{\"threadId\":" << threadId << "}}";
    sendRequest(req.str());
}

void DebugAdapterProtocol::stepOver(int threadId) {
    std::ostringstream req;
    req << "{\"seq\":" << nextSeq_++
        << ",\"type\":\"request\",\"command\":\"next\",\"arguments\":{\"threadId\":" << threadId << "}}";
    sendRequest(req.str());
}

void DebugAdapterProtocol::stepOut(int threadId) {
    std::ostringstream req;
    req << "{\"seq\":" << nextSeq_++
        << ",\"type\":\"request\",\"command\":\"stepOut\",\"arguments\":{\"threadId\":" << threadId << "}}";
    sendRequest(req.str());
}

void DebugAdapterProtocol::setBreakpoint(const std::string& sourcePath, int line,
                                          const std::string& condition, int hitCondition) {
    std::ostringstream req;
    req << "{\"seq\":" << nextSeq_++
        << ",\"type\":\"request\",\"command\":\"setBreakpoints\",\"arguments\":{"
        << "\"source\":{\"path\":\"" << escapeDapJson(sourcePath) << "\"},"
        << "\"breakpoints\":[{\"line\":" << line
        << ",\"condition\":\"" << escapeDapJson(condition)
        << "\",\"hitCondition\":\"" << hitCondition << "\"}]}}";
    sendRequest(req.str());
}

void DebugAdapterProtocol::removeBreakpoint(int breakpointId) {
    (void)breakpointId;
}

void DebugAdapterProtocol::getThreads() {
    std::ostringstream req;
    req << "{\"seq\":" << nextSeq_++ << ",\"type\":\"request\",\"command\":\"threads\"}";
    sendRequest(req.str());
}

void DebugAdapterProtocol::getStackTrace(int threadId, int startFrame, int levels) {
    std::ostringstream req;
    req << "{\"seq\":" << nextSeq_++
        << ",\"type\":\"request\",\"command\":\"stackTrace\",\"arguments\":{\"threadId\":"
        << threadId << ",\"startFrame\":" << startFrame << ",\"levels\":" << levels << "}}";
    sendRequest(req.str());
}

void DebugAdapterProtocol::getScopes(int frameId) {
    std::ostringstream req;
    req << "{\"seq\":" << nextSeq_++
        << ",\"type\":\"request\",\"command\":\"scopes\",\"arguments\":{\"frameId\":" << frameId << "}}";
    sendRequest(req.str());
}

void DebugAdapterProtocol::getVariables(int variablesReference, int filter, int start, int count) {
    std::ostringstream req;
    req << "{\"seq\":" << nextSeq_++
        << ",\"type\":\"request\",\"command\":\"variables\",\"arguments\":{\"variablesReference\":"
        << variablesReference << ",\"filter\":\""
        << (filter == 0 ? "all" : (filter == 1 ? "indexed" : "named"))
        << "\",\"start\":" << start << ",\"count\":" << count << "}}";
    sendRequest(req.str());
}

void DebugAdapterProtocol::evaluate(const std::string& expression, int frameId, const std::string& context) {
    std::ostringstream req;
    req << "{\"seq\":" << nextSeq_++
        << ",\"type\":\"request\",\"command\":\"evaluate\",\"arguments\":{\"expression\":\""
        << escapeDapJson(expression) << "\",\"frameId\":" << frameId
        << ",\"context\":\"" << escapeDapJson(context) << "\"}}";
    sendRequest(req.str());
}

void DebugAdapterProtocol::sendRequest(const std::string& requestJson) {
    if (!connected_) return;
    std::ostringstream framed;
    framed << "Content-Length: " << requestJson.length() << "\r\n\r\n" << requestJson;
    (void)framed;
}

void DebugAdapterProtocol::onMessageReceived(const std::string& message) {
    std::string type = extractJsonString(message, "type");
    if (type == "response") {
        handleResponse(message);
    } else if (type == "event") {
        handleEvent(message);
    }
}

void DebugAdapterProtocol::handleResponse(const std::string& responseJson) {
    if (responseJson.find("\"success\":false") != std::string::npos) {
        std::string errorMsg = extractJsonString(responseJson, "message");
        if (errorMsg.empty()) errorMsg = "Unknown error";
        if (onError) onError(errorMsg);
    }
}

void DebugAdapterProtocol::handleEvent(const std::string& eventJson) {
    std::string eventName = extractJsonString(eventJson, "event");
    if (eventName == "stopped") {
        std::string reason = extractJsonString(eventJson, "reason");
        int threadId = extractJsonInt(eventJson, "threadId", 0);
        if (onStopped) onStopped(reason, threadId);
    } else if (eventName == "continued") {
        int threadId = extractJsonInt(eventJson, "threadId", 0);
        if (onContinued) onContinued(threadId);
    } else if (eventName == "terminated") {
        if (onTerminated) onTerminated();
    } else if (eventName == "output") {
        std::string output = extractJsonString(eventJson, "output");
        std::string category = extractJsonString(eventJson, "category");
        if (category.empty()) category = "stdout";
        if (onOutput) onOutput(output, category);
    }
}

bool DebugAdapterProtocol::isConnected() const { return connected_; }
bool DebugAdapterProtocol::isInitialized() const { return initialized_; }
int DebugAdapterProtocol::getProcessId() const { return processId_; }

} // namespace NexusForge::Debugger
