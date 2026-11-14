// editor/features/code_folding.cpp
// Code Folding Implementation

#include "code_folding.hpp"
#include "../buffer/text_buffer.hpp"
#include <algorithm>
#include <sstream>
#include <stack>

namespace NexusForge::Editor {

// Fold providers
size_t IndentationFoldProvider::getIndentLevel(const std::string& line) const {
    size_t spaces = 0;
    for (char c : line) {
        if (c == ' ') spaces++;
        else if (c == '\t') spaces += indentSize_;
        else break;
    }
    return indentSize_ > 0 ? spaces / indentSize_ : 0;
}

std::vector<FoldRange> IndentationFoldProvider::provideFolds(const std::string& text) {
    std::vector<FoldRange> folds;
    std::vector<std::string> lines;
    std::istringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) lines.push_back(line);

    for (size_t i = 0; i < lines.size(); ++i) {
        if (lines[i].find_first_not_of(" \t\r") == std::string::npos) continue;
        size_t baseIndent = getIndentLevel(lines[i]);
        size_t endLine = i;
        for (size_t j = i + 1; j < lines.size(); ++j) {
            if (lines[j].find_first_not_of(" \t\r") == std::string::npos) continue;
            if (getIndentLevel(lines[j]) > baseIndent) {
                endLine = j;
            } else {
                break;
            }
        }
        if (endLine > i) {
            FoldRange r;
            r.startLine = i;
            r.endLine = endLine;
            r.nestingLevel = static_cast<int>(baseIndent);
            folds.push_back(r);
        }
    }
    return folds;
}

std::vector<FoldRange> BracketFoldProvider::provideFolds(const std::string& text) {
    std::vector<FoldRange> folds;
    std::vector<std::pair<char, char>> pairs = brackets_;
    if (pairs.empty()) pairs = {{'{', '}'}};

    std::istringstream ss(text);
    std::string line;
    size_t lineIdx = 0;
    std::vector<std::pair<char, size_t>> stack;
    while (std::getline(ss, line)) {
        for (char ch : line) {
            for (const auto& [o, c] : pairs) {
                if (ch == o) {
                    stack.push_back({c, lineIdx});
                } else if (ch == c && !stack.empty() && stack.back().first == c) {
                    size_t startLine = stack.back().second;
                    stack.pop_back();
                    if (startLine < lineIdx) {
                        FoldRange r;
                        r.startLine = startLine;
                        r.endLine = lineIdx;
                        r.nestingLevel = static_cast<int>(stack.size());
                        folds.push_back(r);
                    }
                }
            }
        }
        ++lineIdx;
    }
    return folds;
}

void LanguageFoldProvider::setFoldMarkers(const std::vector<std::string>& startMarkers,
                                           const std::vector<std::string>& endMarkers) {
    startMarkers_ = startMarkers;
    endMarkers_ = endMarkers;
}

std::vector<FoldRange> LanguageFoldProvider::provideFolds(const std::string& text) {
    std::vector<FoldRange> folds;
    std::istringstream ss(text);
    std::string line;
    size_t lineIdx = 0;
    std::stack<size_t> opens;
    while (std::getline(ss, line)) {
        for (const auto& m : startMarkers_) {
            if (!m.empty() && line.find(m) != std::string::npos) {
                opens.push(lineIdx);
                break;
            }
        }
        for (const auto& m : endMarkers_) {
            if (!m.empty() && line.find(m) != std::string::npos && !opens.empty()) {
                size_t startLine = opens.top();
                opens.pop();
                if (startLine < lineIdx) {
                    FoldRange r;
                    r.startLine = startLine;
                    r.endLine = lineIdx;
                    folds.push_back(r);
                }
                break;
            }
        }
        ++lineIdx;
    }
    return folds;
}

CodeFoldingManager::CodeFoldingManager() : buffer_(nullptr) {
}

CodeFoldingManager::CodeFoldingManager(TextBuffer* buffer) : buffer_(buffer) {
}

