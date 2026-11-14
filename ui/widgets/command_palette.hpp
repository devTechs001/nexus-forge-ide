// ui/widgets/command_palette.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include <string>
#include <vector>
#include <functional>

namespace NexusForge::UI {

// Command item
struct CommandItem {
    std::string id;
    std::string label;
    std::string description;
    std::string icon;
    std::string category;
    std::vector<std::string> keywords;
    std::function<void()> action;
    int score = 0;
};

// Command Palette widget
class CommandPalette : public Widget {
public:
    using Ptr = std::shared_ptr<CommandPalette>;

    CommandPalette();
    ~CommandPalette() override;

    // Commands
    void addCommand(const CommandItem& command);
    void removeCommand(const std::string& id);
    void clearCommands();
    
    // Search
    void setSearchQuery(const std::string& query);
    const std::string& getSearchQuery() const { return searchQuery_; }
    
    // Selection
    void setSelectedIndex(size_t index);
    size_t getSelectedIndex() const { return selectedIndex_; }
    void selectNext();
    void selectPrevious();
    
    // Visibility
    void show();
    void hide();
    bool isVisible() const { return visible_ && Widget::isVisible(); }
    void toggle();
    
    // Events
    std::function<void(const CommandItem&)> onCommandExecuted;
    std::function<void()> onDismissed;
    
    // Widget overrides
    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;
    bool handleEvent(Core::Event& event) override;
    bool onKeyDown(const Core::KeyPressEvent& event) override;
    bool onCharInput(const Core::CharInputEvent& event) override;
    bool onMouseDown(const Core::MouseButtonPressEvent& event) override;
    bool onMouseMove(const Core::MouseMoveEvent& event) override;

private:
    std::vector<CommandItem> allCommands_;
    std::vector<CommandItem> filteredCommands_;
    std::string searchQuery_;
    size_t selectedIndex_ = 0;
    int scrollOffset_ = 0;
    int hoverIndex_ = -1;
    float lastMouseX_ = 0;
    float lastMouseY_ = 0;

    static constexpr size_t MAX_VISIBLE_ITEMS = 10;
    static constexpr float ITEM_HEIGHT = 32;
    
    void filterCommands();
    int calculateScore(const CommandItem& command, const std::string& query);
    void executeSelectedCommand();
    int getItemAtPosition(float y) const;
    void ensureSelectedVisible();
};

} // namespace NexusForge::UI
