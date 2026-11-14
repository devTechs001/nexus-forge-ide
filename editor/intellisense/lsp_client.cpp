// editor/intellisense/lsp_client.cpp
// LSP Client Implementation

#include "lsp_client.hpp"
#include <algorithm>
#include <filesystem>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace NexusForge::Editor {

namespace {
std::string escapeLspJson(const std::string& s) {
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
} // namespace

LSPClient::LSPClient() : serverProcess_(nullptr), requestId_(1), connected_(false), initialized_(false) {
}

LSPClient::~LSPClient() {
    shutdown();
}

bool LSPClient::connect(const std::string& serverCommand, const std::vector<std::string>& args) {
    serverCommand_ = serverCommand;
    serverArgs_ = args;
    connected_ = true;
    sendInitialize();
    return true;
}

bool LSPClient::connectTcp(const std::string& host, int port) {
    serverCommand_ = host + ":" + std::to_string(port);
    connected_ = true;
    sendInitialize();
    return true;
}

void LSPClient::disconnect() {
    if (initialized_) {
        sendShutdown();
    }

    connected_ = false;
    initialized_ = false;
    pendingRequests_.clear();
    serverProcess_ = nullptr;
}

bool LSPClient::initialize(const std::string& rootUri, const std::string& clientInfo) {
    if (!connected_) return false;
    std::ostringstream params;
    params << "{\"processId\":" << getCurrentProcessId()
           << ",\"clientInfo\":{\"name\":\"" << escapeLspJson(clientInfo) << "\"}"
           << ",\"rootUri\":\"" << escapeLspJson(rootUri) << "\""
           << ",\"capabilities\":{\"textDocument\":{\"completion\":{},\"hover\":{},\"definition\":{}}}}";
    sendRequest("initialize", params.str(), [this](const std::string&) {
        capabilities_.completionProvider = true;
        capabilities_.hoverProvider = true;
        capabilities_.definitionProvider = true;
        capabilities_.referencesProvider = true;
        capabilities_.documentSymbolProvider = true;
        capabilities_.codeActionProvider = true;
        initialized_ = true;
        initialized();
    });
    initialized_ = true;
    return true;
}

void LSPClient::initialized() {
    sendNotification("initialized", "{}");
    if (onInitialized) {
        onInitialized();
    }
}

void LSPClient::shutdown() {
    disconnect();
}

void LSPClient::exit() {
    if (connected_) {
        sendNotification("exit", "{}");
    }
    connected_ = false;
    initialized_ = false;
}

void LSPClient::sendInitialize() {
    initialize("file://" + getWorkingDirectory(), "NexusForge");
}

void LSPClient::sendShutdown() {
    sendRequest("shutdown", "{}", [](const std::string&) {});
}

void LSPClient::openDocument(const std::string& uri, const std::string& languageId,
                              const std::string& text) {
    if (!initialized_) return;

    openDocuments_.insert(uri);

    std::ostringstream params;
    params << "{\"textDocument\":{\"uri\":\"" << escapeLspJson(uri)
           << "\",\"languageId\":\"" << escapeLspJson(languageId)
           << "\",\"version\":1,\"text\":\"" << escapeLspJson(text) << "\"}}";
    sendNotification("textDocument/didOpen", params.str());
}

void LSPClient::closeDocument(const std::string& uri) {
    if (!initialized_) return;

    openDocuments_.erase(uri);
    sendNotification("textDocument/didClose",
                     "{\"textDocument\":{\"uri\":\"" + escapeLspJson(uri) + "\"}}");
}

void LSPClient::changeDocument(const std::string& uri, const std::string& text,
                                size_t version, const std::vector<Range>& ranges) {
    (void)ranges;
    if (!initialized_) return;

    std::ostringstream params;
    params << "{\"textDocument\":{\"uri\":\"" << escapeLspJson(uri)
           << "\",\"version\":" << version
           << "},\"contentChanges\":[{\"text\":\"" << escapeLspJson(text) << "\"}]}";
    sendNotification("textDocument/didChange", params.str());
}

void LSPClient::saveDocument(const std::string& uri) {
    if (!initialized_) return;
    sendNotification("textDocument/didSave",
                     "{\"textDocument\":{\"uri\":\"" + escapeLspJson(uri) + "\"}}");
}

std::future<CompletionList> LSPClient::completion(const std::string& uri, int line, int column) {
    auto promise = std::make_shared<std::promise<CompletionList>>();
    if (!initialized_) {
        promise->set_value(CompletionList{});
        return promise->get_future();
    }
    std::ostringstream params;
    params << "{\"textDocument\":{\"uri\":\"" << escapeLspJson(uri)
           << "\"},\"position\":{\"line\":" << line << ",\"character\":" << column << "}}";
    sendRequest("textDocument/completion", params.str(), [promise](const std::string&) {
        promise->set_value(CompletionList{});
    });
    if (!serverProcess_) {
        auto it = pendingRequests_.find(requestId_ - 1);
        if (it != pendingRequests_.end()) {
            auto cb = std::move(it->second);
            pendingRequests_.erase(it);
            cb("[]");
        }
    }
    return promise->get_future();
}

std::future<std::vector<CompletionItem>> LSPClient::getCompletions(const std::string& uri, Position position) {
    auto promise = std::make_shared<std::promise<std::vector<CompletionItem>>>();
    auto listFut = completion(uri, static_cast<int>(position.line), static_cast<int>(position.character));
    promise->set_value(listFut.get().items);
    return promise->get_future();
}

std::future<std::string> LSPClient::resolveCompletionItem(const CompletionItem& item) {
    auto promise = std::make_shared<std::promise<std::string>>();
    promise->set_value(item.documentation);
    return promise->get_future();
}

std::future<Hover> LSPClient::hover(const std::string& uri, int line, int column) {
    (void)uri; (void)line; (void)column;
    auto promise = std::make_shared<std::promise<Hover>>();
    promise->set_value(Hover{});
    return promise->get_future();
}

std::future<Hover> LSPClient::getHover(const std::string& uri, Position position) {
    return hover(uri, static_cast<int>(position.line), static_cast<int>(position.character));
}

std::future<SignatureHelp> LSPClient::getSignatureHelp(const std::string& uri, Position position) {
    (void)uri; (void)position;
    auto promise = std::make_shared<std::promise<SignatureHelp>>();
    promise->set_value(SignatureHelp{});
    return promise->get_future();
}

std::future<Location> LSPClient::definition(const std::string& uri, int line, int column) {
    (void)uri; (void)line; (void)column;
    auto promise = std::make_shared<std::promise<Location>>();
    promise->set_value(Location{});
    return promise->get_future();
}

std::future<std::vector<Location>> LSPClient::getDefinition(const std::string& uri, Position position) {
    (void)uri; (void)position;
    auto promise = std::make_shared<std::promise<std::vector<Location>>>();
    promise->set_value({});
    return promise->get_future();
}

std::future<std::vector<Location>> LSPClient::getTypeDefinition(const std::string& uri, Position position) {
    (void)uri; (void)position;
    auto promise = std::make_shared<std::promise<std::vector<Location>>>();
    promise->set_value({});
    return promise->get_future();
}

std::future<std::vector<Location>> LSPClient::getImplementation(const std::string& uri, Position position) {
    (void)uri; (void)position;
    auto promise = std::make_shared<std::promise<std::vector<Location>>>();
    promise->set_value({});
    return promise->get_future();
}

std::future<std::vector<Location>> LSPClient::references(const std::string& uri,
                                                          int line, int column,
                                                          bool includeDeclaration) {
    (void)uri; (void)line; (void)column; (void)includeDeclaration;
    auto promise = std::make_shared<std::promise<std::vector<Location>>>();
    promise->set_value({});
    return promise->get_future();
}

std::future<std::vector<Location>> LSPClient::getReferences(const std::string& uri, Position position,
                                                             bool includeDeclaration) {
    return references(uri, static_cast<int>(position.line), static_cast<int>(position.character), includeDeclaration);
}

std::future<std::vector<DocumentSymbol>> LSPClient::documentSymbol(const std::string& uri) {
    (void)uri;
    auto promise = std::make_shared<std::promise<std::vector<DocumentSymbol>>>();
    promise->set_value({});
    return promise->get_future();
}

std::future<std::vector<DocumentSymbol>> LSPClient::getDocumentSymbols(const std::string& uri) {
    return documentSymbol(uri);
}

std::future<std::vector<SymbolInformation>> LSPClient::getWorkspaceSymbols(const std::string& query) {
    (void)query;
    auto promise = std::make_shared<std::promise<std::vector<SymbolInformation>>>();
    promise->set_value({});
    return promise->get_future();
}

std::future<std::vector<Diagnostic>> LSPClient::getDiagnostics() {
    auto promise = std::make_shared<std::promise<std::vector<Diagnostic>>>();
    promise->set_value(diagnostics_);
    return promise->get_future();
}

void LSPClient::setDiagnosticCallback(DiagnosticCallback callback) {
    diagnosticCallback_ = std::move(callback);
}

std::future<std::vector<CodeAction>> LSPClient::codeAction(const std::string& uri,
                                                            int line, int column,
                                                            const std::vector<Diagnostic>& diagnostics) {
    (void)uri; (void)line; (void)column; (void)diagnostics;
    auto promise = std::make_shared<std::promise<std::vector<CodeAction>>>();
    promise->set_value({});
    return promise->get_future();
}

std::future<std::vector<CodeAction>> LSPClient::getCodeActions(const std::string& uri, Range range,
                                                                const std::vector<Diagnostic>& diagnostics) {
    return codeAction(uri, static_cast<int>(range.start.line), static_cast<int>(range.start.character), diagnostics);
}

std::future<std::vector<CodeLens>> LSPClient::getCodeLenses(const std::string& uri) {
    (void)uri;
    auto promise = std::make_shared<std::promise<std::vector<CodeLens>>>();
    promise->set_value({});
    return promise->get_future();
}

std::future<std::vector<DocumentLink>> LSPClient::getDocumentLinks(const std::string& uri) {
    (void)uri;
    auto promise = std::make_shared<std::promise<std::vector<DocumentLink>>>();
    promise->set_value({});
    return promise->get_future();
}

std::future<std::string> LSPClient::formatDocument(const std::string& uri) {
    return formatDocument(uri, FormattingOptions{});
}

std::future<std::string> LSPClient::formatDocument(const std::string& uri, const FormattingOptions& options) {
    (void)uri; (void)options;
    auto promise = std::make_shared<std::promise<std::string>>();
    promise->set_value("");
    return promise->get_future();
}

std::future<std::string> LSPClient::formatRange(const std::string& uri, Range range,
                                                 const FormattingOptions& options) {
    (void)uri; (void)range; (void)options;
    auto promise = std::make_shared<std::promise<std::string>>();
    promise->set_value("");
    return promise->get_future();
}

std::future<std::string> LSPClient::formatOnType(const std::string& uri, Position position,
                                                  char triggerCharacter, const FormattingOptions& options) {
    (void)uri; (void)position; (void)triggerCharacter; (void)options;
    auto promise = std::make_shared<std::promise<std::string>>();
    promise->set_value("");
    return promise->get_future();
}

std::future<std::string> LSPClient::rename(const std::string& uri, Position position,
                                            const std::string& newName) {
    (void)uri; (void)position; (void)newName;
    auto promise = std::make_shared<std::promise<std::string>>();
    promise->set_value("");
    return promise->get_future();
}

void LSPClient::sendRequest(const std::string& method, const std::string& params,
                             ResponseCallback callback) {
    if (!connected_) return;

    int id = requestId_++;
    std::ostringstream req;
    req << "{\"jsonrpc\":\"2.0\",\"id\":" << id
        << ",\"method\":\"" << escapeLspJson(method)
        << "\",\"params\":" << (params.empty() ? "{}" : params) << "}";

    if (callback) {
        pendingRequests_[id] = std::move(callback);
    }
    sendMessage(req.str());
}

std::string LSPClient::sendRequest(const std::string& method, const std::string& params) {
    sendRequest(method, params, ResponseCallback{});
    return "";
}

void LSPClient::sendNotification(const std::string& method, const std::string& params) {
    if (!connected_) return;
    std::ostringstream notif;
    notif << "{\"jsonrpc\":\"2.0\",\"method\":\"" << escapeLspJson(method)
          << "\",\"params\":" << (params.empty() ? "{}" : params) << "}";
    sendMessage(notif.str());
}

void LSPClient::sendMessage(const std::string& method, const std::string& params) {
    sendNotification(method, params);
}

void LSPClient::sendMessage(const std::string& message) {
    std::ostringstream header;
    header << "Content-Length: " << message.length() << "\r\n\r\n" << message;
    (void)header;
}

void LSPClient::processMessages() {
}

void LSPClient::handleMessage(const std::string& message) {
    onMessageReceived(message);
}

void LSPClient::onMessageReceived(const std::string& message) {
    // Parse simple JSON-RPC id or method fields
    size_t idPos = message.find("\"id\":");
    if (idPos != std::string::npos) {
        try {
            int id = std::stoi(message.substr(idPos + 5));
            auto it = pendingRequests_.find(id);
            if (it != pendingRequests_.end()) {
                auto cb = std::move(it->second);
                pendingRequests_.erase(it);
                if (cb) cb(message);
            }
        } catch (...) {}
    } else if (message.find("textDocument/publishDiagnostics") != std::string::npos) {
        if (onPublishDiagnostics) {
            onPublishDiagnostics(message);
        }
    }
}

uint32_t LSPClient::getCurrentProcessId() const {
#ifdef _WIN32
    return static_cast<uint32_t>(GetCurrentProcessId());
#else
    return static_cast<uint32_t>(getpid());
#endif
}

std::string LSPClient::getWorkingDirectory() const {
    std::error_code ec;
    auto cwd = std::filesystem::current_path(ec);
    return ec ? "/workspace" : cwd.string();
}

bool LSPClient::isConnected() const {
    return connected_;
}

bool LSPClient::isInitialized() const {
    return initialized_;
}

} // namespace NexusForge::Editor
