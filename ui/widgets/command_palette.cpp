// ui/widgets/command_palette.cpp
#include "command_palette.hpp"
#include <algorithm>
#include <cctype>

namespace NexusForge::UI {

CommandPalette::CommandPalette() {
    style_.backgroundColor = GradientColor::fromHex(0x1E1E2E);
    style_.foregroundColor = GradientColor::fromHex(0xE8E8F0);
    style_.borderWidth = 1;
    style_.borderColor = GradientColor::fromHex(0x4A4A6A);
    style_.borderRadius = 8;
    style_.padding = EdgeInsets(0);
    
    // Shadow
    style_.hasShadow = true;
    style_.shadowColor = GradientColor::fromRGBA(0, 0, 0, 0.5f);
    style_.shadowBlur = 20;
    style_.shadowOffsetY = 10;
    
    setFocusPolicy(FocusPolicy::StrongFocus);
    hide();
}

CommandPalette::~CommandPalette() = default;

void CommandPalette::addCommand(const CommandItem& command) {
    allCommands_.push_back(command);
}

void CommandPalette::removeCommand(const std::string& id) {
    allCommands_.erase(
        std::remove_if(allCommands_.begin(), allCommands_.end(),
            [&id](const CommandItem& cmd) { return cmd.id == id; }),
        allCommands_.end()
    );
}

void CommandPalette::clearCommands() {
    allCommands_.clear();
    filteredCommands_.clear();
}

void CommandPalette::setSearchQuery(const std::string& query) {
    searchQuery_ = query;
    filterCommands();
    selectedIndex_ = 0;
    scrollOffset_ = 0;
}

void CommandPalette::setSelectedIndex(size_t index) {
    if (index < filteredCommands_.size()) {
        selectedIndex_ = index;
        ensureSelectedVisible();
    }
}

void CommandPalette::selectNext() {
    if (filteredCommands_.empty()) return;
    selectedIndex_ = (selectedIndex_ + 1) % filteredCommands_.size();
    ensureSelectedVisible();
}

void CommandPalette::selectPrevious() {
    if (filteredCommands_.empty()) return;
    selectedIndex_ = (selectedIndex_ + filteredCommands_.size() - 1) % filteredCommands_.size();
    ensureSelectedVisible();
}

void CommandPalette::show() {
    visible_ = true;
    searchQuery_.clear();
    filterCommands();
    selectedIndex_ = 0;
    requestFocus();
}

void CommandPalette::hide() {
    visible_ = false;
    if (onDismissed) {
        onDismissed();
    }
}

void CommandPalette::toggle() {
    if (isVisible()) {
        hide();
    } else {
        show();
    }
}

void CommandPalette::render(RenderEngine& renderer) {
    if (!isVisibleInHierarchy()) return;

    // Draw background with rounded corners
    renderer.drawRoundedRect(bounds_, Color(0.12f, 0.12f, 0.18f, 1.0f), style_.borderRadius);
    
    // Draw border
    renderer.drawRoundedRect(bounds_, Color(0.3f, 0.3f, 0.4f, 1.0f), style_.borderRadius);
    
    float y = bounds_.y + 8;
    float contentWidth = bounds_.width - 16;
    
    // Draw search input background
    Rect searchRect(bounds_.x + 8, y, contentWidth, 40);
    renderer.drawRoundedRect(searchRect, Color(0.15f, 0.15f, 0.22f, 1.0f), 6);
    
    // Draw search icon
    renderer.drawText("🔍", bounds_.x + 16, y + 26, Color(0.5f, 0.5f, 0.6f, 1.0f),
                      style_.fontFamily, 16);
    
    // Draw search text
    if (searchQuery_.empty()) {
        renderer.drawText("Type a command or search...", bounds_.x + 40, y + 26,
                          Color(0.4f, 0.4f, 0.5f, 1.0f), style_.fontFamily, 14);
    } else {
        renderer.drawText(searchQuery_, bounds_.x + 40, y + 26,
                          Color(0.9f, 0.9f, 0.95f, 1.0f), style_.fontFamily, 14);
    }
    
    // Draw separator
    y += 52;
    renderer.drawRect(Rect(bounds_.x + 8, y, contentWidth, 1), Color(0.2f, 0.2f, 0.3f, 1.0f));
    y += 4;
    
    // Draw commands list
    int startIdx = scrollOffset_;
    int endIdx = std::min(static_cast<int>(filteredCommands_.size()), 
                          startIdx + static_cast<int>(MAX_VISIBLE_ITEMS));
    
    for (int i = startIdx; i < endIdx; ++i) {
        const CommandItem& cmd = filteredCommands_[i];
        Rect itemRect(bounds_.x + 8, y, contentWidth, ITEM_HEIGHT);
        
        bool isSelected = (static_cast<size_t>(i) == selectedIndex_);
        bool isHovered = (i == hoverIndex_);
        
        // Draw selection background
        if (isSelected) {
            renderer.drawRoundedRect(itemRect, Color(0.3f, 0.4f, 0.6f, 0.4f), 4);
        } else if (isHovered) {
            renderer.drawRoundedRect(itemRect, Color(0.2f, 0.2f, 0.3f, 0.5f), 4);
        }
        
        // Draw icon
        if (!cmd.icon.empty()) {
            renderer.drawText(cmd.icon, bounds_.x + 16, y + 22,
                              Color(0.6f, 0.6f, 0.7f, 1.0f), style_.fontFamily, 16);
        }
        
        // Draw label
        float labelX = bounds_.x + 40;
        renderer.drawText(cmd.label, labelX, y + 22,
                          isSelected ? Color::White() : Color(0.8f, 0.8f, 0.9f, 1.0f),
                          style_.fontFamily, 14);
        
        // Draw description (truncated)
        if (!cmd.description.empty()) {
            float descX = bounds_.x + contentWidth - 10;
            // Would measure and truncate
            renderer.drawText(cmd.description, descX, y + 22,
                              Color(0.5f, 0.5f, 0.6f, 1.0f),
                              style_.fontFamily, 12);
        }
        
        y += ITEM_HEIGHT;
    }
    
    // Draw footer hint
    y += 8;
    renderer.drawText("↑↓ to navigate • Enter to select • Esc to close",
                      bounds_.x + 16, y + 12,
                      Color(0.4f, 0.4f, 0.5f, 1.0f), style_.fontFamily, 11);
}

Vec2 CommandPalette::measure(float availableWidth, float availableHeight) {
    float width = std::min(600.0f, availableWidth - 100);
    float height = 56 + MAX_VISIBLE_ITEMS * ITEM_HEIGHT + 40;
    return Vec2(width, height);
}

bool CommandPalette::handleEvent(Core::Event& event) {
    return Widget::handleEvent(event);
}

bool CommandPalette::onKeyDown(const Core::KeyPressEvent& event) {
    switch (event.keyCode) {
        case 27: // Escape
            hide();
            return true;
        case 8: // Backspace
            if (!searchQuery_.empty()) {
                searchQuery_.pop_back();
                filterCommands();
                selectedIndex_ = 0;
                scrollOffset_ = 0;
            }
            return true;
        case 38: // Up arrow
            selectPrevious();
            return true;
        case 40: // Down arrow
            selectNext();
            return true;
        case 13: // Enter
            executeSelectedCommand();
            return true;
    }
    return false;
}

bool CommandPalette::onCharInput(const Core::CharInputEvent& event) {
    char c = static_cast<char>(event.codepoint);
    if (c >= 32 && c != 127) {
        searchQuery_ += c;
        filterCommands();
        selectedIndex_ = 0;
        scrollOffset_ = 0;
    }
    return true;
}

bool CommandPalette::onMouseDown(const Core::MouseButtonPressEvent& event) {
    int index = getItemAtPosition(event.y);
    if (index >= 0 && index < static_cast<int>(filteredCommands_.size())) {
        setSelectedIndex(index);
        executeSelectedCommand();
        return true;
    }
    return false;
}

bool CommandPalette::onMouseMove(const Core::MouseMoveEvent& event) {
    lastMouseX_ = event.x;
    lastMouseY_ = event.y;
    hoverIndex_ = getItemAtPosition(event.y);
    markDirty();
    return false;
}

void CommandPalette::filterCommands() {
    filteredCommands_.clear();
    
    if (searchQuery_.empty()) {
        filteredCommands_ = allCommands_;
        return;
    }
    
    // Score and filter commands
    std::vector<std::pair<int, CommandItem>> scored;
    
    for (const auto& cmd : allCommands_) {
        int score = calculateScore(cmd, searchQuery_);
        if (score > 0) {
            scored.push_back({score, cmd});
        }
    }
    
    // Sort by score
    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    for (const auto& [score, cmd] : scored) {
        filteredCommands_.push_back(cmd);
    }
}

int CommandPalette::calculateScore(const CommandItem& command, const std::string& query) {
    int score = 0;
    auto toLower = [](unsigned char c) { return static_cast<char>(std::tolower(c)); };
    std::string lowerQuery = query;
    std::string lowerLabel = command.label;

    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), toLower);
    std::transform(lowerLabel.begin(), lowerLabel.end(), lowerLabel.begin(), toLower);

    // Exact match
    if (lowerLabel.find(lowerQuery) != std::string::npos) {
        score += 100;
    }

    // Starts with match
    if (lowerLabel.find(lowerQuery) == 0) {
        score += 50;
    }

    // Keyword match
    for (const auto& keyword : command.keywords) {
        std::string lowerKeyword = keyword;
        std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), toLower);
        if (lowerKeyword.find(lowerQuery) != std::string::npos) {
            score += 30;
        }
    }

    // Category match
    if (!command.category.empty()) {
        std::string lowerCategory = command.category;
        std::transform(lowerCategory.begin(), lowerCategory.end(), lowerCategory.begin(), toLower);
        if (lowerCategory.find(lowerQuery) != std::string::npos) {
            score += 20;
        }
    }

    return score;
}

