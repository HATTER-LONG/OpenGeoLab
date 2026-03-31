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
         {{"type",
           {{"type", "string"},
            {"required", true},
            {"description", "Source type: \"geometry\", \"mesh\", or \"node\" (internal)."}}},
          {"nodes",
           {{"type", "array"},
            {"required", true},
            {"description",
             "Array of {id: int, visible: bool} pairs. "
             "\"id\" is interpreted according to \"type\": "
             "shapeId for \"geometry\", meshId for \"mesh\", nodeId for \"node\"."}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"updated",
           {{"type", "integer"}, {"description", "Nodes whose visibility actually changed."}}},
          {"skipped",
           {{"type", "integer"},
            {"description", "Entries skipped (malformed or source not found)."}}}}}};
}

nlohmann::json SetVisibilityAction::execute(const nlohmann::json& param,
                                            const Core::ProgressCallback& progress) {
    const auto type = param.value("type", std::string{});
    const auto& nodes = param.value("nodes", nlohmann::json::array());
    int updated = 0;
    int skipped = 0;

    for(const auto& entry : nodes) {
        if(!entry.contains("id")) {
            ++skipped;
            continue;
        }

        const auto id = entry.value("id", static_cast<uint32_t>(0));
        const auto visible = entry.value("visible", true);

        bool changed = false;
        if(type == "node") {
            changed = m_graph.setNodeVisible(static_cast<NodeId>(id), visible);
        } else {
            changed = m_graph.setVisibleBySource(type, id, visible);
        }

        if(changed) {
            ++updated;
        } else {
            ++skipped;
        }
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", "set_visibility"}, {"updated", updated}, {"skipped", skipped}};
}

} // namespace OpenGeoLab::Scene
