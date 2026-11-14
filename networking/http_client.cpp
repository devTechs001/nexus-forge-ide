// networking/http_client.cpp
// HTTP Client Implementation

#include "http_client.hpp"
#include <sstream>
#include <algorithm>
#include <cstring>

namespace NexusForge::Networking {

HttpClient::HttpClient() : timeoutMs_(30000) {
}

HttpClient::~HttpClient() = default;

HttpResponse HttpClient::get(const std::string& url, const HttpHeaders& headers) {
    HttpRequest request;
    request.method = "GET";
    request.url = url;
    request.headers = headers;
    
    return send(request);
}

HttpResponse HttpClient::post(const std::string& url, 
                               const std::string& body,
                               const HttpHeaders& headers) {
    HttpRequest request;
    request.method = "POST";
    request.url = url;
    request.headers = headers;
    request.body = body;
    
    if (headers.find("Content-Type") == headers.end()) {
        request.headers["Content-Type"] = "application/json";
    }
    
    return send(request);
}

HttpResponse HttpClient::put(const std::string& url,
                              const std::string& body,
                              const HttpHeaders& headers) {
    HttpRequest request;
    request.method = "PUT";
    request.url = url;
    request.headers = headers;
    request.body = body;
    
    return send(request);
}

HttpResponse HttpClient::del(const std::string& url,
                              const HttpHeaders& headers) {
    HttpRequest request;
    request.method = "DELETE";
    request.url = url;
    request.headers = headers;
    
    return send(request);
}

HttpResponse HttpClient::patch(const std::string& url,
                                const std::string& body,
                                const HttpHeaders& headers) {
    HttpRequest request;
    request.method = "PATCH";
    request.url = url;
    request.headers = headers;
    request.body = body;
    
    return send(request);
}

HttpResponse HttpClient::send(const HttpRequest& request) {
    HttpResponse response;
    
    // Parse URL
    Url parsedUrl = parseUrl(request.url);
    
    if (parsedUrl.protocol != "http" && parsedUrl.protocol != "https") {
        response.statusCode = 0;
        response.error = "Unsupported protocol: " + parsedUrl.protocol;
        return response;
    }
    
    // Build request
    std::string requestStr = buildHttpRequest(request, parsedUrl);
    
    // In production, would use a proper HTTP library like libcurl
    // For now, return a placeholder response
    
    response.url = request.url;
    response.statusCode = 200;
    response.statusMessage = "OK";
    response.body = "{\"status\": \"placeholder\"}";
    response.headers["Content-Type"] = "application/json";
    
    return response;
}

std::string HttpClient::buildHttpRequest(const HttpRequest& request, const Url& url) {
    std::stringstream ss;
    
    // Request line
    ss << request.method << " " << url.path;
    if (!url.query.empty()) {
        ss << "?" << url.query;
    }
    ss << " HTTP/1.1\r\n";
    
    // Host header
    ss << "Host: " << url.host;
    if (url.port != 80 && url.port != 443) {
        ss << ":" << url.port;
    }
    ss << "\r\n";
    
    // Headers
    for (const auto& [key, value] : request.headers) {
        ss << key << ": " << value << "\r\n";
    }
    
    // Content-Length
    if (!request.body.empty()) {
        ss << "Content-Length: " << request.body.length() << "\r\n";
    }
    
    // End of headers
    ss << "\r\n";
    
    // Body
    if (!request.body.empty()) {
        ss << request.body;
    }
    
    return ss.str();
}

Url HttpClient::parseUrl(const std::string& urlStr) {
    Url url;
    
    // Parse protocol
    size_t protocolEnd = urlStr.find("://");
    if (protocolEnd != std::string::npos) {
        url.protocol = urlStr.substr(0, protocolEnd);
        urlStr = urlStr.substr(protocolEnd + 3);
    } else {
        url.protocol = "http";
    }
    
    // Parse host and port
    size_t pathStart = urlStr.find('/');
    std::string hostPart;
    
    if (pathStart != std::string::npos) {
        hostPart = urlStr.substr(0, pathStart);
        url.path = urlStr.substr(pathStart);
    } else {
        hostPart = urlStr;
        url.path = "/";
    }
    
    // Parse host:port
    size_t portStart = hostPart.rfind(':');
    size_t authEnd = hostPart.rfind('@');
    
    if (portStart != std::string::npos && (authEnd == std::string::npos || portStart > authEnd)) {
        url.host = hostPart.substr(0, portStart);
        url.port = std::stoi(hostPart.substr(portStart + 1));
    } else {
        url.host = hostPart;
        url.port = (url.protocol == "https") ? 443 : 80;
    }
    
    // Parse query string
    size_t queryStart = url.path.find('?');
    if (queryStart != std::string::npos) {
        url.query = url.path.substr(queryStart + 1);
        url.path = url.path.substr(0, queryStart);
    }
    
    // Parse fragment
    size_t fragmentStart = url.path.find('#');
    if (fragmentStart != std::string::npos) {
        url.fragment = url.path.substr(fragmentStart + 1);
        url.path = url.path.substr(0, fragmentStart);
    }
    
    return url;
}

void HttpClient::setTimeout(int timeoutMs) {
    timeoutMs_ = timeoutMs;
}

int HttpClient::getTimeout() const {
    return timeoutMs_;
}

void HttpClient::setUserAgent(const std::string& userAgent) {
    defaultHeaders_["User-Agent"] = userAgent;
}

void HttpClient::setDefaultHeader(const std::string& key, const std::string& value) {
    defaultHeaders_[key] = value;
}

void HttpClient::removeDefaultHeader(const std::string& key) {
    defaultHeaders_.erase(key);
}

void HttpClient::clearDefaultHeaders() {
    defaultHeaders_.clear();
}

// Multipart form data builder
std::string HttpClient::buildMultipartFormData(const MultipartFormData& formData, 
                                                std::string& contentType) {
    std::string boundary = "----NexusForgeBoundary" + generateBoundary();
    contentType = "multipart/form-data; boundary=" + boundary;
    
    std::stringstream ss;
    std::string crlf = "\r\n";
    
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
    // Simple random boundary generator
    const char* chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string result;
    
    for (int i = 0; i < 16; ++i) {
        result += chars[rand() % 62];
    }
    
    return result;
}

// JSON helper
json HttpClient::parseJsonResponse(const HttpResponse& response) {
    try {
        return json::parse(response.body);
    } catch (...) {
        return json::object();
    }
}

std::string HttpClient::jsonToBody(const json& data) {
    return data.dump();
}

} // namespace NexusForge::Networking
