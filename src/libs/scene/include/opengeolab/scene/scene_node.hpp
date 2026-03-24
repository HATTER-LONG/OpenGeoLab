/**
 * @file scene_node.hpp
 * @brief Declares hierarchical scene node data consumed by render systems.
 */
#pragma once

#include <opengeolab/scene/bounding_box.hpp>
#include <opengeolab/scene/render_mesh_data.hpp>

#include <glm/mat4x4.hpp>

#include <string>
#include <vector>

namespace OpenGeoLab::Scene {

/**
 * @brief Semantic entity category represented by a scene node.
 */
enum class EntityType {
    Body,      /**< Solid or top-level body entity. */
    Face,      /**< Surface face entity. */
    Edge,      /**< Edge entity. */
    Vertex,    /**< Vertex entity. */
    MeshRegion /**< Derived mesh region entity. */
};

/**
 * @brief Scene graph node with transform, bounds, meshes, and children.
 */
struct SceneNode {
    int id = 0;                         /**< Stable node identifier. */
    std::string name;                   /**< Human-readable node name. */
    EntityType type = EntityType::Body; /**< Semantic entity type. */
    glm::mat4 transform{1.0F};          /**< Local transform. */
    BoundingBox bounds;                 /**< Local-space bounding box. */
    std::vector<RenderMeshData> meshes; /**< Render meshes owned by the node. */
    std::vector<SceneNode> children;    /**< Child nodes. */
    bool visible = true;                /**< Visibility flag. */
    bool selected = false;              /**< Selection flag. */
};

} // namespace OpenGeoLab::Scene
