// editor/features/code_folding.hpp
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <optional>

namespace NexusForge::Editor {

// Fold range
struct FoldRange {
    size_t startLine = 0;
    size_t endLine = 0;
    size_t foldStart = 0;  // Where fold marker appears
    size_t foldEnd = 0;    // Where fold region ends
    bool collapsed = false;
    std::string placeholder;
    int nestingLevel = 0;
};

// Fold provider interface
class IFoldProvider {
public:
    virtual ~IFoldProvider() = default;
    virtual std::vector<FoldRange> provideFolds(const std::string& text) = 0;
    virtual std::string getName() const = 0;
};

// Indentation-based fold provider
class IndentationFoldProvider : public IFoldProvider {
public:
    void setIndentSize(size_t spaces) { indentSize_ = spaces; }
    
    std::vector<FoldRange> provideFolds(const std::string& text) override;
    std::string getName() const override { return "indentation"; }
    
private:
    size_t indentSize_ = 4;
    size_t getIndentLevel(const std::string& line) const;
};

// Bracket-based fold provider
class BracketFoldProvider : public IFoldProvider {
public:
    void setBrackets(const std::pair<char, char>& brackets) { brackets_.push_back(brackets); }
    
    std::vector<FoldRange> provideFolds(const std::string& text) override;
    std::string getName() const override { return "brackets"; }
    
private:
    std::vector<std::pair<char, char>> brackets_;
};

// Language-specific fold provider
class LanguageFoldProvider : public IFoldProvider {
public:
    void setFoldMarkers(const std::vector<std::string>& startMarkers,
                        const std::vector<std::string>& endMarkers);
    
    std::vector<FoldRange> provideFolds(const std::string& text) override;
    std::string getName() const override { return "language"; }
    
private:
    std::vector<std::string> startMarkers_;
    std::vector<std::string> endMarkers_;
};

// Code folding manager
class CodeFoldingManager {
public:
    CodeFoldingManager();
    ~CodeFoldingManager();

    // Providers
    void addProvider(std::shared_ptr<IFoldProvider> provider);
    void removeProvider(const std::string& name);
    void clearProviders();

    // Folding
    void updateFolds(const std::string& text);
    const std::vector<FoldRange>& getFolds() const { return folds_; }
    
    // Fold manipulation
    void toggleFold(size_t line);
    void foldRegion(size_t startLine, size_t endLine);
    void unfoldRegion(size_t line);
    void foldAll();
    void unfoldAll();
    void foldLevel(int level);
    
    // Query
    bool isFolded(size_t line) const;
    std::optional<FoldRange> getFoldAtLine(size_t line) const;
    std::vector<FoldRange> getFoldsInRange(size_t startLine, size_t endLine) const;
    
    // Visibility
    size_t getNextFoldedLine(size_t fromLine) const;
    size_t getPreviousFoldedLine(size_t fromLine) const;
    
    // Line mapping (for rendering)
    size_t documentLineToViewLine(size_t docLine) const;
    size_t viewLineToDocumentLine(size_t viewLine) const;
    bool isLineVisible(size_t line) const;

    // Events
    using FoldChangedCallback = std::function<void(const FoldRange&, bool)>;
    void addFoldChangedListener(FoldChangedCallback callback);

private:
    std::vector<std::shared_ptr<IFoldProvider>> providers_;
    std::vector<FoldRange> folds_;
    std::vector<FoldChangedCallback> listeners_;
    
    void notifyFoldChanged(const FoldRange& fold, bool collapsed);
    void recalculateVisibility();
};

} // namespace NexusForge::Editor
