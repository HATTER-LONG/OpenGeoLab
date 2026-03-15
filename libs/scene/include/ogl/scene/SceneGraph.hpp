/**
 * @file SceneGraph.hpp
 * @brief Scene graph types used to validate scene-layer data flow.
 */

#pragma once

#include <ogl/geometry/GeometryModel.hpp>
#include <ogl/scene/export.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace OGL::Scene {

/**
 * @brief Scene node generated from the geometry model.
 */
struct OGL_SCENE_EXPORT SceneNode {
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
class OGL_SCENE_EXPORT SceneGraph {
public:
    SceneGraph(std::string scene_id, std::string model_name, std::vector<SceneNode> nodes);

    [[nodiscard]] auto sceneId() const -> const std::string&;
    [[nodiscard]] auto modelName() const -> const std::string&;
    [[nodiscard]] auto nodes() const -> const std::vector<SceneNode>&;
    [[nodiscard]] auto summary() const -> std::string;
    [[nodiscard]] auto toJson() const -> nlohmann::json;

private:
    std::string m_sceneId;
    std::string m_modelName;
    std::vector<SceneNode> m_nodes;
};

/**
 * @brief Build a scene graph from the geometry-layer model.
 * @param geometry_model Geometry model.
 * @return Stable scene graph representation.
 */
OGL_SCENE_EXPORT auto buildSceneGraph(const OGL::Geometry::GeometryModel& geometry_model)
    -> SceneGraph;

} // namespace OGL::Scene
