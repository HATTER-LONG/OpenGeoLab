/**
 * @file scene_graph.hpp
 * @brief SceneGraph — root container for the scene node tree
 *
 * SceneGraph owns the root node and provides CRUD operations,
 * selection management, traversal helpers, and thread-safe locking.
 * Signals are emitted for node add/remove/update and selection change.
 */

#pragma once

#include <opengeolab/scene/bounding_box3d.hpp>
#include <opengeolab/scene/scene_export.hpp>
#include <opengeolab/scene/scene_node.hpp>

#include <kangaroo/util/signal.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Scene {

/**
 * @brief Root container for a scene node tree.
 *
 * Owns a root SceneNode (id=0). Provides node creation, lookup,
 * removal, selection, hover, traversal, and thread-safe locking.
 */
class OPENGEOLAB_SCENE_EXPORT SceneGraph final {
public:
    SceneGraph();
    ~SceneGraph();

    SceneGraph(const SceneGraph&) = delete;
    SceneGraph& operator=(const SceneGraph&) = delete;
    SceneGraph(SceneGraph&&) = delete;
    SceneGraph& operator=(SceneGraph&&) = delete;

    /** @brief Allocate a new unique NodeId. */
    NodeId allocateNodeId();

    /** @brief Root node (always exists, id=0). */
    [[nodiscard]] SceneNode* root() const;

    /**
     * @brief Create a new node and add it as a child of parentId.
     * @param name Human-readable name.
     * @param parentId Parent node id (0 = root).
     * @return Pointer to new node, or nullptr if parentId not found.
     */
    SceneNode* addNode(std::string name, NodeId parentId = 0);

    /**
     * @brief Remove a node and all its descendants.
     * @return true if found and removed.
     */
    bool removeNode(NodeId id);

    /**
     * @brief Find a node by id (searches entire tree).
     * @return Pointer to node, or nullptr if not found.
     */
    [[nodiscard]] SceneNode* findNode(NodeId id) const;

    /**
     * @brief Visit all visible nodes (skips invisible subtrees).
     */
    void traverseVisible(std::function<void(const SceneNode&)> visitor) const;

    /**
     * @brief Visit nodes whose version > sinceVersion.
     */
    void traverseDirty(uint64_t sinceVersion, std::function<void(const SceneNode&)> visitor) const;

    /** @brief Get IDs of all selected nodes. */
    [[nodiscard]] std::vector<NodeId> selectedNodes() const;

    /**
     * @brief Select a node.
     * @param id Node to select.
     * @param append If false, clears existing selection first.
     */
    void selectNode(NodeId id, bool append = false);

    /** @brief Deselect a single node. */
    void deselectNode(NodeId id);

    /** @brief Clear all selection. */
    void clearSelection();

    /** @brief Currently hovered node id. */
    [[nodiscard]] std::optional<NodeId> hoveredNode() const;

    /** @brief Set hovered node (nullopt = none). */
    void setHoveredNode(std::optional<NodeId> id);

    /** @brief Compute scene-wide AABB from all visible nodes. */
    [[nodiscard]] BoundingBox3D sceneBounds() const;

    /** @brief Acquire shared (read) lock. */
    [[nodiscard]] std::shared_lock<std::shared_mutex> readLock() const;

    /** @brief Acquire exclusive (write) lock. */
    [[nodiscard]] std::unique_lock<std::shared_mutex> writeLock();

    /** @brief Global scene version. */
    [[nodiscard]] uint64_t version() const;

    Kangaroo::Util::Signal<NodeId> nodeAdded;   /**< Emitted after addNode. */
    Kangaroo::Util::Signal<NodeId> nodeRemoved; /**< Emitted after removeNode. */
    Kangaroo::Util::Signal<NodeId> nodeUpdated; /**< Emitted when a node changes. */
    Kangaroo::Util::Signal<> selectionChanged;  /**< Emitted on any selection change. */

private:
    /** @brief Recursive helper to find a node in a subtree. */
    SceneNode* findInSubtree(SceneNode* node, NodeId id) const;

    /** @brief Recursive helper for visible traversal. */
    void traverseVisibleImpl(const SceneNode* node,
                             const std::function<void(const SceneNode&)>& visitor) const;

    /** @brief Recursive helper for dirty traversal. */
    void traverseDirtyImpl(const SceneNode* node,
                           uint64_t sinceVersion,
                           const std::function<void(const SceneNode&)>& visitor) const;

    std::unique_ptr<SceneNode> m_root;
    uint32_t m_nextNodeId{1};
    uint64_t m_version{0};
    std::optional<NodeId> m_hoveredNode;
    mutable std::shared_mutex m_mutex;
};

} // namespace OpenGeoLab::Scene
