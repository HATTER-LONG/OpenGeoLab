#include <ogl/python_wrapper/OpenGeoLabPythonBridge.hpp>

#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace {

auto parseJsonOrEmpty(const std::string& params_json) -> nlohmann::json {
    if(params_json.empty()) {
        return nlohmann::json::object();
    }
    return nlohmann::json::parse(params_json);
}

} // namespace

PYBIND11_MODULE(opengeolab, module) {
    module.doc() = "OpenGeoLab placeholder Python bridge for modular component routing";

    py::class_<OGL::PythonWrapper::OpenGeoLabPythonBridge>(module, "OpenGeoLabPythonBridge")
        .def(py::init<>())
        .def(
            "call",
            [](const OGL::PythonWrapper::OpenGeoLabPythonBridge& bridge,
               const std::string& module_name, const std::string& params_json) {
                return bridge.call(module_name, parseJsonOrEmpty(params_json)).dump(2);
            },
            py::arg("module_name"), py::arg("params_json") = "{}")
        .def("suggest_placeholder_geometry_script",
             &OGL::PythonWrapper::OpenGeoLabPythonBridge::suggestPlaceholderGeometryScript,
             py::arg("model_name") = "Bracket_A01", py::arg("body_count") = 3);

    module.def(
        "call",
        [](const std::string& module_name, const std::string& params_json) {
            OGL::PythonWrapper::OpenGeoLabPythonBridge bridge;
            return bridge.call(module_name, parseJsonOrEmpty(params_json)).dump(2);
        },
        py::arg("module_name"), py::arg("params_json") = "{}");
}