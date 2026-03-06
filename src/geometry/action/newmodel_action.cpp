/**
 * @file newmodel_action.cpp
 * @brief Implementation of new model action
 */

#include "newmodel_action.hpp"
#include "../geometry_documentImpl.hpp"
#include "app/service.hpp"
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

    try {
        if(progress_callback) {
            progress_callback(0.5, "Clearing mesh and geometry data...");
        }

        nlohmann::json mesh_req = {{"action", "new_mesh"}};
        auto service = g_ComponentFactory.getInstanceObjectWithID<App::IServiceSingletonFactory>(
            "MeshService");
        auto mesh_res =
            service->processRequest("MeshService", mesh_req, App::IProgressReporterPtr{});
        if(!mesh_res.value("success", false)) {
            const std::string error = mesh_res.value("error", "Failed to clear mesh document");
            LOG_ERROR("NewModelAction: MeshService returned failure: {}", mesh_res.dump());
            response["success"] = false;
            response["error"] = error;
            return response;
        }

        auto doc = GeometryDocumentImpl::instance();
        doc->clear();
        LOG_INFO("NewModelAction: Cleared mesh and created new empty model");
    } catch(const std::exception& e) {
        LOG_ERROR("NewModelAction: Failed to clear model state: {}", e.what());
        response["success"] = false;
        response["error"] = std::string("Failed to clear model state: ") + e.what();
        return response;
    } catch(...) {
        LOG_ERROR("NewModelAction: Failed to clear model state (unknown error)");
        response["success"] = false;
        response["error"] = "Failed to clear model state";
        return response;
    }

    if(progress_callback) {
        progress_callback(1.0, "New model created successfully.");
    }

    response["success"] = true;
    response["message"] = "New model created successfully";
    return response;
}

} // namespace OpenGeoLab::Geometry
