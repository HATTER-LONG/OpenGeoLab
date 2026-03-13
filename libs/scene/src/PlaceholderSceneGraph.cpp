#include <ogl/scene/PlaceholderSceneGraph.hpp>

#include <sstream>
#include <utility>

namespace {

auto buildNodeId(const std::string& model_name, int body_index) -> std::string {
    std::ostringstream stream;
    stream << model_name << "::body_" << body_index;
    return stream.str();
}

} // namespace

namespace ogl::scene {

auto PlaceholderSceneNode::toJson() const -> nlohmann::json {
    return {{"nodeId", nodeId},
            {"displayName", displayName},
            {"renderPrimitive", renderPrimitive},
            {"conceptualBodyIndex", conceptualBodyIndex},
            {"selectable", selectable}};
}

PlaceholderSceneGraph::PlaceholderSceneGraph(std::string scene_id, std::string model_name,
                                             std::vector<PlaceholderSceneNode> nodes)
    : m_sceneId(std::move(scene_id)),
      m_modelName(std::move(model_name)),
      m_nodes(std::move(nodes)) {}

auto PlaceholderSceneGraph::sceneId() const -> const std::string& { return m_sceneId; }

auto PlaceholderSceneGraph::modelName() const -> const std::string& { return m_modelName; }

auto PlaceholderSceneGraph::nodes() const -> const std::vector<PlaceholderSceneNode>& {
    return m_nodes;
}

auto PlaceholderSceneGraph::summary() const -> std::string {
    std::ostringstream stream;
    stream << "Placeholder scene graph '" << m_sceneId << "' stabilizes " << m_nodes.size()
           << " selectable nodes for geometry model '" << m_modelName << "'.";
    return stream.str();
}

auto PlaceholderSceneGraph::toJson() const -> nlohmann::json {
    nlohmann::json nodes_json = nlohmann::json::array();
    for(const auto& node : m_nodes) {
        nodes_json.push_back(node.toJson());
    }

    return {{"sceneId", m_sceneId},
            {"modelName", m_modelName},
            {"nodeCount", m_nodes.size()},
            {"nodes", std::move(nodes_json)},
            {"summary", summary()}};
}

auto buildPlaceholderSceneGraph(const ogl::geometry::PlaceholderGeometryModel& geometry_model)
    -> PlaceholderSceneGraph {
    std::vector<PlaceholderSceneNode> nodes;
    nodes.reserve(static_cast<std::size_t>(geometry_model.bodyCount()));

    for(int body_index = 1; body_index <= geometry_model.bodyCount(); ++body_index) {
        nodes.push_back({.nodeId = buildNodeId(geometry_model.modelName(), body_index),
                         .displayName = geometry_model.modelName() + " Body " +
                                        std::to_string(body_index),
                         .renderPrimitive = body_index % 2 == 0 ? "wire-overlay" : "solid-body",
                         .conceptualBodyIndex = body_index,
                         .selectable = true});
    }

    return PlaceholderSceneGraph(geometry_model.modelName() + "::scene", geometry_model.modelName(),
                                 std::move(nodes));
}

} // namespace ogl::scene