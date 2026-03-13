#include <ogl/python_wrapper/OpenGeoLabPythonBridge.hpp>

#include <ogl/core/ComponentRequestDispatcher.hpp>
#include <ogl/geometry/GeometryComponentRegistration.hpp>

namespace ogl::python_wrapper {

OpenGeoLabPythonBridge::OpenGeoLabPythonBridge() { ogl::geometry::registerGeometryComponents(); }

auto OpenGeoLabPythonBridge::call(const std::string& module_name,
                                  const nlohmann::json& params) const -> nlohmann::json {
    return ogl::core::ComponentRequestDispatcher::dispatch(module_name, params).toJson();
}

auto OpenGeoLabPythonBridge::suggestPlaceholderGeometryScript(const std::string& model_name,
                                                              int body_count) const -> std::string {
    const nlohmann::json request = {
        {"operation", "placeholderModel"},
        {"modelName", model_name},
        {"bodyCount", body_count},
        {"source", "python"},
    };

    return call("geometry", request)
        .value("payload", nlohmann::json::object())
        .value("equivalentPython", std::string{});
}

} // namespace ogl::python_wrapper