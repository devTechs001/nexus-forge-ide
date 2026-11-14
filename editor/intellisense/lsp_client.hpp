// editor/intellisense/lsp_client.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <memory>
#include <future>
#include <cstdint>

namespace NexusForge::Editor {

// LSP Position
struct Position {
    size_t line = 0;
    size_t character = 0;

    bool operator==(const Position& other) const {
        return line == other.line && character == other.character;
    }
    bool operator!=(const Position& other) const { return !(*this == other); }
    bool operator<(const Position& other) const {
        if (line != other.line) return line < other.line;
        return character < other.character;
    }
    bool operator<=(const Position& other) const { return !(other < *this); }
    bool operator>(const Position& other) const { return other < *this; }
    bool operator>=(const Position& other) const { return !(*this < other); }
};

// LSP Range
struct Range {
    Position start;
    Position end;

    bool contains(const Position& pos) const {
        return pos >= start && pos < end;
    }
};

// LSP Location
struct Location {
    std::string uri;
    Range range;
};

// Diagnostic severity
enum class DiagnosticSeverity {
    Error = 1,
    Warning = 2,
    Information = 3,
    Hint = 4
};

// Diagnostic
struct Diagnostic {
    Range range;
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    std::string code;
    std::string source;
    std::string message;
    std::vector<std::string> relatedInformation;
};

// Completion item kind
enum class CompletionItemKind {
    Text = 1,
    Method = 2,
    Function = 3,
    Constructor = 4,
    Field = 5,
    Variable = 6,
    Class = 7,
    Interface = 8,
    Module = 9,
    Property = 10,
    Unit = 11,
    Value = 12,
    Enum = 13,
    Keyword = 14,
    Snippet = 15,
    Color = 16,
    File = 17,
    Reference = 18,
    Folder = 19,
    EnumMember = 20,
    Constant = 21,
    Struct = 22,
    Event = 23,
    Operator = 24,
    TypeParameter = 25
};

using CompletionKind = CompletionItemKind;

// Completion item (unified across LSP and local providers)
struct CompletionItem {
    std::string label;
    CompletionItemKind kind = CompletionItemKind::Text;
    std::string detail;
    std::string documentation;
    std::string sortText;
    std::string filterText;
    std::string insertText;
    std::string textEdit;
    int insertTextFormat = 1;  // 1 = plain, 2 = snippet
    std::string command;
    std::vector<std::string> additionalTextEdits;
    std::vector<std::string> commitCharacters;
    bool deprecated = false;
    bool preselect = false;
    std::string language;
    std::string scope;
    void* data = nullptr;
};

struct CompletionList {
    bool isIncomplete = false;
    std::vector<CompletionItem> items;
};

// Hover contents
struct Hover {
    std::string contents;
    Range range;
};

// Signature help
struct SignatureInformation {
    std::string label;
    std::string documentation;
    std::vector<std::string> parameters;
    int activeParameter = 0;
};

struct SignatureHelp {
    std::vector<SignatureInformation> signatures;
    int activeSignature = 0;
    int activeParameter = 0;
};

// Symbol kind
enum class SymbolKind {
    File = 1,
    Module = 2,
    Namespace = 3,
    Package = 4,
    Class = 5,
    Method = 6,
    Property = 7,
    Field = 8,
    Constructor = 9,
    Enum = 10,
    Interface = 11,
    Function = 12,
    Variable = 13,
    Constant = 14,
    String = 15,
    Number = 16,
    Boolean = 17,
    Array = 18,
    Object = 19,
    Key = 20,
    Null = 21,
    EnumMember = 22,
    Struct = 23,
    Event = 24,
    Operator = 25,
    TypeParameter = 26
};

// Symbol information
struct SymbolInformation {
    std::string name;
    SymbolKind kind = SymbolKind::Variable;
    Location location;
    std::string containerName;
};

// Document symbol
struct DocumentSymbol {
    std::string name;
    std::string detail;
    SymbolKind kind = SymbolKind::Variable;
    Range range;
    Range selectionRange;
    std::vector<DocumentSymbol> children;
};

// Code action
struct CodeAction {
    std::string title;
    std::string kind;
    std::string diagnostics;
    bool isPreferred = false;
    std::string edit;
    std::string command;
};

// Code lens
struct CodeLens {
    Range range;
    std::string command;
    std::string data;
};

// Document link
struct DocumentLink {
    Range range;
    std::string target;
    std::string tooltip;
};

// Formatting options
struct FormattingOptions {
    int tabSize = 4;
    bool insertSpaces = true;
    bool trimTrailingWhitespace = false;
    bool insertFinalNewline = false;
    bool trimFinalNewlines = false;
};

// Server capabilities
struct ServerCapabilities {
    bool completionProvider = false;
    bool hoverProvider = false;
    bool signatureHelpProvider = false;
    bool definitionProvider = false;
    bool typeDefinitionProvider = false;
    bool implementationProvider = false;
    bool referencesProvider = false;
    bool documentSymbolProvider = false;
    bool workspaceSymbolProvider = false;
    bool codeActionProvider = false;
    bool codeLensProvider = false;
    bool documentLinkProvider = false;
    bool documentFormattingProvider = false;
    bool documentRangeFormattingProvider = false;
    bool renameProvider = false;
    bool foldingRangeProvider = false;
};

// LSP Client
class LSPClient {
public:
    using ServerCapabilities = NexusForge::Editor::ServerCapabilities;
    using ResponseCallback = std::function<void(const std::string&)>;

