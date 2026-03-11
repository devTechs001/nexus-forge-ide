// networking/http_client.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <future>
#include <optional>

namespace NexusForge::Networking {

// HTTP Methods
enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS
};

// HTTP Status codes
enum class HttpStatusCode {
    Continue = 100,
    SwitchingProtocols = 101,
    OK = 200,
    Created = 201,
    Accepted = 202,
    NoContent = 204,
    ResetContent = 205,
    PartialContent = 206,
    MultipleChoices = 300,
    MovedPermanently = 301,
    Found = 302,
    SeeOther = 303,
    NotModified = 304,
    TemporaryRedirect = 307,
    PermanentRedirect = 308,
    BadRequest = 400,
    Unauthorized = 401,
    PaymentRequired = 402,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    NotAcceptable = 406,
    ProxyAuthenticationRequired = 407,
    RequestTimeout = 408,
    Conflict = 409,
    Gone = 410,
    LengthRequired = 411,
    PreconditionFailed = 412,
    PayloadTooLarge = 413,
    URITooLong = 414,
    UnsupportedMediaType = 415,
    RangeNotSatisfiable = 416,
    ExpectationFailed = 417,
    TooManyRequests = 429,
    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,
    HTTPVersionNotSupported = 505
};

// HTTP Headers
using HttpHeaders = std::unordered_map<std::string, std::string>;

// HTTP Request
struct HttpRequest {
    HttpMethod method = HttpMethod::GET;
    std::string url;
    std::string path;
    std::string host;
    int port = 443;
    bool useHttps = true;

    HttpHeaders headers;
    std::string body;
    std::string contentType;

    int timeoutMs = 30000;
    bool followRedirects = true;
    int maxRedirects = 5;
    bool verifySSL = true;
    std::string caBundlePath;

    // Authentication
    std::string username;
    std::string password;
    std::string bearerToken;
    std::string apiKey;
    std::string apiKeyHeader = "X-API-Key";

    // Proxy
    std::string proxyHost;
    int proxyPort = 0;
    std::string proxyUsername;
    std::string proxyPassword;

    // Cookies
    std::unordered_map<std::string, std::string> cookies;

    // Query parameters
    std::unordered_map<std::string, std::string> queryParams;

    void setHeader(const std::string& key, const std::string& value);
    void setBasicAuth(const std::string& user, const std::string& pass);
    void setBearerToken(const std::string& token);
    void setJsonBody(const std::string& json);
    void setFormBody(const std::unordered_map<std::string, std::string>& form);
    void addQueryParam(const std::string& key, const std::string& value);
};

// HTTP Response
struct HttpResponse {
    HttpStatusCode statusCode = HttpStatusCode::OK;
    int statusCodeInt = 0;
    std::string statusMessage;
    std::string httpVersion;

    HttpHeaders headers;
    std::string body;
    std::vector<uint8_t> rawBody;

    bool success = false;
    std::string errorMessage;
    std::string errorCode;

    double responseTime = 0;  // milliseconds
    std::string finalUrl;
    int redirectCount = 0;

    // Cookies from response
    std::unordered_map<std::string, std::string> cookies;

    // Helpers
    std::string getHeader(const std::string& key) const;
    bool hasHeader(const std::string& key) const;
    std::string getContentType() const;
    size_t getContentLength() const;
    bool isJson() const;
    bool isHtml() const;
    bool isText() const;
    bool isRedirect() const;
    std::string getRedirectLocation() const;

    // Parse helpers
    std::unordered_map<std::string, std::string> parseJson() const;
    std::string parseText() const { return body; }
};

