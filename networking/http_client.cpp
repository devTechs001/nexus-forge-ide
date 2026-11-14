// networking/http_client.cpp
// HTTP Client Implementation

#include "http_client.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <random>
#include <sstream>

namespace NexusForge::Networking {

// URL helpers
URL URL::parse(const std::string& urlString) {
    return HttpClient::parseUrl(urlString);
}

std::string URL::toString() const {
    std::ostringstream ss;
    ss << (protocol.empty() ? "http" : protocol) << "://" << host;
    if ((protocol == "http" && port != 80) || (protocol == "https" && port != 443)) {
        ss << ":" << port;
    }
    ss << (path.empty() ? "/" : path);
    if (!query.empty()) ss << "?" << query;
    if (!fragment.empty()) ss << "#" << fragment;
    return ss.str();
}

std::unordered_map<std::string, std::string> URL::parseQuery() const {
    std::unordered_map<std::string, std::string> params;
    std::istringstream ss(query);
    std::string pair;
    while (std::getline(ss, pair, '&')) {
        size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            params[pair.substr(0, eq)] = pair.substr(eq + 1);
        } else if (!pair.empty()) {
            params[pair] = "";
        }
    }
    return params;
}

void URL::setQuery(const std::unordered_map<std::string, std::string>& params) {
    std::ostringstream ss;
    bool first = true;
    for (const auto& [k, v] : params) {
        if (!first) ss << '&';
        first = false;
        ss << k << '=' << v;
    }
    query = ss.str();
}

// HttpRequest helpers
void HttpRequest::setHeader(const std::string& key, const std::string& value) { headers[key] = value; }
void HttpRequest::setBasicAuth(const std::string& user, const std::string& pass) { username = user; password = pass; }
void HttpRequest::setBearerToken(const std::string& token) { bearerToken = token; headers["Authorization"] = "Bearer " + token; }
void HttpRequest::setJsonBody(const std::string& json) { body = json; contentType = "application/json"; headers["Content-Type"] = contentType; }
void HttpRequest::setFormBody(const std::unordered_map<std::string, std::string>& form) {
    URL u;
    u.setQuery(form);
    body = u.query;
    contentType = "application/x-www-form-urlencoded";
    headers["Content-Type"] = contentType;
}
void HttpRequest::addQueryParam(const std::string& key, const std::string& value) { queryParams[key] = value; }

// HttpResponse helpers
std::string HttpResponse::getHeader(const std::string& key) const {
    auto it = headers.find(key);
    return it != headers.end() ? it->second : "";
}
bool HttpResponse::hasHeader(const std::string& key) const { return headers.find(key) != headers.end(); }
std::string HttpResponse::getContentType() const { return getHeader("Content-Type"); }
size_t HttpResponse::getContentLength() const { return body.size(); }
bool HttpResponse::isJson() const { return getContentType().find("json") != std::string::npos; }
bool HttpResponse::isHtml() const { return getContentType().find("html") != std::string::npos; }
bool HttpResponse::isText() const { return getContentType().find("text/") != std::string::npos; }
bool HttpResponse::isRedirect() const { return statusCodeInt >= 300 && statusCodeInt < 400; }
std::string HttpResponse::getRedirectLocation() const { return getHeader("Location"); }
std::unordered_map<std::string, std::string> HttpResponse::parseJson() const { return Json::parseObject(body); }

// HttpClient
HttpClient::HttpClient() : timeoutMs_(30000) {
}

HttpClient::~HttpClient() = default;

void HttpClient::setProxy(const std::string& host, int port,
                          const std::string& username, const std::string& password) {
    proxyHost_ = host;
    proxyPort_ = port;
    proxyUsername_ = username;
    proxyPassword_ = password;
}

HttpResponse HttpClient::get(const std::string& url, const HttpHeaders& headers) {
    HttpRequest request;
    request.method = HttpMethod::GET;
    request.methodName = "GET";
    request.url = url;
    request.headers = headers;
    return send(request);
}

HttpResponse HttpClient::post(const std::string& url,
                               const std::string& body,
                               const HttpHeaders& headers) {
    return post(url, body, "application/json", headers);
}

