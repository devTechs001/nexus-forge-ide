// editor/buffer/undo_manager.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace NexusForge::Editor {

enum class ActionType {
    Insert,
    Delete,
    Replace
};

class UndoManager {
public:
    struct UndoAction {
        ActionType type = ActionType::Insert;
        size_t line = 0;
        size_t col = 0;
        size_t endLine = 0;
        size_t endCol = 0;
        std::string text;
        std::string oldText;
        int groupId = 0;
    };

    UndoManager();
    ~UndoManager();

    void beginUndoGroup(const std::string& name);
    void endUndoGroup();

    void push(UndoAction action);
    void pushInsert(size_t line, size_t col, const std::string& text);
    void pushDelete(size_t line, size_t col, const std::string& text);
    void pushReplace(size_t startLine, size_t startCol,
                     size_t endLine, size_t endCol,
                     const std::string& oldText, const std::string& newText);

    bool canUndo() const;
    bool canRedo() const;
    bool canUndoGroup() const;

    void undo();
    void undoGroup();
    void redo();
    void redoGroup();

    void clear();
    void clearRedo();

    size_t getUndoStackSize() const;
    size_t getRedoStackSize() const;

    void setMaxUndoSize(int max);
    int getMaxUndoSize() const;

    void markClean();
    bool isDirty() const;
    bool isUndoing() const;

    std::string getGroupName(int groupId) const;

    std::function<void(bool isUndo)> onUndoRedo;
    std::function<void(ActionType type, size_t line, size_t col, const std::string& text)> onAction;

private:
    std::vector<UndoAction> undoStack_;
    std::vector<UndoAction> redoStack_;
    int maxUndoStack_ = 100;
    bool isUndoing_ = false;
    int currentGroupId_ = 0;
    std::unordered_map<int, std::string> groupNames_;
    size_t cleanState_ = 0;

    UndoAction createInverse(const UndoAction& action) const;
    void applyInverse(const UndoAction& action);
    void applyAction(const UndoAction& action);
};

} // namespace NexusForge::Editor
