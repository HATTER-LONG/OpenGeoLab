/**
 * @file list_nodes_action.cpp
 * @brief ListNodesAction — query all scene nodes with visibility state
 */

#include <opengeolab/scene/list_nodes_action.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <functional>

namespace OpenGeoLab::Scene {

ListNodesAction::ListNodesAction(const SceneGraph& graph) : m_graph(graph) {}
ListNodesAction::~ListNodesAction() = default;

nlohmann::json ListNodesAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "List all scene nodes with visibility state."},
        {"params", nlohmann::json::object()},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"nodes",
           {{"type", "array"},
            {"description", "Array of {nodeId, name, visible, parentId} objects."}}}}}};
}

nlohmann::json ListNodesAction::execute(const nlohmann::json& /*param*/,
                                        const Core::ProgressCallback& progress) {
    auto lock = m_graph.readLock();
    nlohmann::json nodes = nlohmann::json::array();

    std::function<void(const SceneNode&)> collect = [&](const SceneNode& node) {
        nodes.push_back({{"nodeId", node.id()},
                         {"name", std::string(node.name())},
                         {"visible", node.isVisible()},
                         {"parentId", node.parent() ? node.parent()->id() : 0}});
        for(const auto& child : node.children()) {
            collect(*child);
        }
    };

    for(const auto& child : m_graph.root()->children()) {
        collect(*child);
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", "list_nodes"}, {"nodes", nodes}};
}

} // namespace OpenGeoLab::Scene