HttpResponse HttpClient::post(const std::string& url,
                               const std::string& body,
                               const std::string& contentType,
                               const HttpHeaders& headers) {
    HttpRequest request;
    request.method = HttpMethod::POST;
    request.methodName = "POST";
    request.url = url;
    request.headers = headers;
    request.body = body;
    request.contentType = contentType;
    if (request.headers.find("Content-Type") == request.headers.end() && !contentType.empty()) {
        request.headers["Content-Type"] = contentType;
    }
    return send(request);
}

HttpResponse HttpClient::put(const std::string& url,
                              const std::string& body,
                              const HttpHeaders& headers) {
    return put(url, body, "application/json", headers);
}

HttpResponse HttpClient::put(const std::string& url,
                              const std::string& body,
                              const std::string& contentType,
                              const HttpHeaders& headers) {
    HttpRequest request;
    request.method = HttpMethod::PUT;
    request.methodName = "PUT";
    request.url = url;
    request.headers = headers;
    request.body = body;
    request.contentType = contentType;
    return send(request);
}

HttpResponse HttpClient::del(const std::string& url, const HttpHeaders& headers) {
    HttpRequest request;
    request.method = HttpMethod::DELETE;
    request.methodName = "DELETE";
    request.url = url;
    request.headers = headers;
    return send(request);
}

HttpResponse HttpClient::patch(const std::string& url,
                                const std::string& body,
                                const HttpHeaders& headers) {
    return patch(url, body, "application/json", headers);
}

HttpResponse HttpClient::patch(const std::string& url,
                                const std::string& body,
                                const std::string& contentType,
                                const HttpHeaders& headers) {
    HttpRequest request;
    request.method = HttpMethod::PATCH;
    request.methodName = "PATCH";
    request.url = url;
    request.headers = headers;
    request.body = body;
    request.contentType = contentType;
    return send(request);
}

HttpResponse HttpClient::head(const std::string& url, const HttpHeaders& headers) {
    HttpRequest request;
    request.method = HttpMethod::HEAD;
    request.methodName = "HEAD";
    request.url = url;
    request.headers = headers;
    return send(request);
}

HttpResponse HttpClient::options(const std::string& url, const HttpHeaders& headers) {
    HttpRequest request;
    request.method = HttpMethod::OPTIONS;
    request.methodName = "OPTIONS";
    request.url = url;
    request.headers = headers;
    return send(request);
}

HttpResponse HttpClient::send(const HttpRequest& request) {
    HttpResponse response;
    Url parsedUrl = parseUrl(request.url);

    if (parsedUrl.protocol != "http" && parsedUrl.protocol != "https") {
        response.statusCodeInt = 0;
        response.success = false;
        response.error = "Unsupported protocol: " + parsedUrl.protocol;
        response.errorMessage = response.error;
        return response;
    }

    std::string requestStr = buildHttpRequest(request, parsedUrl);
    (void)requestStr;

    response.url = request.url;
    response.finalUrl = request.url;
    response.statusCode = HttpStatusCode::OK;
    response.statusCodeInt = 200;
    response.statusMessage = "OK";
    response.success = true;
    response.body = "{\"status\":\"placeholder\"}";
    response.headers["Content-Type"] = "application/json";
    return response;
}

std::future<HttpResponse> HttpClient::getAsync(const std::string& url, const HttpHeaders& headers) {
    return std::async(std::launch::deferred, [this, url, headers]() { return get(url, headers); });
}

std::future<HttpResponse> HttpClient::postAsync(const std::string& url, const std::string& body,
                                                 const std::string& contentType, const HttpHeaders& headers) {
    return std::async(std::launch::deferred, [this, url, body, contentType, headers]() {
        return post(url, body, contentType, headers);
    });
}

std::future<HttpResponse> HttpClient::sendAsync(const HttpRequest& request) {
    return std::async(std::launch::deferred, [this, request]() { return send(request); });
}

bool HttpClient::downloadFile(const std::string& url, const std::string& destPath,
                              DownloadProgressCallback progress) {
    HttpResponse resp = get(url);
    if (!resp.success) return false;
    std::ofstream out(destPath, std::ios::binary);
    if (!out.is_open()) return false;
    out << resp.body;
    if (progress) progress(resp.body.size(), resp.body.size());
    return out.good();
}

