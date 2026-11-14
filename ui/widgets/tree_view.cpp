// ui/widgets/tree_view.cpp
#include "tree_view.hpp"
#include <algorithm>

namespace NexusForge::UI {

void TreeNode::addChild(Ptr child) {
    if (!child) return;
    child->parent = this;
    children.push_back(std::move(child));
    data.hasChildren = !children.empty();
}

void TreeNode::removeChild(Ptr child) {
    if (!child) return;
    children.erase(
        std::remove(children.begin(), children.end(), child),
        children.end()
    );
    if (child->parent == this) child->parent = nullptr;
    data.hasChildren = !children.empty();
}

TreeNode::Ptr TreeNode::findChild(const std::string& id) {
    for (auto& child : children) {
        if (child->data.id == id) {
            return child;
        }
        auto found = child->findChild(id);
        if (found) return found;
    }
    return nullptr;
}

size_t TreeNode::getDepth() const {
    size_t depth = 0;
    const TreeNode* node = this;
    while (node->parent) {
        depth++;
        node = node->parent;
    }
    return depth;
}

// TreeView implementation
TreeView::TreeView() {
    style_.backgroundColor = GradientColor::fromHex(0x252538);
    style_.foregroundColor = GradientColor::fromHex(0xE8E8F0);
    setFocusPolicy(FocusPolicy::ClickFocus);
}

TreeView::~TreeView() = default;

void TreeView::setRootNode(TreeNode::Ptr root) {
    rootNode_ = std::move(root);
    rebuildVisibleNodes();
}

void TreeView::selectNode(TreeNode::Ptr node) {
    if (selectedNode_) {
        selectedNode_->data.selected = false;
    }
    selectedNode_ = node;
    if (selectedNode_) {
        selectedNode_->data.selected = true;
    }
    if (onNodeSelected) {
        onNodeSelected(node);
    }
    markDirty();
}

void TreeView::expandNode(TreeNode::Ptr node) {
    if (!node) return;
    node->data.expanded = true;
    rebuildVisibleNodes();
    if (onNodeExpanded) {
        onNodeExpanded(node);
    }
}

void TreeView::collapseNode(TreeNode::Ptr node) {
    if (!node) return;
    node->data.expanded = false;
    rebuildVisibleNodes();
    if (onNodeCollapsed) {
        onNodeCollapsed(node);
    }
}

void TreeView::toggleNode(TreeNode::Ptr node) {
    if (!node) return;
    if (node->data.expanded) {
        collapseNode(node);
    } else {
        expandNode(node);
    }
}

void TreeView::expandAll() {
    if (rootNode_) {
        std::vector<TreeNode::Ptr> stack = {rootNode_};
        while (!stack.empty()) {
            auto node = stack.back();
            stack.pop_back();
            node->data.expanded = true;
            for (auto& child : node->children) {
                stack.push_back(child);
            }
        }
    }
    rebuildVisibleNodes();
}

void TreeView::collapseAll() {
    if (rootNode_) {
        std::vector<TreeNode::Ptr> stack = {rootNode_};
        while (!stack.empty()) {
            auto node = stack.back();
            stack.pop_back();
            node->data.expanded = false;
            for (auto& child : node->children) {
                stack.push_back(child);
            }
        }
    }
    rebuildVisibleNodes();
}

void TreeView::render(RenderEngine& renderer) {
    if (!isVisibleInHierarchy()) return;

    float x = bounds_.x;
    float y = bounds_.y - scrollOffset_;
    
    // Draw rows
    for (TreeNode* node : visibleNodes_) {
        Rect rowRect(x, y, bounds_.width, rowHeight_);
        
        // Draw selection background
        if (node->data.selected) {
            renderer.drawRect(rowRect, Color(0.3f, 0.4f, 0.6f, 0.5f));
        }
        
        // Draw hover background
        // Would need to track hover state
        
        // Draw expand/collapse icon
        if (node->data.hasChildren) {
            float iconX = x + node->getDepth() * indent_ + 4;
            float iconY = y + rowHeight_ / 2;
            std::string icon = node->data.expanded ? "▼" : "▶";
            renderer.drawText(icon, iconX, iconY + style_.fontSize / 2,
                              Color(0.6f, 0.6f, 0.7f, 1.0f),
                              style_.fontFamily, 10);
        }
        
        // Draw label
        float textX = x + node->getDepth() * indent_ + 20;
        float textY = y + rowHeight_ / 2 + style_.fontSize / 3;
        renderer.drawText(node->data.label, textX, textY,
                          node->data.selected ? Color::White() : Color(0.8f, 0.8f, 0.9f, 1.0f),
                          style_.fontFamily, style_.fontSize);
        
        y += rowHeight_;
    }
}

Vec2 TreeView::measure(float availableWidth, float availableHeight) {
    return Vec2(availableWidth, std::max(100.0f, constraints_.minHeight));
}

bool TreeView::handleEvent(Core::Event& event) {
    return Widget::handleEvent(event);
}

bool TreeView::onMouseDown(const Core::MouseButtonPressEvent& event) {
    if (!rootNode_) return false;
    TreeNode* node = getNodeAtPosition(event.x, event.y);
    if (node) {
        TreeNode::Ptr nodePtr;
        try {
            nodePtr = node->shared_from_this();
        } catch (const std::bad_weak_ptr&) {
            nodePtr = (node == rootNode_.get()) ? rootNode_ : rootNode_->findChild(node->data.id);
        }
        if (nodePtr) {
            if (node->data.hasChildren) {
                // Check if clicked on expand icon
                float indentX = bounds_.x + node->getDepth() * indent_ + 4;
                if (event.x >= indentX && event.x <= indentX + 16) {
                    toggleNode(nodePtr);
                    return true;
                }
            }
            selectNode(nodePtr);
            return true;
        }
    }
    return false;
}

bool TreeView::onScroll(const Core::ScrollEvent& event) {
    int delta = event.yOffset > 0 ? -30 : 30;
    int maxScroll = std::max(0, static_cast<int>(visibleNodes_.size() * rowHeight_) -
                                static_cast<int>(bounds_.height));
    scrollOffset_ = std::clamp(scrollOffset_ + delta, 0, maxScroll);
    return true;
}

bool TreeView::onKeyDown(const Core::KeyPressEvent& event) {
    if (!selectedNode_) return false;

    auto findVisibleIndex = [&]() -> int {
        for (size_t i = 0; i < visibleNodes_.size(); ++i) {
            if (visibleNodes_[i] == selectedNode_.get()) return static_cast<int>(i);
        }
        return -1;
    };

    switch (event.keyCode) {
        case 38: { // Up arrow
            int idx = findVisibleIndex();
            if (idx > 0) {
                selectNode(visibleNodes_[idx - 1]->shared_from_this());
                return true;
            }
            break;
        }
        case 40: { // Down arrow
            int idx = findVisibleIndex();
            if (idx >= 0 && idx + 1 < static_cast<int>(visibleNodes_.size())) {
                selectNode(visibleNodes_[idx + 1]->shared_from_this());
                return true;
            }
            break;
        }
        case 37: // Left arrow
            if (selectedNode_->data.expanded) {
                collapseNode(selectedNode_);
            } else if (selectedNode_->parent) {
                selectNode(selectedNode_->parent->shared_from_this());
            }
            return true;
        case 39: // Right arrow
            if (selectedNode_->data.hasChildren && !selectedNode_->data.expanded) {
                expandNode(selectedNode_);
                return true;
            }
            break;
        case 13: // Enter
            if (onNodeDoubleClicked) {
                onNodeDoubleClicked(selectedNode_);
                return true;
            }
            break;
    }

    return false;
}

void TreeView::rebuildVisibleNodes() {
    visibleNodes_.clear();
    if (rootNode_) {
        addVisibleNodes(rootNode_, 0);
    }
}

void TreeView::addVisibleNodes(TreeNode::Ptr node, int depth) {
    visibleNodes_.push_back(node.get());
    
    if (node->data.expanded) {
        for (auto& child : node->children) {
            addVisibleNodes(child, depth + 1);
        }
    }
}

TreeNode* TreeView::getNodeAtPosition(float x, float y) {
    float rowY = bounds_.y - scrollOffset_;
    for (TreeNode* node : visibleNodes_) {
        if (y >= rowY && y < rowY + rowHeight_) {
            return node;
        }
        rowY += rowHeight_;
    }
    return nullptr;
}

void TreeView::scrollToNode(TreeNode::Ptr node) {
    // Would scroll to make node visible
}

} // namespace NexusForge::UI
