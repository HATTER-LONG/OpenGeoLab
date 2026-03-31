/**
 * @file list_nodes_action.cpp
 * @brief ListNodesAction — query all scene nodes with visibility state
 */

#include <opengeolab/scene/list_nodes_action.hpp>
#include <opengeolab/scene/scene_graph.hpp>

namespace OpenGeoLab::Scene {

ListNodesAction::ListNodesAction(const SceneGraph& graph) : m_graph(graph) {}
ListNodesAction::~ListNodesAction() = default;

nlohmann::json ListNodesAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "List all scene nodes with visibility and source info."},
        {"params", nlohmann::json::object()},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"nodes",
           {{"type", "array"},
            {"description", "Array of {sourceType, sourceId, name, visible} objects."}}}}}};
}

nlohmann::json ListNodesAction::execute(const nlohmann::json& /*param*/,
                                        const Core::ProgressCallback& progress) {
    nlohmann::json nodes = nlohmann::json::array();

    m_graph.forEachNode([&](const SceneNode& node) {
        nodes.push_back({{"sourceType", std::string(node.sourceType())},
                         {"sourceId", node.sourceId()},
                         {"name", std::string(node.name())},
                         {"visible", node.isVisible()}});
    });

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", "list_nodes"}, {"nodes", nodes}};
}

} // namespace OpenGeoLab::Scene
