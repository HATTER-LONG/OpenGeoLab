#include <ogl/python_wrapper/OpenGeoLabPythonBridge.hpp>

#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace {

auto parsePythonJsonArgument(const py::object& value) -> nlohmann::json {
    if(value.is_none()) {
        return nlohmann::json::object();
    }

    if(py::isinstance<py::str>(value)) {
        const auto params_json = value.cast<std::string>();
        return params_json.empty() ? nlohmann::json::object() : nlohmann::json::parse(params_json);
    }

    const auto json_module = py::module_::import("json");
    return nlohmann::json::parse(json_module.attr("dumps")(value).cast<std::string>());
}

} // namespace

PYBIND11_MODULE(opengeolab, module) {
    module.doc() = "OpenGeoLab Python bridge for modular component routing";

    py::class_<OGL::PythonWrapper::OpenGeoLabPythonBridge>(module, "OpenGeoLabPythonBridge")
        .def(py::init<>())
        .def(
            "process",
            [](const OGL::PythonWrapper::OpenGeoLabPythonBridge& bridge,
               const py::object& request) {
                return bridge.process(parsePythonJsonArgument(request)).dump(2);
            },
            py::arg("request"));

    module.def(
        "process",
        [](const py::object& request) {
            OGL::PythonWrapper::OpenGeoLabPythonBridge bridge;
            return bridge.process(parsePythonJsonArgument(request)).dump(2);
        },
        py::arg("request"));
}
