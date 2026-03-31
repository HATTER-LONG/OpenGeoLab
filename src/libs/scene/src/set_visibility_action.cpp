/**
 * @file set_visibility_action.cpp
 * @brief SetVisibilityAction — batch set scene node visibility
 */

#include <opengeolab/scene/set_visibility_action.hpp>

#include <opengeolab/scene/scene_graph.hpp>

namespace OpenGeoLab::Scene {

SetVisibilityAction::SetVisibilityAction(SceneGraph& graph) : m_graph(graph) {}
SetVisibilityAction::~SetVisibilityAction() = default;

nlohmann::json SetVisibilityAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Batch-set visibility of scene nodes."},
        {"params",
         {{"nodes",
           {{"type", "array"},
            {"required", true},
            {"description", "Array of {nodeId: int, visible: bool} pairs."}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"updated", {{"type", "integer"}, {"description", "Nodes whose visibility changed."}}},
          {"skipped", {{"type", "integer"}, {"description", "Node IDs not found."}}}}}};
}

nlohmann::json SetVisibilityAction::execute(const nlohmann::json& param,
                                            const Core::ProgressCallback& progress) {
    const auto& nodes = param.value("nodes", nlohmann::json::array());
    int updated = 0;
    int skipped = 0;

    for(const auto& entry : nodes) {
        if(!entry.contains("nodeId")) {
            ++skipped;
            continue;
        }

        const auto node_id = entry.value("nodeId", static_cast<NodeId>(0));
        const auto visible = entry.value("visible", true);

        auto* node = m_graph.findNode(node_id);
        if(node == nullptr) {
            ++skipped;
            continue;
        }
        if(m_graph.setNodeVisible(node_id, visible)) {
            ++updated;
        }
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", "set_visibility"}, {"updated", updated}, {"skipped", skipped}};
}

} // namespace OpenGeoLab::Scene
