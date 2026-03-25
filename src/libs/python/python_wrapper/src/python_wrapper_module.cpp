/**
 * @file python_wrapper_module.cpp
 * @brief Pybind11 bridge dispatching JSON requests to C++ modules.
 */

#include <opengeolab/geometry/geometry_module.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <pybind11/pybind11.h>

#include <functional>
#include <string>
#include <string_view>

namespace Py = pybind11;

PYBIND11_MODULE(opengeolab_pywrapper, module) {
    module.doc() = "OpenGeoLab JSON process bridge — dispatches to C++ modules";

    module.def(
        "process",
        [](const std::string& request_json, Py::object progress_callback) -> std::string {
            // Wrap Python callable into C++ ProgressCallback
            OpenGeoLab::Geometry::ModuleProgressCallback cpp_callback;
            if(!progress_callback.is_none()) {
                cpp_callback = [cb = std::move(progress_callback)](double progress,
                                                                   std::string_view message) {
                    cb(progress, Py::str(std::string(message)));
                };
            }

            // Extract module name to route to correct C++ handler
            // Quick parse — look for "module" key in JSON
            const auto module_pos = request_json.find("\"module\"");
            if(module_pos != std::string::npos) {
                if(request_json.find("\"geometry\"", module_pos) != std::string::npos) {
                    static OpenGeoLab::Scene::SceneGraph scene_graph;
                    static OpenGeoLab::Geometry::GeometryModule geometry_module{scene_graph};
                    return geometry_module.process(request_json, cpp_callback);
                }
            }

            return R"({"ok":false,"summary":"No C++ handler for this module."})";
        },
        Py::arg("request_json"), Py::arg("progress_callback") = Py::none(),
        "Forward a JSON request to the appropriate C++ module.");

    module.def(
        "protocol_version", []() -> std::string { return "1.0"; },
        "Return the protocol version supported by this bridge.");
}
