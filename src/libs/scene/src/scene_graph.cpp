#include <opengeolab/scene/scene_graph.hpp>

#include <utility>

namespace OpenGeoLab::Scene {

namespace {

// NOLINTBEGIN(misc-no-recursion) — tree traversal is inherently recursive
template <typename Visitor> void visitAllNodes(SceneNode* node, const Visitor& visitor) {
    if(node == nullptr) {
        return;
    }

    visitor(node);
    for(const std::unique_ptr<SceneNode>& child : node->children()) {
        visitAllNodes(child.get(), visitor);
    }
}

[[nodiscard]] bool subtreeContainsNodeId(SceneNode* node, NodeId id) {
    bool found = false;
    visitAllNodes(node, [&](SceneNode* current) {
        if(current->id() == id) {
            found = true;
        }
    });
    return found;
}

} // namespace
// NOLINTEND(misc-no-recursion)

SceneGraph::SceneGraph() : m_root(std::make_unique<SceneNode>(0, "root")) {}

SceneGraph::~SceneGraph() = default;

NodeId SceneGraph::allocateNodeId() {
    std::unique_lock const lock(m_mutex);
    return m_nextNodeId++;
}

SceneNode* SceneGraph::root() const {
    std::shared_lock const lock(m_mutex);
    return m_root.get();
}

NodeId SceneGraph::addNode(std::string name, NodeId parent_id) {
    NodeId added_id = 0;
    {
        std::unique_lock const lock(m_mutex);
        SceneNode* parent = findInSubtree(m_root.get(), parent_id);
        if(parent == nullptr) {
            return 0;
        }

        added_id = m_nextNodeId++;
        SceneNode* added_node =
            parent->addChild(std::make_unique<SceneNode>(added_id, std::move(name)));
        if(added_node == nullptr) {
            return 0;
        }

        added_node->markDirty();
        ++m_version;
    }

    nodeAdded(added_id);
    return added_id;
}

bool SceneGraph::removeNode(NodeId id) {
    if(id == 0) {
        return false;
    }

    bool removed = false;
    bool selection_changed = false;
    bool hovered_removed = false;
    {
        std::unique_lock const lock(m_mutex);
        SceneNode* node = findInSubtree(m_root.get(), id);
        if(node == nullptr || node->parent() == nullptr) {
            return false;
        }

        visitAllNodes(node, [&](SceneNode* current) {
            selection_changed = selection_changed || current->isSelected();
        });
        if(m_hoveredNode.has_value()) {
            hovered_removed = subtreeContainsNodeId(node, *m_hoveredNode);
        }

        removed = node->parent()->removeChild(id) != nullptr;
        if(!removed) {
            return false;
        }

        if(hovered_removed) {
            m_hoveredNode.reset();
        }
        ++m_version;
    }

    nodeRemoved(id);
    if(selection_changed) {
        selectionChanged();
    }
    return true;
}

SceneNode* SceneGraph::findNode(NodeId id) const {
    std::shared_lock const lock(m_mutex);
    return findInSubtree(m_root.get(), id);
}

SceneNode* SceneGraph::findNodeBySource(std::string_view type, uint32_t src_id) const {
    std::shared_lock const lock(m_mutex);
    SceneNode* result = nullptr;
    std::function<void(SceneNode*)> search = [&](SceneNode* node) {
        if(result != nullptr) {
            return;
        }
        if(node->sourceType() == type && node->sourceId() == src_id) {
            result = node;
            return;
        }
        for(const auto& child : node->children()) {
            search(child.get());
        }
    };
    search(m_root.get());
    return result;
}

bool SceneGraph::setNodeVisible(NodeId id, bool visible) {
    {
        std::unique_lock const lock(m_mutex);
        SceneNode* node = findInSubtree(m_root.get(), id);
        if(node == nullptr || node->isVisible() == visible) {
            return false;
        }
        node->setVisible(visible);
        node->markDirty();
        ++m_version;
    }
    nodeUpdated(id);
    return true;
}

bool SceneGraph::setVisibleBySource(std::string_view source_type,
                                    uint32_t source_id,
                                    bool visible) {
    NodeId matched_id = 0;
    {
        std::unique_lock const lock(m_mutex);
        SceneNode* node = nullptr;
        std::function<void(SceneNode*)> search = [&](SceneNode* current) {
            if(node != nullptr) {
                return;
            }
            if(current->sourceType() == source_type && current->sourceId() == source_id) {
                node = current;
                return;
            }
            for(const auto& child : current->children()) {
                search(child.get());
            }
        };
        search(m_root.get());

        if(node == nullptr || node->isVisible() == visible) {
            return false;
        }
        matched_id = node->id();
        node->setVisible(visible);
        node->markDirty();
        ++m_version;
    }
    nodeUpdated(matched_id);
    return true;
}

bool SceneGraph::setNodeSource(NodeId id, std::string_view source_type, uint32_t source_id) {
    {
        std::unique_lock const lock(m_mutex);
        SceneNode* node = findInSubtree(m_root.get(), id);
        if(node == nullptr) {
            return false;
        }
        node->setSource(std::string{source_type}, source_id);
        node->markDirty();
        ++m_version;
    }
    nodeUpdated(id);
    return true;
}

bool SceneGraph::configureNode(NodeId id, const std::function<void(SceneNode&)>& fn) {
    {
        std::unique_lock const lock(m_mutex);
        SceneNode* node = findInSubtree(m_root.get(), id);
        if(node == nullptr) {
            return false;
        }
        fn(*node);
        node->markDirty();
        ++m_version;
    }
    nodeUpdated(id);
    return true;
}

void SceneGraph::traverseVisible(std::function<void(const SceneNode&)> visitor) const {
    std::shared_lock const lock(m_mutex);
    traverseVisibleImpl(m_root.get(), visitor);
}

void SceneGraph::forEachNode(std::function<void(const SceneNode&)> visitor) const {
    std::shared_lock const lock(m_mutex);
    std::function<void(const SceneNode&)> recurse = [&](const SceneNode& node) {
        visitor(node);
        for(const auto& child : node.children()) {
            recurse(*child);
        }
    };
    for(const auto& child : m_root->children()) {
        recurse(*child);
    }
}

void SceneGraph::traverseDirty(uint64_t since_version,
                               std::function<void(const SceneNode&)> visitor) const {
    std::shared_lock const lock(m_mutex);
    traverseDirtyImpl(m_root.get(), since_version, visitor);
}

std::vector<NodeId> SceneGraph::selectedNodes() const {
    std::vector<NodeId> selected;
    std::shared_lock const lock(m_mutex);
    visitAllNodes(m_root.get(), [&](SceneNode* node) {
        if(node->isSelected()) {
            selected.push_back(node->id());
        }
    });
    return selected;
}

void SceneGraph::selectNode(NodeId id, bool append) {
    std::vector<NodeId> updated_nodes;
    bool selection_changed = false;
    {
        std::unique_lock const lock(m_mutex);
        SceneNode* node = findInSubtree(m_root.get(), id);
        if(node == nullptr) {
            return;
        }

        if(!append) {
            visitAllNodes(m_root.get(), [&](SceneNode* current) {
                if(current != node && current->isSelected()) {
                    current->setSelected(false);
                    current->markDirty();
                    updated_nodes.push_back(current->id());
                    selection_changed = true;
                }
            });
        }

        if(!node->isSelected()) {
            node->setSelected(true);
            node->markDirty();
            updated_nodes.push_back(node->id());
            selection_changed = true;
        }

        if(selection_changed) {
            ++m_version;
        }
    }

    if(!selection_changed) {
        return;
    }

    for(const NodeId updated_id : updated_nodes) {
        nodeUpdated(updated_id);
    }
    selectionChanged();
}

void SceneGraph::deselectNode(NodeId id) {
    bool changed = false;
    {
        std::unique_lock const lock(m_mutex);
        SceneNode* node = findInSubtree(m_root.get(), id);
        if(node == nullptr || !node->isSelected()) {
            return;
        }

        node->setSelected(false);
        node->markDirty();
        ++m_version;
        changed = true;
    }

    if(changed) {
        nodeUpdated(id);
        selectionChanged();
    }
}

void SceneGraph::clearSelection() {
    std::vector<NodeId> updated_nodes;
    {
        std::unique_lock const lock(m_mutex);
        visitAllNodes(m_root.get(), [&](SceneNode* node) {
            if(!node->isSelected()) {
                return;
            }

            node->setSelected(false);
            node->markDirty();
            updated_nodes.push_back(node->id());
        });

        if(updated_nodes.empty()) {
            return;
        }

        ++m_version;
    }

    for(const NodeId updated_id : updated_nodes) {
        nodeUpdated(updated_id);
    }
    selectionChanged();
}

std::optional<NodeId> SceneGraph::hoveredNode() const {
    std::shared_lock const lock(m_mutex);
    return m_hoveredNode;
}

void SceneGraph::setHoveredNode(std::optional<NodeId> id) {
    std::vector<NodeId> updated_nodes;
    {
        std::unique_lock const lock(m_mutex);
        if(id == m_hoveredNode) {
            return;
        }

        if(id.has_value() && findInSubtree(m_root.get(), *id) == nullptr) {
            return;
        }

        if(m_hoveredNode.has_value()) {
            SceneNode* previous = findInSubtree(m_root.get(), *m_hoveredNode);
            if(previous != nullptr && previous->isHovered()) {
                previous->setHovered(false);
                previous->markDirty();
                updated_nodes.push_back(previous->id());
            }
        }

        m_hoveredNode = id;

        if(m_hoveredNode.has_value()) {
            SceneNode* current = findInSubtree(m_root.get(), *m_hoveredNode);
            if(current != nullptr && !current->isHovered()) {
                current->setHovered(true);
                current->markDirty();
                updated_nodes.push_back(current->id());
            }
        }

        if(updated_nodes.empty()) {
            return;
        }

        ++m_version;
    }

    for(const NodeId updated_id : updated_nodes) {
        nodeUpdated(updated_id);
    }
}

BoundingBox3D SceneGraph::sceneBounds() const {
    BoundingBox3D bounds;
    std::shared_lock const lock(m_mutex);
    traverseVisibleImpl(m_root.get(),
                        [&](const SceneNode& node) { bounds.expand(node.worldBounds()); });
    return bounds;
}

std::shared_lock<std::shared_mutex> SceneGraph::readLock() const {
    return std::shared_lock<std::shared_mutex>(m_mutex);
}

std::unique_lock<std::shared_mutex> SceneGraph::writeLock() {
    return std::unique_lock<std::shared_mutex>(m_mutex);
}

uint64_t SceneGraph::version() const {
    std::shared_lock const lock(m_mutex);
    return m_version;
}

SceneNode* SceneGraph::findInSubtree(SceneNode* node, NodeId id) const { // NOLINT
    if(node == nullptr) {
        return nullptr;
    }
    if(node->id() == id) {
        return node;
    }

    for(const std::unique_ptr<SceneNode>& child : node->children()) {
        if(SceneNode* found = findInSubtree(child.get(), id); found != nullptr) {
            return found;
        }
    }

    return nullptr;
}

void SceneGraph::traverseVisibleImpl(const SceneNode* node, // NOLINT
                                     const std::function<void(const SceneNode&)>& visitor) const {
    if(node == nullptr || !node->isVisible()) {
        return;
    }

    visitor(*node);
    for(const std::unique_ptr<SceneNode>& child : node->children()) {
        traverseVisibleImpl(child.get(), visitor);
    }
}

void SceneGraph::traverseDirtyImpl(const SceneNode* node, // NOLINT(misc-no-recursion)
                                   uint64_t since_version,
                                   const std::function<void(const SceneNode&)>& visitor) const {
    if(node == nullptr) {
        return;
    }

    if(node->version() > since_version) {
        visitor(*node);
    }

    for(const std::unique_ptr<SceneNode>& child : node->children()) {
        traverseDirtyImpl(child.get(), since_version, visitor);
    }
}

} // namespace OpenGeoLab::Scene