CodeFoldingManager::~CodeFoldingManager() = default;

void CodeFoldingManager::updateFoldRegions() {
    if (!buffer_) return;
    
    foldRegions_.clear();
    
    // Simple brace-based folding
    std::stack<size_t> openBraces;
    
    for (size_t line = 0; line < buffer_->getLineCount(); ++line) {
        std::string lineText = buffer_->getLine(line);
        
        for (size_t i = 0; i < lineText.length(); ++i) {
            if (lineText[i] == '{') {
                openBraces.push(line);
            } else if (lineText[i] == '}' && !openBraces.empty()) {
                size_t startLine = openBraces.top();
                openBraces.pop();
                
                if (startLine < line) {
                    FoldRegion region;
                    region.startLine = startLine;
                    region.endLine = line;
                    region.collapsed = false;
                    foldRegions_.push_back(region);
                }
            }
        }
    }
    
    // Sort by start line
    std::sort(foldRegions_.begin(), foldRegions_.end(),
        [](const FoldRegion& a, const FoldRegion& b) {
            return a.startLine < b.startLine;
        });
}

void CodeFoldingManager::toggleFold(size_t line) {
    for (auto& region : foldRegions_) {
        if (region.startLine == line) {
            region.collapsed = !region.collapsed;
            return;
        }
    }
}

void CodeFoldingManager::foldRegion(size_t line) {
    for (auto& region : foldRegions_) {
        if (region.startLine == line) {
            region.collapsed = true;
            return;
        }
    }
}

void CodeFoldingManager::unfoldRegion(size_t line) {
    for (auto& region : foldRegions_) {
        if (region.startLine == line) {
            region.collapsed = false;
            return;
        }
    }
}

void CodeFoldingManager::foldAll() {
    for (auto& region : foldRegions_) {
        region.collapsed = true;
    }
}

void CodeFoldingManager::unfoldAll() {
    for (auto& region : foldRegions_) {
        region.collapsed = false;
    }
}

bool CodeFoldingManager::isLineFolded(size_t line) const {
    for (const auto& region : foldRegions_) {
        if (region.collapsed && line > region.startLine && line <= region.endLine) {
            return true;
        }
    }
    return false;
}

size_t CodeFoldingManager::getFoldedLineCount() const {
    // Merge collapsed (startLine, endLine] intervals so nested folds are not double-counted
    std::vector<std::pair<size_t, size_t>> intervals;
    for (const auto& region : foldRegions_) {
        if (region.collapsed && region.endLine > region.startLine) {
            intervals.emplace_back(region.startLine + 1, region.endLine);
        }
    }
    if (intervals.empty()) return 0;
    std::sort(intervals.begin(), intervals.end());
    size_t count = 0;
    size_t curStart = intervals[0].first;
    size_t curEnd = intervals[0].second;
    for (size_t i = 1; i < intervals.size(); ++i) {
        if (intervals[i].first <= curEnd + 1) {
            curEnd = std::max(curEnd, intervals[i].second);
        } else {
            count += (curEnd - curStart + 1);
            curStart = intervals[i].first;
            curEnd = intervals[i].second;
        }
    }
    count += (curEnd - curStart + 1);
    return count;
}

const std::vector<CodeFoldingManager::FoldRegion>& CodeFoldingManager::getFoldRegions() const {
    return foldRegions_;
}

size_t CodeFoldingManager::getNextFoldLine(size_t currentLine) const {
    for (const auto& region : foldRegions_) {
        if (region.startLine > currentLine) {
            return region.startLine;
        }
    }
    return currentLine;
}

size_t CodeFoldingManager::getPreviousFoldLine(size_t currentLine) const {
    size_t prevFold = currentLine;
    for (const auto& region : foldRegions_) {
        if (region.startLine < currentLine) {
            prevFold = region.startLine;
        }
    }
    return prevFold;
}

