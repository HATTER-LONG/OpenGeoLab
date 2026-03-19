#include <opengeolab/command/BackendDispatcher.hpp>
#include <opengeolab/command/JsonProtocol.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(opengeolab_pywrapper, module)
{
    module.doc() = "OpenGeoLab JSON process bridge";

    module.def(
        "process",
        [](const std::string& request_json) {
            return OpenGeoLab::Command::BackendDispatcher::process(request_json);
        },
        py::arg("request_json")
    );

    module.def("protocol_version", []() {
        return std::string(OpenGeoLab::Command::PROCESS_PROTOCOL_VERSION);
    });

    module.def("supported_actions", []() {
        return OpenGeoLab::Command::BackendDispatcher::supportedActions();
    });
}