// HTTP Client
class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    // Configuration
    void setDefaultTimeout(int timeoutMs) { defaultTimeout_ = timeoutMs; }
    void setDefaultUserAgent(const std::string& userAgent) { defaultUserAgent_ = userAgent; }
    void setVerifySSL(bool verify) { verifySSL_ = verify; }
    void setProxy(const std::string& host, int port,
                  const std::string& username = "", const std::string& password = "");
    void setCookieJar(std::unordered_map<std::string, std::string>* cookies) { cookieJar_ = cookies; }

    // Synchronous requests
    HttpResponse get(const std::string& url, const HttpHeaders& headers = {});
    HttpResponse post(const std::string& url, const std::string& body = "",
                      const std::string& contentType = "application/json",
                      const HttpHeaders& headers = {});
    HttpResponse put(const std::string& url, const std::string& body = "",
                     const std::string& contentType = "application/json",
                     const HttpHeaders& headers = {});
    HttpResponse patch(const std::string& url, const std::string& body = "",
                       const std::string& contentType = "application/json",
                       const HttpHeaders& headers = {});
    HttpResponse del(const std::string& url, const HttpHeaders& headers = {});
    HttpResponse head(const std::string& url, const HttpHeaders& headers = {});
    HttpResponse options(const std::string& url, const HttpHeaders& headers = {});

    // Request with full configuration
    HttpResponse send(const HttpRequest& request);

    // Asynchronous requests
    std::future<HttpResponse> getAsync(const std::string& url,
                                        const HttpHeaders& headers = {});
    std::future<HttpResponse> postAsync(const std::string& url, const std::string& body = "",
                                         const std::string& contentType = "application/json",
                                         const HttpHeaders& headers = {});
    std::future<HttpResponse> sendAsync(const HttpRequest& request);

    // Streaming download
    using DownloadProgressCallback = std::function<void(size_t downloaded, size_t total)>;
    bool downloadFile(const std::string& url, const std::string& destPath,
                      DownloadProgressCallback progress = nullptr);

    // Streaming upload
    using UploadProgressCallback = std::function<void(size_t uploaded, size_t total)>;
    HttpResponse uploadFile(const std::string& url, const std::string& filePath,
                            const std::string& fieldName = "file",
                            UploadProgressCallback progress = nullptr);

    // Multipart form data
    struct MultipartPart {
        std::string name;
        std::string filename;
        std::string contentType;
        std::string content;  // For text data
        std::vector<uint8_t> binaryContent;  // For file data
        bool isFile = false;
    };
    HttpResponse postMultipart(const std::string& url,
                                const std::vector<MultipartPart>& parts,
                                const HttpHeaders& headers = {});

    // WebSocket support
    struct WebSocketOptions {
        std::vector<std::string> protocols;
        HttpHeaders headers;
        int pingIntervalMs = 30000;
    };

    // Convenience methods
    bool isOnline() const;
    std::string getUserAgent() const { return defaultUserAgent_; }

private:
    int defaultTimeout_ = 30000;
    std::string defaultUserAgent_ = "NexusForge/1.0";
    bool verifySSL_ = true;
    std::string proxyHost_;
    int proxyPort_ = 0;
    std::string proxyUsername_;
    std::string proxyPassword_;
    std::unordered_map<std::string, std::string>* cookieJar_ = nullptr;

    // Internal
    HttpResponse executeRequest(const HttpRequest& request);
    std::string buildUrl(const HttpRequest& request);
    HttpHeaders buildHeaders(const HttpRequest& request);
    void applyCookies(HttpRequest& request);
    void storeCookies(const HttpResponse& response);
    HttpResponse handleRedirect(const HttpRequest& request, const HttpResponse& response,
                                 int redirectCount);
};

// WebSocket client
class WebSocketClient {
public:
    WebSocketClient();
    ~WebSocketClient();

    // Connection
    bool connect(const std::string& url, const WebSocketOptions& options = {});
    void disconnect(int closeCode = 1000, const std::string& reason = "");
    bool isConnected() const { return connected_; }

    // Messages
    void send(const std::string& message);
    void sendBinary(const std::vector<uint8_t>& data);

    // Events
    using MessageCallback = std::function<void(const std::string&)>;
    using BinaryCallback = std::function<void(const std::vector<uint8_t>&)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    using CloseCallback = std::function<void(int, const std::string&)>;
    using ConnectCallback = std::function<void()>;

    void onMessage(MessageCallback callback) { messageCallback_ = callback; }
    void onBinary(BinaryCallback callback) { binaryCallback_ = callback; }
    void onError(ErrorCallback callback) { errorCallback_ = callback; }
    void onClose(CloseCallback callback) { closeCallback_ = callback; }
    void onConnect(ConnectCallback callback) { connectCallback_ = callback; }

    // Ping/Pong
    void ping();
    void setPingInterval(int intervalMs) { pingInterval_ = intervalMs; }

private:
    bool connected_ = false;
    std::string url_;
    WebSocketOptions options_;

    MessageCallback messageCallback_;
    BinaryCallback binaryCallback_;
    ErrorCallback errorCallback_;
    CloseCallback closeCallback_;
    ConnectCallback connectCallback_;

    int pingInterval_ = 30000;
    std::string subprotocol_;

    // Internal implementation
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// URL utilities
struct URL {
    std::string protocol;
    std::string host;
    int port;
    std::string path;
    std::string query;
    std::string fragment;
    std::string username;
    std::string password;

    static URL parse(const std::string& urlString);
    std::string toString() const;
    std::unordered_map<std::string, std::string> parseQuery() const;
    void setQuery(const std::unordered_map<std::string, std::string>& params);
};

// JSON helpers (simplified)
namespace Json {
    std::string stringify(const std::unordered_map<std::string, std::string>& obj);
    std::string stringify(const std::vector<std::string>& arr);
    std::unordered_map<std::string, std::string> parseObject(const std::string& json);
    std::vector<std::string> parseArray(const std::string& json);
}

} // namespace NexusForge::Networking
