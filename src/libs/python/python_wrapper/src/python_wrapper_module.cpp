/**
 * @file python_wrapper_module.cpp
 * @brief Lightweight pybind11 bridge — placeholder for future C++ module dispatch.
 */

#include <pybind11/pybind11.h>

#include <string>

namespace Py = pybind11;

PYBIND11_MODULE(opengeolab_pywrapper, module) {
    module.doc() = "OpenGeoLab JSON process bridge (lightweight placeholder)";

    module.def(
        "process",
        [](const std::string& /*request_json*/) -> std::string {
            return R"({"ok":false,"summary":"No C++ modules registered yet."})";
        },
        Py::arg("request_json"), "Forward a JSON request to C++ modules (not yet implemented).");

    module.def(
        "protocol_version", []() -> std::string { return "1.0"; },
        "Return the protocol version supported by this bridge.");
}
