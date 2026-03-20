#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace Py = pybind11;

PYBIND11_MODULE(opengeolab_pywrapper, module) {
    module.doc() = "OpenGeoLab JSON process bridge";

    module.def(
        "process", [](const std::string& request_json) { return true; }, Py::arg("request_json"));

    module.def("protocol_version", []() { return "1.0"; });

    module.def("supported_actions", []() { return std::vector<std::string>{"example_action"}; });
}
