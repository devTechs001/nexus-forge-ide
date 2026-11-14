// ui/widgets/tree_view.cpp
#include "tree_view.hpp"

namespace NexusForge::UI {

void TreeNode::addChild(Ptr child) {
    child->parent = this;
    children.push_back(child);
    data.hasChildren = !children.empty();
}

void TreeNode::removeChild(Ptr child) {
    children.erase(
        std::remove(children.begin(), children.end(), child),
        children.end()
    );
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
    node->data.expanded = true;
    rebuildVisibleNodes();
    if (onNodeExpanded) {
        onNodeExpanded(node);
    }
}

void TreeView::collapseNode(TreeNode::Ptr node) {
    node->data.expanded = false;
    rebuildVisibleNodes();
    if (onNodeCollapsed) {
        onNodeCollapsed(node);
    }
}

void TreeView::toggleNode(TreeNode::Ptr node) {
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
    TreeNode* node = getNodeAtPosition(event.x, event.y);
    if (node) {
        auto nodePtr = rootNode_->findChild(node->data.id);
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
        }
    }
    return true;
}

bool TreeView::onScroll(const Core::ScrollEvent& event) {
    int delta = event.yOffset > 0 ? -30 : 30;
    scrollOffset_ += delta;
    scrollOffset_ = std::max(0, scrollOffset_);
    scrollOffset_ = std::min(scrollOffset_, 
                              static_cast<int>(visibleNodes_.size() * rowHeight_) - 
                              static_cast<int>(bounds_.height));
    return true;
}

bool TreeView::onKeyDown(const Core::KeyPressEvent& event) {
    if (!selectedNode_) return false;
    
    switch (event.keyCode) {
        case 38: // Up arrow
            // Select previous visible node
            break;
        case 40: // Down arrow
            // Select next visible node
            break;
        case 37: // Left arrow
            if (selectedNode_->data.expanded) {
                collapseNode(selectedNode_);
            } else if (selectedNode_->parent) {
                selectNode(std::shared_ptr<TreeNode>(selectedNode_->parent, [](TreeNode*){}));
            }
            break;
        case 39: // Right arrow
            if (selectedNode_->data.hasChildren && !selectedNode_->data.expanded) {
                expandNode(selectedNode_);
            }
            break;
        case 13: // Enter
            if (onNodeDoubleClicked) {
                onNodeDoubleClicked(selectedNode_);
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