void CommandPalette::executeSelectedCommand() {
    if (selectedIndex_ < filteredCommands_.size()) {
        CommandItem cmd = filteredCommands_[selectedIndex_];
        hide();
        if (cmd.action) {
            cmd.action();
        }
        if (onCommandExecuted) {
            onCommandExecuted(cmd);
        }
    }
}

int CommandPalette::getItemAtPosition(float y) const {
    float itemStartY = bounds_.y + 56;

    if (y < itemStartY) return -1;

    int visualIndex = static_cast<int>((y - itemStartY) / ITEM_HEIGHT);
    if (visualIndex < 0 || visualIndex >= static_cast<int>(MAX_VISIBLE_ITEMS)) {
        return -1;
    }

    int actualIndex = visualIndex + scrollOffset_;
    if (actualIndex >= 0 && actualIndex < static_cast<int>(filteredCommands_.size())) {
        return actualIndex;
    }

    return -1;
}

void CommandPalette::ensureSelectedVisible() {
    if (selectedIndex_ < static_cast<size_t>(scrollOffset_)) {
        scrollOffset_ = selectedIndex_;
    } else if (selectedIndex_ >= static_cast<size_t>(scrollOffset_) + MAX_VISIBLE_ITEMS) {
        scrollOffset_ = selectedIndex_ - MAX_VISIBLE_ITEMS + 1;
    }
}

} // namespace NexusForge::UI
