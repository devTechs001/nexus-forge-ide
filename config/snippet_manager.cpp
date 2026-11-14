// config/snippet_manager.cpp
// Snippet Manager Implementation

#include "snippet_manager.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>

namespace NexusForge::Config {

SnippetManager::SnippetManager() {
    loadDefaultSnippets();
}

SnippetManager& SnippetManager::getInstance() {
    static SnippetManager instance;
    return instance;
}

void SnippetManager::addSnippet(const Snippet& snippet) {
    snippets_[snippet.id] = snippet;
    indexSnippet(snippet);
}

void SnippetManager::removeSnippet(const std::string& id) {
    auto it = snippets_.find(id);
    if (it == snippets_.end()) return;

    // Remove from prefix index
    auto& byPref = byPrefix_[it->second.prefix];
    byPref.erase(std::remove_if(byPref.begin(), byPref.end(),
        [&](const Snippet& s) { return s.id == id; }), byPref.end());

    // Remove from language index
    if (!it->second.scope.empty()) {
        auto& byLang = byLanguage_[it->second.scope];
        byLang.erase(std::remove_if(byLang.begin(), byLang.end(),
            [&](const Snippet& s) { return s.id == id; }), byLang.end());
    }

    snippets_.erase(it);
}

void SnippetManager::updateSnippet(const std::string& id, const Snippet& snippet) {
    removeSnippet(id);
    addSnippet(snippet);
}

std::vector<Snippet> SnippetManager::getSnippetsByPrefix(const std::string& prefix,
                                                          const std::string& language) const {
    std::vector<Snippet> result;
    auto it = byPrefix_.find(prefix);
    if (it != byPrefix_.end()) {
        for (const auto& s : it->second) {
            if (language.empty() || s.scope == language) {
                result.push_back(s);
            }
        }
    }
    return result;
}

std::vector<Snippet> SnippetManager::getSnippetsForLanguage(const std::string& language) const {
    auto it = byLanguage_.find(language);
    if (it != byLanguage_.end()) return it->second;
    return {};
}

const Snippet* SnippetManager::getSnippet(const std::string& id) const {
    auto it = snippets_.find(id);
    if (it != snippets_.end()) return &it->second;
    return nullptr;
}

bool SnippetManager::loadSnippets(const std::string& path) {
    return true;
}

bool SnippetManager::saveSnippets(const std::string& path) const {
    return true;
}

void SnippetManager::loadDefaultSnippets() {
}

void SnippetManager::loadUserSnippets() {
}

void SnippetManager::saveUserSnippet(const Snippet& snippet) {
}

std::string SnippetManager::expandVariables(const std::string& body) const {
    std::string result = body;
    for (const auto& [name, value] : variables_) {
        size_t pos = 0;
        std::string var = "${" + name + "}";
        while ((pos = result.find(var, pos)) != std::string::npos) {
            result.replace(pos, var.length(), value);
            pos += value.length();
        }
    }
    return result;
}

std::string SnippetManager::expandVariable(const std::string& name) const {
    auto it = variables_.find(name);
    if (it != variables_.end()) return it->second;

    auto provIt = variableProviders_.find(name);
    if (provIt != variableProviders_.end()) return provIt->second();

    return "";
}

void SnippetManager::setVariable(const std::string& name, const std::string& value) {
    variables_[name] = value;
}

void SnippetManager::setVariableProvider(const std::string& name,
                                          std::function<std::string()> provider) {
    variableProviders_[name] = std::move(provider);
}

void SnippetManager::indexSnippet(const Snippet& snippet) {
    byPrefix_[snippet.prefix].push_back(snippet);
    if (!snippet.scope.empty()) {
        byLanguage_[snippet.scope].push_back(snippet);
    }
}

void SnippetManager::loadSnippetsFromDirectory(const std::string& path) {
}

} // namespace NexusForge::Config