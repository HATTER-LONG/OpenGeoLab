/**
 * @file clear_mesh_action.cpp
 * @brief ClearMeshAction implementation
 */

#include "clear_mesh_action.hpp"

#include <opengeolab/mesh/mesh_store.hpp>

namespace OpenGeoLab::Mesh {

ClearMeshAction::ClearMeshAction(MeshStore& mesh_store) : m_meshStore(mesh_store) {}

ClearMeshAction::~ClearMeshAction() = default;

nlohmann::json ClearMeshAction::describe() const {
    return {
        {"name", ACTION_NAME},
        {"description", "Remove one stored mesh by shapeId or clear the whole mesh store."},
        {"params",
         {{"shapeId",
           {{"type", "integer"},
            {"required", false},
            {"description", "Optional shape identifier to clear only one mesh."}}}}},
        {"returns",
         {{"ok",
           {{"type", "boolean"}, {"description", "true when the action completes successfully."}}},
          {"action", {{"type", "string"}, {"description", "Echo of the action name."}}},
          {"cleared",
           {{"type", "integer"},
            {"description", "Number of mesh entries removed from the store."}}}}}};
}

nlohmann::json ClearMeshAction::execute(const nlohmann::json& param,
                                        const Core::ProgressCallback& progress) {
    std::size_t cleared = 0;

    if(param.contains("shapeId")) {
        cleared =
            m_meshStore.removeMesh(param.value("shapeId", static_cast<uint32_t>(0))) ? 1U : 0U;
    } else {
        cleared = m_meshStore.size();
        m_meshStore.clear();
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", ACTION_NAME}, {"cleared", cleared}};
}

} // namespace OpenGeoLab::Mesh
