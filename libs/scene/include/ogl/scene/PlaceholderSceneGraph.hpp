/**
 * @file PlaceholderSceneGraph.hpp
 * @brief Placeholder scene graph types used to validate scene-layer data flow.
 */

#pragma once

#include <ogl/geometry/PlaceholderGeometryModel.hpp>
#include <ogl/scene/export.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace ogl::scene {

/**
 * @brief Placeholder scene node generated from the geometry placeholder model.
 */
struct OGL_SCENE_EXPORT PlaceholderSceneNode {
    std::string nodeId;
    std::string displayName;
    std::string renderPrimitive;
    int conceptualBodyIndex{0};
    bool selectable{true};

    [[nodiscard]] auto toJson() const -> nlohmann::json;
};

/**
 * @brief Lightweight scene graph that stabilizes IDs between geometry and render layers.
 */
class OGL_SCENE_EXPORT PlaceholderSceneGraph {
public:
    PlaceholderSceneGraph(std::string scene_id,
                          std::string model_name,
                          std::vector<PlaceholderSceneNode> nodes);

    [[nodiscard]] auto sceneId() const -> const std::string&;
    [[nodiscard]] auto modelName() const -> const std::string&;
    [[nodiscard]] auto nodes() const -> const std::vector<PlaceholderSceneNode>&;
    [[nodiscard]] auto summary() const -> std::string;
    [[nodiscard]] auto toJson() const -> nlohmann::json;

private:
    std::string m_sceneId;
    std::string m_modelName;
    std::vector<PlaceholderSceneNode> m_nodes;
};

/**
 * @brief Build a placeholder scene graph from the geometry-layer placeholder model.
 * @param geometry_model Placeholder geometry model.
 * @return Stable scene graph representation.
 */
OGL_SCENE_EXPORT auto
buildPlaceholderSceneGraph(const ogl::geometry::PlaceholderGeometryModel& geometry_model)
    -> PlaceholderSceneGraph;

} // namespace ogl::scene