bool CodeFoldingManager::canFoldAtLine(size_t line) const {
    for (const auto& region : foldRegions_) {
        if (region.startLine == line) {
            return true;
        }
    }
    return false;
}

std::string CodeFoldingManager::getFoldPlaceholder(size_t line) const {
    for (const auto& region : foldRegions_) {
        if (region.startLine == line && region.collapsed) {
            return "...";
        }
    }
    return "";
}

void CodeFoldingManager::setBuffer(TextBuffer* buffer) {
    buffer_ = buffer;
    if (buffer_) {
        updateFoldRegions();
    }
}

void CodeFoldingManager::addProvider(std::shared_ptr<IFoldProvider> provider) {
    if (provider) providers_.push_back(std::move(provider));
}

void CodeFoldingManager::removeProvider(const std::string& name) {
    providers_.erase(std::remove_if(providers_.begin(), providers_.end(),
        [&](const auto& p) { return p && p->getName() == name; }), providers_.end());
}

void CodeFoldingManager::clearProviders() {
    providers_.clear();
}

void CodeFoldingManager::updateFolds(const std::string& text) {
    foldRegions_.clear();
    if (providers_.empty()) {
        BracketFoldProvider bp;
        foldRegions_ = bp.provideFolds(text);
    } else {
        for (const auto& p : providers_) {
            auto provided = p->provideFolds(text);
            foldRegions_.insert(foldRegions_.end(), provided.begin(), provided.end());
        }
    }
    std::sort(foldRegions_.begin(), foldRegions_.end(),
        [](const FoldRange& a, const FoldRange& b) { return a.startLine < b.startLine; });
}

void CodeFoldingManager::foldRegion(size_t startLine, size_t endLine) {
    for (auto& region : foldRegions_) {
        if (region.startLine == startLine && region.endLine == endLine) {
            region.collapsed = true;
            notifyFoldChanged(region, true);
            return;
        }
    }
    if (endLine > startLine) {
        FoldRange r;
        r.startLine = startLine;
        r.endLine = endLine;
        r.collapsed = true;
        foldRegions_.push_back(r);
        notifyFoldChanged(r, true);
    }
}

void CodeFoldingManager::foldLevel(int level) {
    for (auto& region : foldRegions_) {
        if (region.nestingLevel >= level) {
            region.collapsed = true;
            notifyFoldChanged(region, true);
        }
    }
}

std::optional<FoldRange> CodeFoldingManager::getFoldAtLine(size_t line) const {
    for (const auto& region : foldRegions_) {
        if (region.startLine == line) return region;
    }
    return std::nullopt;
}

std::vector<FoldRange> CodeFoldingManager::getFoldsInRange(size_t startLine, size_t endLine) const {
    std::vector<FoldRange> result;
    for (const auto& region : foldRegions_) {
        if (region.startLine >= startLine && region.startLine <= endLine) {
            result.push_back(region);
        }
    }
    return result;
}

size_t CodeFoldingManager::documentLineToViewLine(size_t docLine) const {
    size_t hidden = 0;
    for (size_t line = 0; line < docLine; ++line) {
        if (isLineFolded(line)) hidden++;
    }
    return docLine - hidden;
}

size_t CodeFoldingManager::viewLineToDocumentLine(size_t viewLine) const {
    size_t docLine = 0;
    size_t visible = 0;
    while (visible < viewLine) {
        if (!isLineFolded(docLine)) visible++;
        docLine++;
    }
    while (isLineFolded(docLine)) docLine++;
    return docLine;
}

void CodeFoldingManager::addFoldChangedListener(FoldChangedCallback callback) {
    if (callback) listeners_.push_back(std::move(callback));
}

void CodeFoldingManager::notifyFoldChanged(const FoldRange& fold, bool collapsed) {
    for (const auto& cb : listeners_) {
        if (cb) cb(fold, collapsed);
    }
}

void CodeFoldingManager::recalculateVisibility() {
}

} // namespace NexusForge::Editor
