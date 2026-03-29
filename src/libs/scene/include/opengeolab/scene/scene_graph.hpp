/**
 * @file scene_graph.hpp
 * @brief Thread-safe scene graph container with changeset-based incremental updates.
 */

#pragma once

#include <opengeolab/scene/bounding_box.hpp>
#include <opengeolab/scene/scene_export.hpp>
#include <opengeolab/scene/scene_node.hpp>

#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Scene {

/// Thread-safe flat scene graph with incremental changeset tracking.
///
/// Writers (main thread) use addNode / updateVisual / removeNode which
/// record changes in a pending changeset.  The render thread calls
/// consumeChangeset() during synchronize() to drain pending changes.
///
/// Thread safety:
/// - Write operations acquire a unique_lock.
/// - Read operations acquire a shared_lock.
/// - consumeChangeset() acquires a unique_lock (swap-and-clear).
class OPENGEOLAB_SCENE_EXPORT SceneGraph {
public:
    /// Incremental changeset consumed by the render thread.
    struct Changeset {
        std::vector<std::string> added;   ///< Node IDs added since last consume.
        std::vector<std::string> updated; ///< Node IDs whose visual data changed.
        std::vector<std::string> removed; ///< Node IDs removed since last consume.

        [[nodiscard]] bool empty() const {
            return added.empty() && updated.empty() && removed.empty();
        }
    };

    // ── Write operations (main thread, unique_lock) ─────────────────────

    /// Add a new node.  Overwrites if a node with the same ID already exists.
    void addNode(SceneNode node);

    /// Update the visual data and bounds of an existing node.
    void updateVisual(const std::string& id, Core::VisualData visual);

    /// Remove a node by ID.  No-op if the node does not exist.
    void removeNode(const std::string& id);

    /// Set the visibility flag of an existing node.
    /// Records the node as updated in the pending changeset.
    void setNodeVisibility(const std::string& id, bool visible);

    // ── Read operations (any thread, shared_lock) ───────────────────────

    /// Look up a node by ID.  Returns nullptr if not found.
    [[nodiscard]] const SceneNode* findNode(const std::string& id) const;

    /// Return all current node IDs.
    [[nodiscard]] std::vector<std::string> allNodeIds() const;

    /// Compute the bounding box that encloses all visible nodes.
    [[nodiscard]] BoundingBox sceneBounds() const;

    /// True if there are pending changes not yet consumed.
    [[nodiscard]] bool hasChanges() const;

    // ── Changeset consumption (render thread, unique_lock) ──────────────

    /// Atomically drain the pending changeset (swap-and-clear).
    [[nodiscard]] Changeset consumeChangeset();

private:
    mutable std::shared_mutex m_mutex;
    std::unordered_map<std::string, SceneNode> m_nodes;
    Changeset m_pendingChanges;
};

} // namespace OpenGeoLab::Scene
