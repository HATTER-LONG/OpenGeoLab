/// @file list_meshes_action.cpp
/// @brief Lists all mesh entries with summary information.

#include <opengeolab/mesh/list_meshes_action.hpp>

#include <opengeolab/mesh/mesh_store.hpp>

#include <opengeolab/core/logger.hpp>

namespace OpenGeoLab::Mesh {

ListMeshesAction::ListMeshesAction(MeshStore& store) : m_store(store) {}
ListMeshesAction::~ListMeshesAction() = default;

nlohmann::json ListMeshesAction::describe() const {
    return {{"name", ACTION_NAME},
            {"description", "List all mesh entries with summary information."},
            {"params", nlohmann::json::object()},
            {"returns",
             {{"ok", {{"type", "boolean"}, {"description", "true on success"}}},
              {"action", {{"type", "string"}, {"description", "Action name"}}},
              {"count", {{"type", "integer"}, {"description", "Number of meshes"}}},
              {"meshes",
               {{"type", "array"},
                {"description", "Array of mesh summaries (meshId, name, sourceShapeId, nodeCount, "
                                "elementCount)"}}}}}};
}

nlohmann::json ListMeshesAction::execute(const nlohmann::json& /*param*/,
                                         const Core::ProgressCallback& progress) {
    if(progress) {
        progress(0.3, "Listing meshes...");
    }

    auto ids = m_store.allMeshIds();
    nlohmann::json meshes = nlohmann::json::array();

    for(const auto id : ids) {
        const auto entry = m_store.find(id);
        if(!entry) {
            continue;
        }

        nlohmann::json item = {{"meshId", entry->id},
                               {"name", entry->name},
                               {"nodeCount", entry->nodeCount()},
                               {"elementCount", entry->elementCount()}};

        if(entry->sourceShapeId.has_value()) {
            item["sourceShapeId"] = entry->sourceShapeId.value();
        } else {
            item["sourceShapeId"] = nullptr;
        }

        meshes.push_back(std::move(item));
    }

    if(progress) {
        progress(1.0, "Done");
    }

    return {{"ok", true}, {"action", ACTION_NAME}, {"count", meshes.size()}, {"meshes", meshes}};
}

} // namespace OpenGeoLab::Mesh
