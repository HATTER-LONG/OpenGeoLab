/**
 * @file python_wrapper_module.cpp
 * @brief Defines the pybind11 bridge that forwards JSON protocol requests to commands.
 */

#include <opengeolab/command/bounding_box_command.hpp>
#include <opengeolab/command/command_dispatcher.hpp>
#include <opengeolab/command/get_stored_bbox_command.hpp>
#include <opengeolab/command/set_points_command.hpp>
#include <opengeolab/geometry/point_store.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <memory>
#include <string>
#include <vector>

namespace Py = pybind11;
using OpenGeoLab::Command::BoundingBoxCommand;
using OpenGeoLab::Command::CommandDispatcher;
using OpenGeoLab::Command::GetStoredBBoxCommand;
using OpenGeoLab::Command::SetPointsCommand;
using OpenGeoLab::Geometry::PointStore;

PYBIND11_MODULE(opengeolab_pywrapper, module) {
    module.doc() = "OpenGeoLab JSON process bridge";

    static auto dispatcher = [] {
        auto point_store = std::make_shared<PointStore>();
        CommandDispatcher command_dispatcher;
        command_dispatcher.registerCommand(std::make_unique<BoundingBoxCommand>());
        command_dispatcher.registerCommand(std::make_unique<SetPointsCommand>(point_store));
        command_dispatcher.registerCommand(std::make_unique<GetStoredBBoxCommand>(point_store));
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