    LSPClient();
    ~LSPClient();

    // Connection
    bool connect(const std::string& serverPath, const std::vector<std::string>& args = {});
    bool connectTcp(const std::string& host, int port);
    void disconnect();
    bool isConnected() const;
    bool isInitialized() const;

    // Lifecycle
    bool initialize(const std::string& rootUri, const std::string& clientInfo = "NexusForge");
    void initialized();
    void shutdown();
    void exit();

    // Document management
    void openDocument(const std::string& uri, const std::string& languageId, const std::string& text);
    void closeDocument(const std::string& uri);
    void changeDocument(const std::string& uri, const std::string& text,
                        size_t version, const std::vector<Range>& ranges = {});
    void saveDocument(const std::string& uri);

    // Completions
    std::future<CompletionList> completion(const std::string& uri, int line, int column);
    std::future<std::vector<CompletionItem>> getCompletions(const std::string& uri, Position position);
    std::future<std::string> resolveCompletionItem(const CompletionItem& item);

    // Hover
    std::future<Hover> hover(const std::string& uri, int line, int column);
    std::future<Hover> getHover(const std::string& uri, Position position);

    // Signature help
    std::future<SignatureHelp> getSignatureHelp(const std::string& uri, Position position);

    // Go to definition
    std::future<Location> definition(const std::string& uri, int line, int column);
    std::future<std::vector<Location>> getDefinition(const std::string& uri, Position position);

    // Go to type definition
    std::future<std::vector<Location>> getTypeDefinition(const std::string& uri, Position position);

    // Go to implementation
    std::future<std::vector<Location>> getImplementation(const std::string& uri, Position position);

    // References
    std::future<std::vector<Location>> references(const std::string& uri, int line, int column,
                                                  bool includeDeclaration = true);
    std::future<std::vector<Location>> getReferences(const std::string& uri, Position position,
                                                      bool includeDeclaration = true);

    // Document symbols
    std::future<std::vector<DocumentSymbol>> documentSymbol(const std::string& uri);
    std::future<std::vector<DocumentSymbol>> getDocumentSymbols(const std::string& uri);

    // Workspace symbols
    std::future<std::vector<SymbolInformation>> getWorkspaceSymbols(const std::string& query);

    // Code actions
    std::future<std::vector<CodeAction>> codeAction(const std::string& uri, int line, int column,
                                                     const std::vector<Diagnostic>& diagnostics);
    std::future<std::vector<CodeAction>> getCodeActions(const std::string& uri, Range range,
                                                         const std::vector<Diagnostic>& diagnostics);

    // Code lenses
    std::future<std::vector<CodeLens>> getCodeLenses(const std::string& uri);

    // Document links
    std::future<std::vector<DocumentLink>> getDocumentLinks(const std::string& uri);

    // Formatting
    std::future<std::string> formatDocument(const std::string& uri);
    std::future<std::string> formatDocument(const std::string& uri, const FormattingOptions& options);
    std::future<std::string> formatRange(const std::string& uri, Range range,
                                          const FormattingOptions& options);
    std::future<std::string> formatOnType(const std::string& uri, Position position,
                                           char triggerCharacter, const FormattingOptions& options);

    // Rename
    std::future<std::string> rename(const std::string& uri, Position position,
                                     const std::string& newName);

    // Diagnostics
    std::future<std::vector<Diagnostic>> getDiagnostics();
    using DiagnosticCallback = std::function<void(const std::string&, const std::vector<Diagnostic>&)>;
    void setDiagnosticCallback(DiagnosticCallback callback);

    // Incoming protocol message handler
    void onMessageReceived(const std::string& message);

    // Capabilities
    const ServerCapabilities& getCapabilities() const { return capabilities_; }
    const ServerCapabilities& getServerCapabilities() const { return capabilities_; }

    // Events
    std::function<void()> onInitialized;
    std::function<void(const std::string&)> onPublishDiagnostics;
    std::function<void(const std::string&)> onShowMessage;

private:
    void* serverProcess_ = nullptr;
    std::string serverCommand_;
    std::vector<std::string> serverArgs_;
    int requestId_ = 1;
    bool connected_ = false;
    bool initialized_ = false;
    ServerCapabilities capabilities_;
    DiagnosticCallback diagnosticCallback_;
    std::unordered_set<std::string> openDocuments_;
    std::vector<Diagnostic> diagnostics_;
    std::unordered_map<int, ResponseCallback> pendingRequests_;

    // Internal
    void sendInitialize();
    void sendShutdown();
    void sendRequest(const std::string& method, const std::string& params, ResponseCallback callback);
    void sendNotification(const std::string& method, const std::string& params);
    void sendMessage(const std::string& message);
    void sendMessage(const std::string& method, const std::string& params);
    std::string sendRequest(const std::string& method, const std::string& params);
    void processMessages();
    void handleMessage(const std::string& message);
    uint32_t getCurrentProcessId() const;
    std::string getWorkingDirectory() const;

    // Process handling
#ifdef _WIN32
    void* processHandle_ = nullptr;
    void* readPipe_ = nullptr;
    void* writePipe_ = nullptr;
#else
    int processPid_ = -1;
    int readFd_ = -1;
    int writeFd_ = -1;
#endif
};

} // namespace NexusForge::Editor
