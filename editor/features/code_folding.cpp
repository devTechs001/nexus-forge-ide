// editor/features/code_folding.cpp
// Code Folding Implementation

#include "code_folding.hpp"
#include "../buffer/text_buffer.hpp"
#include <algorithm>
#include <stack>

namespace NexusForge::Editor {

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
    size_t count = 0;
    for (const auto& region : foldRegions_) {
        if (region.collapsed) {
            count += region.endLine - region.startLine - 1;
        }
    }
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

} // namespace NexusForge::Editor
