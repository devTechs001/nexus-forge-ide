// editor/buffer/undo_manager.cpp
// Undo Manager Implementation

#include "undo_manager.hpp"
#include <algorithm>

namespace NexusForge::Editor {

UndoManager::UndoManager() 
    : maxUndoStack_(100), 
      isUndoing_(false),
      currentGroupId_(0) {
}

UndoManager::~UndoManager() = default;

void UndoManager::beginUndoGroup(const std::string& name) {
    currentGroupId_++;
    groupNames_[currentGroupId_] = name;
}

void UndoManager::endUndoGroup() {
    // Group is implicitly ended when a new action is added
}

void UndoManager::push(UndoAction action) {
    if (isUndoing_) return;
    
    // Clear redo stack
    redoStack_.clear();
    
    // Push to undo stack
    if (action.groupId == 0) {
        action.groupId = currentGroupId_;
    }
    
    undoStack_.push_back(action);
    
    // Limit stack size
    while (undoStack_.size() > static_cast<size_t>(maxUndoStack_)) {
        undoStack_.erase(undoStack_.begin());
    }
}

void UndoManager::pushInsert(size_t line, size_t col, const std::string& text) {
    UndoAction action;
    action.type = ActionType::Insert;
    action.line = line;
    action.col = col;
    action.text = text;
    action.groupId = currentGroupId_;
    push(action);
}

void UndoManager::pushDelete(size_t line, size_t col, const std::string& text) {
    UndoAction action;
    action.type = ActionType::Delete;
    action.line = line;
    action.col = col;
    action.text = text;
    action.groupId = currentGroupId_;
    push(action);
}

void UndoManager::pushReplace(size_t startLine, size_t startCol,
                               size_t endLine, size_t endCol,
                               const std::string& oldText, const std::string& newText) {
    UndoAction action;
    action.type = ActionType::Replace;
    action.line = startLine;
    action.col = startCol;
    action.endLine = endLine;
    action.endCol = endCol;
    action.text = newText;
    action.oldText = oldText;
    action.groupId = currentGroupId_;
    push(action);
}

bool UndoManager::canUndo() const {
    return !undoStack_.empty();
}

bool UndoManager::canRedo() const {
    return !redoStack_.empty();
}

bool UndoManager::canUndoGroup() const {
    if (undoStack_.empty()) return false;
    
    int lastGroupId = undoStack_.back().groupId;
    return lastGroupId != 0;
}

void UndoManager::undo() {
    if (undoStack_.empty()) return;
    
    isUndoing_ = true;
    
    UndoAction action = undoStack_.back();
    undoStack_.pop_back();
    
    // Create inverse action for redo
    UndoAction inverse = createInverse(action);
    redoStack_.push_back(inverse);
    
    // Apply inverse
    applyInverse(action);
    
    isUndoing_ = false;
    
    if (onUndoRedo) {
        onUndoRedo(true);
    }
}

void UndoManager::undoGroup() {
    if (undoStack_.empty()) return;
    
    int groupId = undoStack_.back().groupId;
    if (groupId == 0) {
        undo();
        return;
    }
    
    isUndoing_ = true;
    
    while (!undoStack_.empty() && undoStack_.back().groupId == groupId) {
        undo();
    }
    
    isUndoing_ = false;
}

void UndoManager::redo() {
    if (redoStack_.empty()) return;
    
    isUndoing_ = true;
    
    UndoAction action = redoStack_.back();
    redoStack_.pop_back();
    
    // Push back to undo stack
    undoStack_.push_back(action);
    
    // Apply action
    applyAction(action);
    
    isUndoing_ = false;
    
    if (onUndoRedo) {
        onUndoRedo(false);
    }
}

void UndoManager::redoGroup() {
    if (redoStack_.empty()) return;
    
    int groupId = redoStack_.back().groupId;
    if (groupId == 0) {
        redo();
        return;
    }
    
    isUndoing_ = true;
    
    while (!redoStack_.empty() && redoStack_.back().groupId == groupId) {
        redo();
    }
    
    isUndoing_ = false;
}

void UndoManager::clear() {
    undoStack_.clear();
    redoStack_.clear();
    groupNames_.clear();
    currentGroupId_ = 0;
}

void UndoManager::clearRedo() {
    redoStack_.clear();
}

size_t UndoManager::getUndoStackSize() const {
    return undoStack_.size();
}

size_t UndoManager::getRedoStackSize() const {
    return redoStack_.size();
}

void UndoManager::setMaxUndoSize(int max) {
    maxUndoStack_ = max;
    
    while (static_cast<int>(undoStack_.size()) > maxUndoStack_) {
        undoStack_.erase(undoStack_.begin());
    }
}

int UndoManager::getMaxUndoSize() const {
    return maxUndoStack_;
}

UndoManager::UndoAction UndoManager::createInverse(const UndoAction& action) const {
    UndoAction inverse;
    inverse.groupId = action.groupId;
    
    switch (action.type) {
        case ActionType::Insert:
            inverse.type = ActionType::Delete;
            inverse.line = action.line;
            inverse.col = action.col;
            inverse.text = action.text;
            break;
            
        case ActionType::Delete:
            inverse.type = ActionType::Insert;
            inverse.line = action.line;
            inverse.col = action.col;
            inverse.text = action.text;
            break;
            
        case ActionType::Replace:
            inverse.type = ActionType::Replace;
            inverse.line = action.line;
            inverse.col = action.col;
            inverse.endLine = action.endLine;
            inverse.endCol = action.endCol;
            inverse.text = action.oldText;
            inverse.oldText = action.text;
            break;
    }
    
    return inverse;
}

void UndoManager::applyInverse(const UndoAction& action) {
    if (onAction) {
        switch (action.type) {
            case ActionType::Insert:
                // Inverse of insert is delete
                onAction(ActionType::Delete, action.line, action.col, action.text);
                break;
                
            case ActionType::Delete:
                // Inverse of delete is insert
                onAction(ActionType::Insert, action.line, action.col, action.text);
                break;
                
            case ActionType::Replace:
                // Inverse of replace is replace with old text
                onAction(ActionType::Replace, action.line, action.col, action.oldText);
                break;
        }
    }
}

void UndoManager::applyAction(const UndoAction& action) {
    if (onAction) {
        switch (action.type) {
            case ActionType::Insert:
                onAction(ActionType::Insert, action.line, action.col, action.text);
                break;
                
            case ActionType::Delete:
                onAction(ActionType::Delete, action.line, action.col, action.text);
                break;
                
            case ActionType::Replace:
                onAction(ActionType::Replace, action.line, action.col, action.text);
                break;
        }
    }
}

std::string UndoManager::getGroupName(int groupId) const {
    auto it = groupNames_.find(groupId);
    if (it != groupNames_.end()) {
        return it->second;
    }
    return "";
}

bool UndoManager::isUndoing() const {
    return isUndoing_;
}

void UndoManager::markClean() {
    cleanState_ = undoStack_.size();
}

bool UndoManager::isDirty() const {
    return undoStack_.size() != cleanState_;
}

} // namespace NexusForge::Editor
