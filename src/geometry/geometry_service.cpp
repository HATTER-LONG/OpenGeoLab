#include "geometry/geometry_service.hpp"
#include "util/logger.hpp"

namespace OpenGeoLab::Geometry {
nlohmann::json GeometryService::processRequest(const std::string& module_name,
                                               const nlohmann::json& params,
                                               App::IProgressReporterPtr progress_reporter) {
    LOG_INFO("GeometryService: Processing request for module: {}", module_name);
    nlohmann::json response;

    response["status"] = "success";
    response["data"] = {{"message", "Example module processed"}};

    return response;
}

GeometryServiceFactory::tObjectSharedPtr GeometryServiceFactory::instance() const {
    static tObjectSharedPtr singleton_instance = std::make_shared<GeometryService>();
    return singleton_instance;
}
} // namespace OpenGeoLab::Geometry