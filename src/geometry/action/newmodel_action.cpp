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

    auto doc = GeometryDocumentImpl::instance();
    doc->clear();
    LOG_INFO("NewModelAction: Created new empty model");

    // Also clear any existing mesh data via MeshService to ensure a clean state
    try {
        nlohmann::json mesh_req = {{"action", "new_mesh"}};
        auto service = g_ComponentFactory.getInstanceObjectWithID<App::IServiceSingletonFactory>(
            "MeshService");
        auto mesh_res =
            service->processRequest("MeshService", mesh_req, App::IProgressReporterPtr{});
        if(mesh_res.value("success", false)) {
            LOG_INFO("NewModelAction: Requested mesh clear via MeshService");
        } else {
            LOG_WARN("NewModelAction: MeshService returned failure: {}", mesh_res.dump());
        }
    } catch(const std::exception& e) {
        LOG_WARN("NewModelAction: MeshService request failed: {}", e.what());
    } catch(...) {
        LOG_WARN("NewModelAction: MeshService request failed (unknown error)");
    }

    if(progress_callback) {
        progress_callback(1.0, "New model created successfully.");
    }

    response["success"] = true;
    response["message"] = "New model created successfully";
    return response;
}

} // namespace OpenGeoLab::Geometry
