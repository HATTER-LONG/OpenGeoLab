#include "geometry/geometry_service.hpp"
#include "create_action.hpp"
#include "new_model_action.hpp"
#include "util/logger.hpp"
#include "util/progress_bridge.hpp"

namespace OpenGeoLab::Geometry {
nlohmann::json GeometryService::processRequest(const std::string& module_name,
                                               const nlohmann::json& params,
                                               App::IProgressReporterPtr progress_reporter) {
    LOG_INFO("GeometryService: Processing request for module: {}", module_name);
    nlohmann::json response;

    if(!params.contains("action") || !params["action"].is_string()) {
        throw std::invalid_argument("Missing or invalid 'action' parameter.");
    }

    auto action = g_ComponentFactory.createObjectWithID<GeometryActionFactory>(
        params["action"].get<std::string>());

    bool success = action->execute(params, Util::makeProgressCallback(progress_reporter, 0.0, 1.0));

    if(!success) {
        throw std::runtime_error("Geometry action '" + params["action"].get<std::string>() +
                                 "' failed to execute.");
    }

    response["status"] = "success";
    return response;
}

GeometryServiceFactory::tObjectSharedPtr GeometryServiceFactory::instance() const {
    static tObjectSharedPtr singleton_instance = std::make_shared<GeometryService>();
    return singleton_instance;
}
void registerServices() {
    g_ComponentFactory.registInstanceFactoryWithID<GeometryServiceFactory>("GeometryService");
    g_ComponentFactory.registFactoryWithID<CreateActionFactory>(CreateAction::actionName());
    g_ComponentFactory.registFactoryWithID<NewModelActionFactory>(NewModelAction::actionName());
}
} // namespace OpenGeoLab::Geometry