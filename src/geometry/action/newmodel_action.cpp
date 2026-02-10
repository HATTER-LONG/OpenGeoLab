/**
 * @file newmodel_action.cpp
 * @brief Implementation of new model action
 */

#include "newmodel_action.hpp"
#include "../geometry_documentImpl.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Geometry {

nlohmann::json NewModelAction::execute(const nlohmann::json& /*params*/,
                                       Util::ProgressCallback progress_callback) {
    nlohmann::json response;

    if(progress_callback && !progress_callback(0.1, "Creating new model...")) {
        response["success"] = false;
        response["error"] = "Operation cancelled";
        return response;
    }

    auto doc = GeometryDocumentImpl::instance();
    doc->clear();
    LOG_INFO("NewModelAction: Created new empty model");

    if(progress_callback) {
        progress_callback(1.0, "New model created successfully.");
    }

    response["success"] = true;
    response["message"] = "New model created successfully";
    return response;
}

} // namespace OpenGeoLab::Geometry
