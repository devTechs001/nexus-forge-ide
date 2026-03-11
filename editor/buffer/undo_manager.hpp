// editor/buffer/undo_manager.hpp
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>

namespace NexusForge::Editor {

// Change types
enum class ChangeType {
    Insert,
    Delete,
    Replace,
    Move,
    Format
};

// Single change record
struct Change {
    ChangeType type = ChangeType::Insert;
    size_t startLine = 0;
    size_t startColumn = 0;
    size_t endLine = 0;
    size_t endColumn = 0;
    std::string oldText;
    std::string newText;
    std::optional<size> cursorPosition;
};

// Undoable action
struct UndoAction {
    std::string description;
    std::vector<Change> changes;
    uint64_t timestamp = 0;
    bool mergeable = false;
    std::string groupId;
};

// Undo manager
class UndoManager {
public:
    UndoManager();
    ~UndoManager();
    
    // Configuration
    void setMaxUndoDepth(size_t depth) { maxDepth_ = depth; }
    size_t getMaxUndoDepth() const { return maxDepth_; }
    
    void setMergeInterval(int milliseconds) { mergeInterval_ = milliseconds; }
    int getMergeInterval() const { return mergeInterval_; }
    
    // Begin/end compound actions
    void beginGroup(const std::string& description = "");
    void endGroup();
    bool isInGroup() const { return groupDepth_ > 0; }
    
    // Record changes
    void push(const Change& change);
    void push(const std::vector<Change>& changes, const std::string& description = "");
    
    // Undo/Redo
    bool canUndo() const { return !undoStack_.empty(); }
    bool canRedo() const { return !redoStack_.empty(); }
    
    std::optional<UndoAction> undo();
    std::optional<UndoAction> redo();
    
    void undo(int count);
    void redo(int count);
    
    // Clear
    void clear();
    void clearRedo();
    void clearUndo();
    
    // State
    size_t getUndoStackDepth() const { return undoStack_.size(); }
    size_t getRedoStackDepth() const { return redoStack_.size(); }
    
    std::string getNextUndoDescription() const;
    std::string getNextRedoDescription() const;
    
    // Modified state
    void markClean();
    bool isClean() const { return cleanState_ >= 0; }
    void setModified();
    bool isModified() const { return cleanState_ < 0; }
    
    // Events
    using ChangeCallback = std::function<void()>;
    void addChangeListener(ChangeCallback callback);
    
private:
    std::vector<UndoAction> undoStack_;
    std::vector<UndoAction> redoStack_;
    size_t maxDepth_ = 1000;
    int mergeInterval_ = 500;  // milliseconds
    
    // Grouping
    int groupDepth_ = 0;
    std::vector<Change> currentGroup_;
    std::string currentGroupDescription_;
    
    // Modified tracking
    int cleanState_ = 0;  // -1 = modified, >= 0 = index of clean state
    
    // Listeners
    std::vector<ChangeCallback> listeners_;
    
    bool canMerge(const UndoAction& previous, const UndoAction& current) const;
    void notifyChange();
    void trimStack();
};

// Change builder for fluent API
class ChangeBuilder {
public:
    ChangeBuilder& insert(size_t line, size_t column, const std::string& text);
    ChangeBuilder& remove(size_t startLine, size_t startColumn,
                          size_t endLine, size_t endColumn);
    ChangeBuilder& replace(size_t startLine, size_t startColumn,
                           size_t endLine, size_t endColumn, const std::string& text);
    ChangeBuilder& move(size_t fromLine, size_t fromColumn,
                        size_t toLine, size_t toColumn);
    
    Change build() const;
    std::vector<Change> buildMultiple() const;
    
private:
    std::vector<Change> changes_;
};

// Document snapshot for complex undo scenarios
class DocumentSnapshot {
public:
    DocumentSnapshot();
    
    void capture(const std::string& content);
    void restore();
    
    std::string getContent() const { return content_; }
    uint64_t getTimestamp() const { return timestamp_; }
    
    using RestoreCallback = std::function<void(const std::string&)>;
    void setRestoreCallback(RestoreCallback callback);
    
private:
    std::string content_;
    uint64_t timestamp_ = 0;
    RestoreCallback restoreCallback_;
};

} // namespace NexusForge::Editor
