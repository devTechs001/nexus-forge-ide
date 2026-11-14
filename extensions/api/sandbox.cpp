// extensions/api/sandbox.cpp
// Extension Sandbox Implementation

#include "sandbox.hpp"
#include <iostream>

namespace NexusForge::Extensions {

// SandboxAPI stub implementations
bool SandboxAPI::readFile(const std::string& path, std::string& content) { return false; }
bool SandboxAPI::writeFile(const std::string& path, const std::string& content) { return false; }
bool SandboxAPI::fileExists(const std::string& path) { return false; }
bool SandboxAPI::httpGet(const std::string& url, std::string& response) { return false; }
bool SandboxAPI::httpPost(const std::string& url, const std::string& body, std::string& response) { return false; }
void SandboxAPI::log(const std::string& message) { std::cout << "[Sandbox] " << message << std::endl; }
void SandboxAPI::warn(const std::string& message) { std::cout << "[Sandbox WARN] " << message << std::endl; }
void SandboxAPI::error(const std::string& message) { std::cerr << "[Sandbox ERROR] " << message << std::endl; }
int SandboxAPI::setTimeout(std::function<void()> callback, int delay) { return 0; }
int SandboxAPI::setInterval(std::function<void()> callback, int interval) { return 0; }
void SandboxAPI::clearTimeout(int id) {}
void SandboxAPI::clearInterval(int id) {}

} // namespace NexusForge::Extensions