/// @file delete_mesh_action.cpp
/// @brief Removes a mesh entry from MeshStore.

#include <opengeolab/mesh/delete_mesh_action.hpp>

#include <opengeolab/mesh/mesh_store.hpp>

#include <opengeolab/core/logger.hpp>

namespace OpenGeoLab::Mesh {

DeleteMeshAction::DeleteMeshAction(MeshStore& store) : m_store(store) {}
DeleteMeshAction::~DeleteMeshAction() = default;

nlohmann::json DeleteMeshAction::describe() const {
    return {{"name", ACTION_NAME},
            {"description", "Delete a mesh entry from the store."},
            {"params",
             {{"meshId",
               {{"type", "integer"}, {"required", true}, {"description", "Mesh ID to delete"}}}}},
            {"returns",
             {{"ok", {{"type", "boolean"}, {"description", "true on success"}}},
              {"action", {{"type", "string"}, {"description", "Action name"}}},
              {"meshId", {{"type", "integer"}, {"description", "Deleted mesh ID"}}}}}};
}

nlohmann::json DeleteMeshAction::execute(const nlohmann::json& param,
                                         const Core::ProgressCallback& progress) {
    if(!param.contains("meshId") || !param["meshId"].is_number_integer()) {
        return {{"ok", false}, {"summary", "Missing or invalid 'meshId' parameter"}};
    }
    const auto mesh_id = param["meshId"].get<uint32_t>();

    // Check existence before removal. shared_ptr keeps the entry alive
    // even if another thread removes it concurrently.
    if(!m_store.find(mesh_id)) {
        return {{"ok", false}, {"summary", "Mesh ID " + std::to_string(mesh_id) + " not found"}};
    }

    if(progress) {
        progress(0.5, "Deleting mesh...");
    }

    m_store.remove(mesh_id);

    LOG_INFO("DeleteMeshAction: removed meshId={}", mesh_id);

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", ACTION_NAME}, {"meshId", mesh_id}};
}

} // namespace OpenGeoLab::Mesh
