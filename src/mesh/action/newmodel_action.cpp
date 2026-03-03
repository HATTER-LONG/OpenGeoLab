/**
 * @file newmodel_action.cpp
 * @brief Implementation of NewModelAction for mesh
 */

#include "newmodel_action.hpp"
#include "../mesh_documentImpl.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Mesh {

nlohmann::json NewModelAction::execute(const nlohmann::json& /*params*/,
                                       Util::ProgressCallback progress_callback) {
    nlohmann::json response;

    if(progress_callback && !progress_callback(0.1, "Creating new mesh...")) {
        response["success"] = false;
        response["error"] = "Operation cancelled";
        return response;
    }

    auto doc = MeshDocumentImpl::instance();
    doc->clear();
    LOG_INFO("NewModelAction: Created new empty mesh");

    if(progress_callback) {
        progress_callback(1.0, "New mesh created successfully.");
    }

    response["success"] = true;
    response["message"] = "New mesh created successfully";
    return response;
}

} // namespace OpenGeoLab::Mesh
