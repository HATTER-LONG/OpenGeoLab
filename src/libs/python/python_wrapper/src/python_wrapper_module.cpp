/**
 * @file python_wrapper_module.cpp
 * @brief Pybind11 bridge dispatching JSON requests to C++ modules via CommandDispatcher.
 */
#include <opengeolab/command/command_dispatcher.hpp>
#include <opengeolab/command/module_registry.hpp>
#include <opengeolab/core/progress_callback.hpp>

#include <kangaroo/util/plugin_component_factory.hpp>
#include <nlohmann/json.hpp>
#include <pybind11/pybind11.h>

#include <mutex>
#include <string>
#include <string_view>

namespace Py = pybind11;

/**
 * @brief Lazy-initialized dispatcher backed by the global PluginComponentFactory.
 */
static OpenGeoLab::Command::CommandDispatcher& getDispatcher() {
    static std::once_flag flag;
    std::call_once(flag,
                   [] { OpenGeoLab::Command::registerBuiltinModules(g_PluginComponentFactory); });
    static OpenGeoLab::Command::CommandDispatcher dispatcher(g_PluginComponentFactory);
    return dispatcher;
}

PYBIND11_MODULE(opengeolab_pywrapper, m) {
    m.doc() = "OpenGeoLab JSON process bridge — dispatches to C++ modules";

    m.def(
        "process",
        [](std::string_view request_json, Py::object progress_callback) -> std::string {
            auto request = nlohmann::json::parse(request_json);

            OpenGeoLab::Core::ProgressCallback cpp_progress;
            if(progress_callback.is_none()) {
                cpp_progress = OpenGeoLab::Core::NO_PROGRESS_CALLBACK;
            } else {
                cpp_progress = [cb = Py::object(progress_callback)](
                                   double progress, const std::string& message) -> bool {
                    try {
                        const Py::gil_scoped_acquire acquire;
                        auto result = cb(progress, message);
                        return result.cast<bool>();
                    } catch(const Py::error_already_set& e) {
                        throw std::runtime_error(std::string("Python progress callback raised: ") +
                                                 e.what());
                    }
                };
            }

            auto result = getDispatcher().dispatch(request, cpp_progress);
            return result.dump();
        },
        Py::arg("request_json"), Py::arg("progress_callback") = Py::none(),
        "Forward a JSON request to the appropriate C++ module.\n\n"
        "Args:\n"
        "    request_json: JSON string with 'module', 'action', 'param' fields\n"
        "    progress_callback: Optional callable(progress: float, message: str) -> bool\n\n"
        "Returns:\n"
        "    JSON string with the response from the module");

    m.def(
        "has_module",
        [](std::string_view module_name) -> bool { return getDispatcher().hasModule(module_name); },
        Py::arg("module_name"), "Check if a module is registered.");

    m.def(
        "list_modules",
        []() -> Py::list {
            auto infos = getDispatcher().listModules();
            Py::list result;
            for(const auto& info : infos) {
                result.append(info.m_moduleName);
            }
            return result;
        },
        "List all registered module names.");

    m.def(
        "describe", []() -> std::string { return getDispatcher().describe().dump(); },
        "Describe the entire command system (request schema + all modules/actions).\n\n"
        "Returns:\n"
        "    JSON string with 'request_schema' and 'modules' fields");
}