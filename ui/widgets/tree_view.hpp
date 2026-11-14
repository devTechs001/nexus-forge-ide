// ui/widgets/tree_view.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace NexusForge::UI {

// Tree node data
struct TreeNodeData {
    std::string id;
    std::string label;
    std::string icon;
    bool expanded = false;
    bool selected = false;
    bool hasChildren = false;
    void* userData = nullptr;
};

// Tree node
class TreeNode : public std::enable_shared_from_this<TreeNode> {
public:
    using Ptr = std::shared_ptr<TreeNode>;
    
    TreeNodeData data;
    std::vector<Ptr> children;
    TreeNode* parent = nullptr;
    
    void addChild(Ptr child);
    void removeChild(Ptr child);
    Ptr findChild(const std::string& id);
    size_t getDepth() const;
};

// Tree View widget
class TreeView : public Widget {
public:
    using Ptr = std::shared_ptr<TreeView>;

    TreeView();
    ~TreeView() override;

    // Data
    void setRootNode(TreeNode::Ptr root);
    TreeNode::Ptr getRootNode() const { return rootNode_; }
    TreeNode::Ptr getSelectedNode() const { return selectedNode_; }
    
    // Selection
    void selectNode(TreeNode::Ptr node);
    void expandNode(TreeNode::Ptr node);
    void collapseNode(TreeNode::Ptr node);
    void toggleNode(TreeNode::Ptr node);
    void expandAll();
    void collapseAll();
    
    // Indentation
    void setIndent(float indent) { indent_ = indent; }
    float getIndent() const { return indent_; }
    
    // Events
    std::function<void(TreeNode::Ptr)> onNodeSelected;
    std::function<void(TreeNode::Ptr)> onNodeExpanded;
    std::function<void(TreeNode::Ptr)> onNodeCollapsed;
    std::function<void(TreeNode::Ptr)> onNodeDoubleClicked;
    
    // Widget overrides
    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;
    bool handleEvent(Core::Event& event) override;
    bool onMouseDown(const Core::MouseButtonPressEvent& event) override;
    bool onScroll(const Core::ScrollEvent& event) override;
    bool onKeyDown(const Core::KeyPressEvent& event) override;

private:
    TreeNode::Ptr rootNode_;
    TreeNode::Ptr selectedNode_;
    float indent_ = 20;
    float rowHeight_ = 28;
    int scrollOffset_ = 0;
    
    std::vector<TreeNode*> visibleNodes_;
    
    void rebuildVisibleNodes();
    void addVisibleNodes(TreeNode::Ptr node, int depth);
    TreeNode* getNodeAtPosition(float x, float y);
    void scrollToNode(TreeNode::Ptr node);
};

} // namespace NexusForge::UI
