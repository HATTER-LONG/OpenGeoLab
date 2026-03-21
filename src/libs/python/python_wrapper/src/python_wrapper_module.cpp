/**
 * @file python_wrapper_module.cpp
 * @brief Defines the pybind11 bridge that forwards JSON protocol requests via ModuleDispatcher.
 */

#include <opengeolab/command/module_dispatcher.hpp>
#include <opengeolab/command/module_registry.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <string>
#include <vector>

namespace Py = pybind11;

PYBIND11_MODULE(opengeolab_pywrapper, module) {
    module.doc() = "OpenGeoLab JSON process bridge";

    auto& factory = Kangaroo::Util::PluginComponentFactory::instance();
    OpenGeoLab::Command::registerAllModules(factory);

    static auto dispatcher = OpenGeoLab::Command::ModuleDispatcher(factory);

    module.def(
        "process",
        [](const std::string& request_json) -> std::string {
            return dispatcher.dispatch(request_json);
        },
        Py::arg("request_json"));

    module.def("protocol_version", []() { return "1.0"; });

    module.def("registered_modules", []() { return dispatcher.registeredModules(); });
}
