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
    std::string shortcut;
    bool enabled = true;
    int score = 0;  // For fuzzy matching
    void* data = nullptr;
};

// Command palette widget
class CommandPalette : public Widget {
public:
    using Ptr = std::shared_ptr<CommandPalette>;

    CommandPalette();
    ~CommandPalette() override;

    // Commands
    void addCommand(const CommandItem& command);
    void removeCommand(const std::string& id);
    void clearCommands();

    const std::vector<CommandItem>& getCommands() const { return commands_; }

    // Search
    void setSearchText(const std::string& text);
    const std::string& getSearchText() const { return searchText_; }

    void updateResults();
    const std::vector<CommandItem>& getFilteredCommands() const { return filteredCommands_; }

    // Selection
    void setSelectedIndex(int index);
    int getSelectedIndex() const { return selectedIndex_; }
    CommandItem* getSelectedCommand();

    void selectNext();
    void selectPrevious();
    void selectFirst();
    void selectLast();

    // Execution
    void executeSelected();
    void executeCommand(const std::string& id);

    // Appearance
    void setMaxVisibleItems(int count) { maxVisibleItems_ = count; }
    int getMaxVisibleItems() const { return maxVisibleItems_; }

    void setPlaceholder(const std::string& placeholder) { placeholder_ = placeholder; }
    const std::string& getPlaceholder() const { return placeholder_; }

    // Callbacks
    std::function<void(const CommandItem&)> onCommandExecuted;
    std::function<void()> onDismissed;

    // Show/Hide
    void show();
    void hide();
    bool isVisible() const { return visible_; }

    // Rendering
    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;

    // Events
    bool onCharInput(const Core::CharInputEvent& event) override;
    bool onKeyDown(const Core::KeyPressEvent& event) override;
    bool onMouseDown(const Core::MouseButtonPressEvent& event) override;
    bool onMouseMove(const Core::MouseMoveEvent& event) override;

protected:
    int fuzzyMatch(const std::string& pattern, const std::string& text);
    void sortResults();
    Rect getItemBounds(int index) const;
    int getItemAtPosition(float x, float y) const;

    std::vector<CommandItem> commands_;
    std::vector<CommandItem> filteredCommands_;
    std::string searchText_;
    int selectedIndex_ = 0;
    int maxVisibleItems_ = 10;
    std::string placeholder_ = "Type a command...";

    float itemHeight_ = 32;
    float scrollOffset_ = 0;
};

// Quick open (file search)
class QuickOpen : public CommandPalette {
public:
    QuickOpen();

    void setFileProvider(std::function<std::vector<std::string>()> provider);
    void updateFileList();

    std::function<void(const std::string&)> onFileSelected;

private:
    std::function<std::vector<std::string>()> fileProvider_;
    std::vector<std::string> files_;
};

} // namespace NexusForge::UI
