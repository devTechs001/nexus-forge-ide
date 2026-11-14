// editor/intellisense/lsp_client.cpp
// LSP Client Implementation

#include "lsp_client.hpp"
#include <algorithm>
#include <sstream>

namespace NexusForge::Editor {

LSPClient::LSPClient() : serverProcess_(nullptr), requestId_(1), initialized_(false) {
}

LSPClient::~LSPClient() {
    shutdown();
}

bool LSPClient::connect(const std::string& serverCommand, const std::vector<std::string>& args) {
    serverCommand_ = serverCommand;
    serverArgs_ = args;
    
    // In production, spawn the language server process
    // For now, mark as connected
    connected_ = true;
    
    // Send initialize request
    sendInitialize();
    
    return true;
}

void LSPClient::disconnect() {
    if (initialized_) {
        sendShutdown();
    }
    
    connected_ = false;
    initialized_ = false;
    
    if (serverProcess_) {
        // Kill server process
        serverProcess_ = nullptr;
    }
}

void LSPClient::shutdown() {
    disconnect();
}

void LSPClient::sendInitialize() {
    if (!connected_) return;
    
    // Build initialize params
    InitializeParams params;
    params.processId = getCurrentProcessId();
    params.rootUri = "file://" + getWorkingDirectory();
    params.capabilities.textDocument.completion = true;
    params.capabilities.textDocument.hover = true;
    params.capabilities.textDocument.definition = true;
    params.capabilities.textDocument.references = true;
    params.capabilities.textDocument.documentSymbol = true;
    params.capabilities.textDocument.codeAction = true;
    
    sendRequest("initialize", params, [this](const json& result) {
        InitializeResult initResult = result.get<InitializeResult>();
        serverCapabilities_ = initResult.capabilities;
        initialized_ = true;
        
        // Send initialized notification
        sendNotification("initialized", {});
        
        if (onInitialized) {
            onInitialized();
        }
    });
}

void LSPClient::sendShutdown() {
    sendRequest("shutdown", {}, [this](const json&) {
        // Server is shutting down
    });
}

void LSPClient::openDocument(const std::string& uri, const std::string& languageId,
                              const std::string& text) {
    if (!initialized_) return;
    
    openDocuments_.insert(uri);
    
    TextDocumentItem item;
    item.uri = uri;
    item.languageId = languageId;
    item.version = 1;
    item.text = text;
    
    sendNotification("textDocument/didOpen", {{"textDocument", item}});
}

void LSPClient::closeDocument(const std::string& uri) {
    if (!initialized_) return;
    
    openDocuments_.erase(uri);
    
    TextDocumentIdentifier doc;
    doc.uri = uri;
    
    sendNotification("textDocument/didClose", {{"textDocument", doc}});
}

void LSPClient::changeDocument(const std::string& uri, const std::string& text,
                                int version) {
    if (!initialized_) return;
    
    TextDocumentIdentifier doc;
    doc.uri = uri;
    
    VersionedTextDocumentIdentifier versionedDoc;
    versionedDoc.uri = uri;
    versionedDoc.version = version;
    
    TextDocumentContentChangeEvent change;
    change.text = text;
    
    sendNotification("textDocument/didChange", {
        {"textDocument", versionedDoc},
        {"contentChanges", {change}}
    });
}

std::future<CompletionList> LSPClient::completion(const std::string& uri,
                                                    int line, int column) {
    auto promise = std::make_shared<std::promise<CompletionList>>();
    
    if (!initialized_) {
        promise->set_value(CompletionList{});
        return promise->get_future();
    }
    
    TextDocumentPositionParams params;
    params.textDocument.uri = uri;
    params.position.line = line;
    params.position.character = column;
    
    sendRequest("textDocument/completion", params, [promise](const json& result) {
        CompletionList list;
        
        if (result.is_array()) {
            for (const auto& item : result) {
                CompletionItem ci;
                ci.label = item.value("label", "");
                ci.kind = static_cast<CompletionItemKind>(item.value("kind", 0));
                ci.detail = item.value("detail", "");
                ci.documentation = item.value("documentation", "");
                ci.sortText = item.value("sortText", "");
                ci.filterText = item.value("filterText", "");
                ci.insertText = item.value("insertText", ci.label);
                list.items.push_back(ci);
            }
        }
        
        promise->set_value(list);
    });
    
    return promise->get_future();
}

std::future<Hover> LSPClient::hover(const std::string& uri, int line, int column) {
    auto promise = std::make_shared<std::promise<Hover>>();
    
    if (!initialized_) {
        promise->set_value(Hover{});
        return promise->get_future();
    }
    
    TextDocumentPositionParams params;
    params.textDocument.uri = uri;
    params.position.line = line;
    params.position.character = column;
    
    sendRequest("textDocument/hover", params, [promise](const json& result) {
        Hover hover;
        if (!result.is_null()) {
            // Parse hover result
            if (result.contains("contents")) {
                // Handle contents
            }
        }
        promise->set_value(hover);
    });
    
    return promise->get_future();
}

std::future<Location> LSPClient::definition(const std::string& uri, int line, int column) {
    auto promise = std::make_shared<std::promise<Location>>();
    
    if (!initialized_) {
        promise->set_value(Location{});
        return promise->get_future();
    }
    
    TextDocumentPositionParams params;
    params.textDocument.uri = uri;
    params.position.line = line;
    params.position.character = column;
    
    sendRequest("textDocument/definition", params, [promise](const json& result) {
        Location loc;
        
        if (result.is_array() && !result.empty()) {
            const auto& item = result[0];
            loc.uri = item.value("uri", "");
            
            if (item.contains("range")) {
                // Parse range
            }
        }
        
        promise->set_value(loc);
    });
    
    return promise->get_future();
}

std::future<std::vector<Location>> LSPClient::references(const std::string& uri,
                                                          int line, int column,
                                                          bool includeDeclaration) {
    auto promise = std::make_shared<std::promise<std::vector<Location>>>();
    
    if (!initialized_) {
        promise->set_value({});
        return promise->get_future();
    }
    
    ReferenceParams params;
    params.textDocument.uri = uri;
    params.position.line = line;
    params.position.character = column;
    params.context.includeDeclaration = includeDeclaration;
    
    sendRequest("textDocument/references", params, [promise](const json& result) {
        std::vector<Location> locations;
        
        if (result.is_array()) {
            for (const auto& item : result) {
                Location loc;
                loc.uri = item.value("uri", "");
                locations.push_back(loc);
            }
        }
        
        promise->set_value(locations);
    });
    
    return promise->get_future();
}

std::future<std::vector<Diagnostic>> LSPClient::getDiagnostics() {
    auto promise = std::make_shared<std::promise<std::vector<Diagnostic>>>();
    
    // Diagnostics are typically pushed by the server
    // This returns cached diagnostics
    promise->set_value(diagnostics_);
    
    return promise->get_future();
}

std::future<std::vector<CodeAction>> LSPClient::codeAction(const std::string& uri,
                                                            int line, int column,
                                                            const std::vector<Diagnostic>& diagnostics) {
    auto promise = std::make_shared<std::promise<std::vector<CodeAction>>>();
    
    if (!initialized_) {
        promise->set_value({});
        return promise->get_future();
    }
    
    CodeActionParams params;
    params.textDocument.uri = uri;
    params.range.start.line = line;
    params.range.start.character = column;
    params.range.end = params.range.start;
    params.context.diagnostics = diagnostics;
    
    sendRequest("textDocument/codeAction", params, [promise](const json& result) {
        std::vector<CodeAction> actions;
        
        if (result.is_array()) {
            for (const auto& item : result) {
                CodeAction action;
                action.title = item.value("title", "");
                action.kind = item.value("kind", "quickfix");
                actions.push_back(action);
            }
        }
        
        promise->set_value(actions);
    });
    
    return promise->get_future();
}

std::future<std::string> LSPClient::formatDocument(const std::string& uri) {
    auto promise = std::make_shared<std::promise<std::string>>();
    
    if (!initialized_ || !serverCapabilities_.documentFormattingProvider) {
        promise->set_value("");
        return promise->get_future();
    }
    
    DocumentFormattingParams params;
    params.textDocument.uri = uri;
    
    sendRequest("textDocument/formatting", params, [promise](const json& result) {
        std::string formatted;
        
        if (result.is_array()) {
            // Apply text edits
            for (const auto& edit : result) {
                // Parse and apply edits
            }
        }
        
        promise->set_value(formatted);
    });
    
    return promise->get_future();
}

std::future<std::vector<DocumentSymbol>> LSPClient::documentSymbol(const std::string& uri) {
    auto promise = std::make_shared<std::promise<std::vector<DocumentSymbol>>>();
    
    if (!initialized_) {
        promise->set_value({});
        return promise->get_future();
    }
    
    DocumentSymbolParams params;
    params.textDocument.uri = uri;
    
    sendRequest("textDocument/documentSymbol", params, [promise](const json& result) {
        std::vector<DocumentSymbol> symbols;
        
        if (result.is_array()) {
            for (const auto& item : result) {
                DocumentSymbol sym;
                sym.name = item.value("name", "");
                sym.kind = static_cast<SymbolKind>(item.value("kind", 0));
                sym.detail = item.value("detail", "");
                symbols.push_back(sym);
            }
        }
        
        promise->set_value(symbols);
    });
    
    return promise->get_future();
}

void LSPClient::sendRequest(const std::string& method, const json& params,
                             ResponseCallback callback) {
    if (!connected_) return;
    
    int id = requestId_++;
    
    json request = {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"method", method},
        {"params", params}
    };
    
