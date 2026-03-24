/**
 * @file scene_graph.hpp
 * @brief Declares the SceneGraph tree container.
 */
#pragma once

#include <opengeolab/scene/bounding_box.hpp>
#include <opengeolab/scene/scene_export.hpp>
#include <opengeolab/scene/scene_node.hpp>

#include <functional>

namespace OpenGeoLab::Scene {

/**
 * @brief Owns a mutable tree of scene nodes rooted at an undeletable root node.
 */
class OPENGEOLAB_SCENE_EXPORT SceneGraph {
public:
    /**
     * @brief Constructs a scene graph with a root node whose id is zero.
     */
    SceneGraph();

    /**
     * @brief Returns the mutable root node.
     */
    [[nodiscard]] SceneNode& root();

    /**
     * @brief Returns the immutable root node.
     */
    [[nodiscard]] const SceneNode& root() const;

    /**
     * @brief Finds a node by identifier.
     * @param id Node identifier to search for.
     * @return Pointer to the node, or nullptr when not found.
     */
    [[nodiscard]] SceneNode* findById(int id);

    /**
     * @brief Finds a node by identifier.
     * @param id Node identifier to search for.
     * @return Pointer to the node, or nullptr when not found.
     */
    [[nodiscard]] const SceneNode* findById(int id) const;

    /**
     * @brief Adds a node below a parent node.
     * @param node Node value to insert.
     * @param parentId Parent identifier, defaulting to the root.
     * @return Assigned node id, or zero when the parent does not exist.
     */
    int addNode(SceneNode node, int parentId = 0);

    /**
     * @brief Removes a node and its entire subtree.
     * @param id Identifier of the node to remove.
     * @return True when a node was removed.
     */
    bool removeNode(int id);

    /**
     * @brief Computes merged bounds across every valid node bound in the tree.
     */
    [[nodiscard]] BoundingBox worldBounds() const;

    std::function<void()> onChanged; /**< Optional callback invoked after successful mutations. */

private:
    SceneNode root_;
    int nextId_ = 1;
};

} // namespace OpenGeoLab::Scene