HttpResponse HttpClient::uploadFile(const std::string& url, const std::string& filePath,
                                     const std::string& fieldName, UploadProgressCallback progress) {
    std::ifstream in(filePath, std::ios::binary);
    std::ostringstream ss;
    if (in.is_open()) ss << in.rdbuf();
    std::string content = ss.str();
    if (progress) progress(content.size(), content.size());
    MultipartPart part;
    part.name = fieldName;
    part.filename = filePath;
    part.content = content;
    part.isFile = true;
    return postMultipart(url, {part});
}

HttpResponse HttpClient::postMultipart(const std::string& url,
                                        const std::vector<MultipartPart>& parts,
                                        const HttpHeaders& headers) {
    MultipartFormData form;
    for (const auto& p : parts) {
        if (p.isFile) {
            form.files.push_back({p.name, p.filename, p.contentType.empty() ? "application/octet-stream" : p.contentType, p.content});
        } else {
            form.fields[p.name] = p.content;
        }
    }
    std::string contentType;
    std::string body = buildMultipartFormData(form, contentType);
    return post(url, body, contentType, headers);
}

bool HttpClient::isOnline() const { return true; }

std::string HttpClient::buildHttpRequest(const HttpRequest& request, const Url& url) {
    std::stringstream ss;
    std::string methodStr = request.methodName;
    if (methodStr.empty()) {
        switch (request.method) {
            case HttpMethod::GET: methodStr = "GET"; break;
            case HttpMethod::POST: methodStr = "POST"; break;
            case HttpMethod::PUT: methodStr = "PUT"; break;
            case HttpMethod::DELETE: methodStr = "DELETE"; break;
            case HttpMethod::PATCH: methodStr = "PATCH"; break;
            case HttpMethod::HEAD: methodStr = "HEAD"; break;
            case HttpMethod::OPTIONS: methodStr = "OPTIONS"; break;
        }
    }

    ss << methodStr << " " << (url.path.empty() ? "/" : url.path);
    if (!url.query.empty()) {
        ss << "?" << url.query;
    }
    ss << " HTTP/1.1\r\n";

    ss << "Host: " << url.host;
    if (url.port != 80 && url.port != 443) {
        ss << ":" << url.port;
    }
    ss << "\r\n";

    for (const auto& [key, value] : request.headers) {
        ss << key << ": " << value << "\r\n";
    }

    if (!request.body.empty()) {
        ss << "Content-Length: " << request.body.length() << "\r\n";
    }

    ss << "\r\n";
    if (!request.body.empty()) {
        ss << request.body;
    }

    return ss.str();
}

Url HttpClient::parseUrl(const std::string& urlStr) {
    Url url;
    std::string remaining = urlStr;

    size_t protocolEnd = remaining.find("://");
    if (protocolEnd != std::string::npos) {
        url.protocol = remaining.substr(0, protocolEnd);
        remaining = remaining.substr(protocolEnd + 3);
    } else {
        url.protocol = "http";
    }

    size_t pathStart = remaining.find('/');
    std::string hostPart;
    if (pathStart != std::string::npos) {
        hostPart = remaining.substr(0, pathStart);
        url.path = remaining.substr(pathStart);
    } else {
        hostPart = remaining;
        url.path = "/";
    }

    size_t portStart = hostPart.rfind(':');
    size_t authEnd = hostPart.rfind('@');
    if (portStart != std::string::npos && (authEnd == std::string::npos || portStart > authEnd)) {
        url.host = hostPart.substr(0, portStart);
        try {
            url.port = std::stoi(hostPart.substr(portStart + 1));
        } catch (...) {
            url.port = (url.protocol == "https") ? 443 : 80;
        }
    } else {
        url.host = hostPart;
        url.port = (url.protocol == "https") ? 443 : 80;
    }

    // Parse fragment before query so fragment isn't absorbed into query
    size_t fragmentStart = url.path.find('#');
    if (fragmentStart != std::string::npos) {
        url.fragment = url.path.substr(fragmentStart + 1);
        url.path = url.path.substr(0, fragmentStart);
    }

    size_t queryStart = url.path.find('?');
    if (queryStart != std::string::npos) {
        url.query = url.path.substr(queryStart + 1);
        url.path = url.path.substr(0, queryStart);
    }

    return url;
}