    pendingRequests_[id] = std::move(callback);
    
    // In production, send to server process
    std::string requestStr = request.dump();
    sendMessage(requestStr);
}

void LSPClient::sendNotification(const std::string& method, const json& params) {
    if (!connected_) return;
    
    json notification = {
        {"jsonrpc", "2.0"},
        {"method", method},
        {"params", params}
    };
    
    std::string notifStr = notification.dump();
    sendMessage(notifStr);
}

void LSPClient::sendMessage(const std::string& message) {
    // In production, write to server process stdin
    // Format: Content-Length: <length>\r\n\r\n<message>
    
    std::ostringstream header;
    header << "Content-Length: " << message.length() << "\r\n\r\n";
    
    std::string fullMessage = header.str() + message;
    
    // Write to server
    if (serverProcess_) {
        // serverProcess_->write(fullMessage);
    }
}

void LSPClient::onMessageReceived(const std::string& message) {
    // Parse LSP message
    // In production, parse Content-Length header first
    
    try {
        json msg = json::parse(message);
        
        if (msg.contains("id")) {
            // Response
            int id = msg["id"];
            auto it = pendingRequests_.find(id);
            if (it != pendingRequests_.end()) {
                it->second(msg.value("result", json{}));
                pendingRequests_.erase(it);
            }
        } else if (msg.contains("method")) {
            // Notification or request from server
            std::string method = msg["method"];
            
            if (method == "textDocument/publishDiagnostics") {
                // Handle diagnostics
                auto params = msg["params"];
                // Parse and store diagnostics
            }
        }
    } catch (...) {
        // Parse error
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
    // In production, get actual working directory
    return "/workspace";
}

bool LSPClient::isConnected() const {
    return connected_;
}

bool LSPClient::isInitialized() const {
    return initialized_;
}

const ServerCapabilities& LSPClient::getServerCapabilities() const {
    return serverCapabilities_;
}

} // namespace NexusForge::Editor
