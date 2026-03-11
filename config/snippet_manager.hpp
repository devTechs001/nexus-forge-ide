// config/snippet_manager.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace NexusForge::Config {

// Snippet variable
struct SnippetVariable {
    std::string name;
    std::string defaultValue;
    std::vector<std::string> choices;
    std::string description;
};

// Code snippet
struct Snippet {
    std::string id;
    std::string prefix;
    std::string body;
    std::string description;
    std::string scope;  // Language scope
    std::string tagDescription;
    std::vector<std::string> aliases;
    std::vector<SnippetVariable> variables;
    int priority = 0;
};

// Snippet manager
class SnippetManager {
public:
    static SnippetManager& getInstance();

    // Snippet management
    void addSnippet(const Snippet& snippet);
    void removeSnippet(const std::string& id);
    void updateSnippet(const std::string& id, const Snippet& snippet);

    // Lookup
    std::vector<Snippet> getSnippetsByPrefix(const std::string& prefix,
                                              const std::string& language = "") const;
    std::vector<Snippet> getSnippetsForLanguage(const std::string& language) const;
    const Snippet* getSnippet(const std::string& id) const;

    // Load/Save
    bool loadSnippets(const std::string& path);
    bool saveSnippets(const std::string& path) const;
    void loadDefaultSnippets();

    // User snippets
    void loadUserSnippets();
    void saveUserSnippet(const Snippet& snippet);

    // Variable expansion
    std::string expandVariables(const std::string& body) const;
    std::string expandVariable(const std::string& name) const;

    // Variables
    void setVariable(const std::string& name, const std::string& value);
    void setVariableProvider(const std::string& name,
                              std::function<std::string()> provider);

private:
    SnippetManager();

    std::unordered_map<std::string, Snippet> snippets_;
    std::unordered_map<std::string, std::vector<Snippet>> byPrefix_;
    std::unordered_map<std::string, std::vector<Snippet>> byLanguage_;
    std::unordered_map<std::string, std::string> variables_;
    std::unordered_map<std::string, std::function<std::string()>> variableProviders_;

    void indexSnippet(const Snippet& snippet);
    void loadSnippetsFromDirectory(const std::string& path);
};

// Built-in snippet variables
namespace SnippetVariables {
    std::string currentDate();
    std::string currentTime();
    std::string currentTimestamp();
    std::string username();
    std::string filepath();
    std::string filename();
    std::string fileExtension();
    std::string relativeFilepath();
    std::string selectedText();
    std::string lineContent();
    std::string lineNumber();
    std::string clipboardContent();
    std::string workspaceRoot();
}

} // namespace NexusForge::Config
