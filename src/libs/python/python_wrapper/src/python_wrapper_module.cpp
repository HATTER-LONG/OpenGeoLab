/**
 * @file python_wrapper_module.cpp
 * @brief Defines the pybind11 bridge that forwards JSON protocol requests to commands.
 */

#include <opengeolab/command/bounding_box_command.hpp>
#include <opengeolab/command/command_dispatcher.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <memory>
#include <string>
#include <vector>

namespace Py = pybind11;
using OpenGeoLab::Command::BoundingBoxCommand;
using OpenGeoLab::Command::CommandDispatcher;

PYBIND11_MODULE(opengeolab_pywrapper, module) {
    module.doc() = "OpenGeoLab JSON process bridge";

    static auto dispatcher = [] {
        CommandDispatcher command_dispatcher;
        command_dispatcher.registerCommand(std::make_unique<BoundingBoxCommand>());
        return command_dispatcher;
    }();

    module.def(
        "process",
        [](const std::string& request_json) -> std::string {
            return dispatcher.dispatch(request_json);
        },
        Py::arg("request_json"));

    module.def("protocol_version", []() { return "1.0"; });

    module.def("supported_actions", []() {
        const auto actions = dispatcher.registeredActions();
        return std::vector<std::string>(actions.begin(), actions.end());
    });
}
