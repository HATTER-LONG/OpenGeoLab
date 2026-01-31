/**
 * @file new_model_action.cpp
 * @brief Implementation of new model action
 */

#include "newmodel_action.hpp"
#include "../geometry_document_managerImpl.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Geometry {

bool NewModelAction::execute(const nlohmann::json& /*params*/,
                             Util::ProgressCallback progress_callback) {
    if(progress_callback && !progress_callback(0.1, "Creating new model...")) {
        return false; // Cancelled
    }

    // Create a new document (this clears the old one)
    auto doc = GeometryDocumentManagerImpl::instance()->newDocumentImplType();

    if(!doc) {
        LOG_ERROR("NewModelAction: Failed to create new document");
        if(progress_callback) {
            progress_callback(1.0, "Error: Failed to create new model.");
        }
        return false;
    }

    LOG_INFO("NewModelAction: Created new empty model");

    if(progress_callback) {
        progress_callback(1.0, "New model created successfully.");
    }

    return true;
}

} // namespace OpenGeoLab::Geometry
