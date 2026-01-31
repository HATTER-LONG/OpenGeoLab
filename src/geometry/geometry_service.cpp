/**
 * @file geometry_service.cpp
 * @brief Implementation of GeometryService request processing
 */

#include "geometry/geometry_service.hpp"
#include "action/create_action.hpp"
#include "action/newmodel_action.hpp"
#include "geometry_document_managerImpl.hpp"
#include "util/logger.hpp"
#include "util/progress_bridge.hpp"

namespace OpenGeoLab::Geometry {
nlohmann::json GeometryService::processRequest(const std::string& module_name,
                                               const nlohmann::json& params,
                                               App::IProgressReporterPtr progress_reporter) {
    LOG_DEBUG("GeometryService: Processing request for module: {}", module_name);
    nlohmann::json response;

    if(!params.contains("action") || !params["action"].is_string()) {
        LOG_ERROR("GeometryService: Missing or invalid 'action' parameter");
        throw std::invalid_argument("Missing or invalid 'action' parameter.");
    }

    const std::string action_name = params["action"].get<std::string>();
    LOG_INFO("GeometryService: Executing action '{}'", action_name);

    auto action = g_ComponentFactory.createObjectWithID<GeometryActionFactory>(action_name);

    bool success = action->execute(params, Util::makeProgressCallback(progress_reporter, 0.0, 1.0));

    if(!success) {
        LOG_ERROR("GeometryService: Action '{}' failed to execute", action_name);
        throw std::runtime_error("Geometry action '" + action_name + "' failed to execute.");
    }

    LOG_INFO("GeometryService: Action '{}' completed successfully", action_name);
    response["status"] = "success";
    return response;
}

GeometryServiceFactory::tObjectSharedPtr GeometryServiceFactory::instance() const {
    static tObjectSharedPtr singleton_instance = std::make_shared<GeometryService>();
    return singleton_instance;
}
void registerServices() {
    LOG_DEBUG("GeometryService: Registering geometry services and actions");
    g_ComponentFactory.registInstanceFactoryWithID<GeometryServiceFactory>("GeometryService");
    g_ComponentFactory.registFactoryWithID<CreateActionFactory>(CreateAction::actionName());
    g_ComponentFactory.registFactoryWithID<NewModelActionFactory>(NewModelAction::actionName());
    g_ComponentFactory.registInstanceFactory<GeometryDocumentManagerImplSingletonFactory>();

    auto test_getter =
        g_ComponentFactory.getInstanceObject<GeometryDocumentManagerImplSingletonFactory>();
    auto test_getter2 = g_ComponentFactory.getInstanceObject<IGeoDocumentManagerSingletonFactory>();
}
} // namespace OpenGeoLab::Geometry