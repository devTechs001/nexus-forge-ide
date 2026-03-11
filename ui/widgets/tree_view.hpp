// ui/widgets/tree_view.hpp
#pragma once

#include "../framework/widget_system.hpp"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace NexusForge::UI {

// Tree node
struct TreeNode {
    std::string id;
    std::string text;
    std::string icon;
    std::string expandedIcon;
    std::string leafIcon;
    bool expandable = true;
    bool expanded = false;
    bool selectable = true;
    bool selected = false;
    int level = 0;
    void* data = nullptr;
    std::vector<std::shared_ptr<TreeNode>> children;

    TreeNode* getParent() const { return parent; }
    size_t getChildCount() const { return children.size(); }
    TreeNode* getChild(size_t index) const { return children[index].get(); }

    TreeNode* parent = nullptr;
};

// Tree view widget
class TreeView : public Widget {
public:
    using Ptr = std::shared_ptr<TreeView>;
    using NodePtr = std::shared_ptr<TreeNode>;

    TreeView();
    ~TreeView() override;

    // Root
    void setRootNode(NodePtr root);
    TreeNode* getRootNode() { return rootNode_.get(); }
    const TreeNode* getRootNode() const { return rootNode_.get(); }

    // Selection
    void setSelectedNode(TreeNode* node);
    TreeNode* getSelectedNode() const { return selectedNode_; }
    void clearSelection();
    void setMultiSelect(bool enabled) { multiSelect_ = enabled; }
    bool isMultiSelect() const { return multiSelect_; }

    // Expansion
    void expandNode(TreeNode* node);
    void collapseNode(TreeNode* node);
    void toggleNode(TreeNode* node);
    void expandAll();
    void collapseAll();
    void expandToNode(TreeNode* node);

    // Node management
    NodePtr addNode(TreeNode* parent, const std::string& text, void* data = nullptr);
    void removeNode(TreeNode* node);
    void clearNodes();

    // Find
    TreeNode* findNode(const std::string& id) const;
    TreeNode* findNodeByData(void* data) const;
    std::vector<TreeNode*> findNodesByText(const std::string& text) const;

    // Hit testing
    TreeNode* getNodeAtPosition(float x, float y) const;
    bool isExpandButtonAtPosition(float x, float y, TreeNode** node) const;

    // Appearance
    void setIndent(float indent) { indent_ = indent; }
    float getIndent() const { return indent_; }
    void setItemHeight(float height) { itemHeight_ = height; }
    float getItemHeight() const { return itemHeight_; }
    void setShowLines(bool show) { showLines_ = show; }
    void setShowIcons(bool show) { showIcons_ = show; }

    // Callbacks
    std::function<void(TreeNode*)> onNodeSelected;
    std::function<void(TreeNode*)> onNodeExpanded;
    std::function<void(TreeNode*)> onNodeCollapsed;
    std::function<void(TreeNode*)> onNodeDoubleClicked;
    std::function<void(TreeNode*)> onNodeContextMenu;

    // Rendering
    void render(RenderEngine& renderer) override;
    Vec2 measure(float availableWidth, float availableHeight) override;

    // Events
    bool onMouseDown(const Core::MouseButtonPressEvent& event) override;
    bool onMouseMove(const Core::MouseMoveEvent& event) override;
    bool onScroll(const Core::ScrollEvent& event) override;
    bool onKeyDown(const Core::KeyPressEvent& event) override;

protected:
    void updateVisibleNodes();
    void renderNode(RenderEngine& renderer, TreeNode* node, float x, float y);
    void renderExpandButton(RenderEngine& renderer, TreeNode* node, float x, float y);
    void renderNodeIcon(RenderEngine& renderer, TreeNode* node, float x, float y);
    void renderNodeText(RenderEngine& renderer, TreeNode* node, float x, float y);
    void renderLines(RenderEngine& renderer, TreeNode* node, float x, float y);

    NodePtr rootNode_;
    TreeNode* selectedNode_ = nullptr;
    TreeNode* hoveredNode_ = nullptr;

    std::vector<TreeNode*> visibleNodes_;
    std::unordered_map<TreeNode*, Rect> nodeBounds_;

    float indent_ = 20;
    float itemHeight_ = 24;
    bool showLines_ = true;
    bool showIcons_ = true;
    bool multiSelect_ = false;

    float scrollOffset_ = 0;
};

} // namespace NexusForge::UI
