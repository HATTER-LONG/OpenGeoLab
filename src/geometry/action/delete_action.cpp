/**
 * @file delete_action.cpp
 * @brief Implementation of delete entities action
 */

#include "delete_action.hpp"
#include "../geometry_documentImpl.hpp"
#include "../geometry_document_managerImpl.hpp"
#include "util/logger.hpp"


namespace OpenGeoLab::Geometry {

[[nodiscard]] bool DeleteAction::execute(const nlohmann::json& params,
                                         Util::ProgressCallback progress_callback) {
    if(!params.contains("entities") || !params["entities"].is_array()) {
        if(progress_callback) {
            progress_callback(1.0, "Error: Missing or invalid 'entities' parameter.");
        }
        LOG_ERROR("DeleteAction: Missing or invalid 'entities' parameter");
        return false;
    }

    std::vector<EntityId> entityIds;
    for(const auto& idJson : params["entities"]) {
        entityIds.push_back(idJson.get<EntityId>());
    }

    if(entityIds.empty()) {
        if(progress_callback) {
            progress_callback(1.0, "Error: No entities specified for deletion.");
        }
        LOG_ERROR("DeleteAction: No entities specified for deletion");
        return false;
    }

    bool deleteChildren = params.value("deleteChildren", true);

    if(progress_callback && !progress_callback(0.1, "Preparing delete operation...")) {
        return false;
    }

    auto doc = std::dynamic_pointer_cast<GeometryDocumentImpl>(
        GeometryDocumentManager::instance().currentDocument());

    if(!doc) {
        if(progress_callback) {
            progress_callback(1.0, "Error: No active document.");
        }
        LOG_ERROR("DeleteAction: No active document");
        return false;
    }

    if(progress_callback && !progress_callback(0.3, "Deleting entities...")) {
        return false;
    }

    bool result = doc->deleteEntities(entityIds, deleteChildren);

    if(result) {
        if(progress_callback) {
            progress_callback(1.0, "Delete completed successfully.");
        }
        LOG_INFO("DeleteAction: Deleted {} entities", entityIds.size());
    } else {
        if(progress_callback) {
            progress_callback(1.0, "Warning: Some entities could not be deleted.");
        }
        LOG_WARN("DeleteAction: Some entities could not be deleted");
    }

    return result;
}

} // namespace OpenGeoLab::Geometry