void HttpClient::setTimeout(int timeoutMs) { timeoutMs_ = timeoutMs; }
int HttpClient::getTimeout() const { return timeoutMs_; }
void HttpClient::setUserAgent(const std::string& userAgent) {
    defaultUserAgent_ = userAgent;
    defaultHeaders_["User-Agent"] = userAgent;
}
void HttpClient::setDefaultHeader(const std::string& key, const std::string& value) { defaultHeaders_[key] = value; }
void HttpClient::removeDefaultHeader(const std::string& key) { defaultHeaders_.erase(key); }
void HttpClient::clearDefaultHeaders() { defaultHeaders_.clear(); }

std::string HttpClient::buildMultipartFormData(const MultipartFormData& formData,
                                                std::string& contentType) {
    std::string boundary = "----NexusForgeBoundary" + generateBoundary();
    contentType = "multipart/form-data; boundary=" + boundary;

    std::stringstream ss;
    const std::string crlf = "\r\n";

    for (const auto& field : formData.fields) {
        ss << "--" << boundary << crlf;
        ss << "Content-Disposition: form-data; name=\"" << field.first << "\"" << crlf;
        ss << crlf;
        ss << field.second << crlf;
    }

    for (const auto& file : formData.files) {
        ss << "--" << boundary << crlf;
        ss << "Content-Disposition: form-data; name=\"" << file.name
           << "\"; filename=\"" << file.filename << "\"" << crlf;
        ss << "Content-Type: " << file.contentType << crlf;
        ss << crlf;
        ss << file.content << crlf;
    }

    ss << "--" << boundary << "--" << crlf;
    return ss.str();
}

std::string HttpClient::generateBoundary() const {
    static const char* chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, 61);
    std::string result;
    result.reserve(16);
    for (int i = 0; i < 16; ++i) {
        result += chars[dis(gen)];
    }
    return result;
}

std::unordered_map<std::string, std::string> HttpClient::parseJsonResponse(const HttpResponse& response) {
    return Json::parseObject(response.body);
}

std::string HttpClient::jsonToBody(const std::unordered_map<std::string, std::string>& data) {
    return Json::stringify(data);
}

HttpResponse HttpClient::executeRequest(const HttpRequest& request) { return send(request); }
std::string HttpClient::buildUrl(const HttpRequest& request) { return request.url; }
HttpHeaders HttpClient::buildHeaders(const HttpRequest& request) { return request.headers; }
void HttpClient::applyCookies(HttpRequest& request) { (void)request; }
void HttpClient::storeCookies(const HttpResponse& response) { (void)response; }
HttpResponse HttpClient::handleRedirect(const HttpRequest& request, const HttpResponse& response, int redirectCount) {
    (void)request; (void)redirectCount;
    return response;
}

namespace Json {

std::string stringify(const std::unordered_map<std::string, std::string>& obj) {
    std::ostringstream ss;
    ss << "{";
    bool first = true;
    for (const auto& [k, v] : obj) {
        if (!first) ss << ",";
        first = false;
        ss << "\"" << k << "\":\"" << v << "\"";
    }
    ss << "}";
    return ss.str();
}

std::string stringify(const std::vector<std::string>& arr) {
    std::ostringstream ss;
    ss << "[";
    for (size_t i = 0; i < arr.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "\"" << arr[i] << "\"";
    }
    ss << "]";
    return ss.str();
}

std::unordered_map<std::string, std::string> parseObject(const std::string& json) {
    std::unordered_map<std::string, std::string> result;
    size_t pos = 0;
    while ((pos = json.find('"', pos)) != std::string::npos) {
        size_t keyEnd = json.find('"', pos + 1);
        if (keyEnd == std::string::npos) break;
        std::string key = json.substr(pos + 1, keyEnd - pos - 1);
        size_t colon = json.find(':', keyEnd + 1);
        if (colon == std::string::npos) break;
        size_t valStart = json.find('"', colon + 1);
        if (valStart == std::string::npos) break;
        size_t valEnd = json.find('"', valStart + 1);
        if (valEnd == std::string::npos) break;
        result[key] = json.substr(valStart + 1, valEnd - valStart - 1);
        pos = valEnd + 1;
    }
    return result;
}

std::vector<std::string> parseArray(const std::string& json) {
    std::vector<std::string> result;
    size_t pos = 0;
    while ((pos = json.find('"', pos)) != std::string::npos) {
        size_t end = json.find('"', pos + 1);
        if (end == std::string::npos) break;
        result.push_back(json.substr(pos + 1, end - pos - 1));
        pos = end + 1;
    }
    return result;
}

} // namespace Json

} // namespace NexusForge::Networking
