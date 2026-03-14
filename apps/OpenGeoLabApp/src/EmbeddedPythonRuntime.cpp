#include <ogl/app/EmbeddedPythonRuntime.hpp>

#include <pybind11/embed.h>
#include <pybind11/eval.h>
#include <pybind11/iostream.h>

#include <ogl/app/OpenGeoLabController.hpp>

#include <QCoreApplication>

#include <nlohmann/json.hpp>

#include <sstream>
#include <stdexcept>

namespace py = pybind11;

namespace {

OGL::App::OpenGeoLabController* active_controller_instance = nullptr;

auto activeController() -> OGL::App::OpenGeoLabController& {
    if(active_controller_instance == nullptr) {
        throw std::runtime_error(
            "No active OpenGeoLab controller is bound to the embedded Python runtime.");
    }

    return *active_controller_instance;
}

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

auto toPythonJson(const nlohmann::json& value) -> py::object {
    return py::module_::import("json").attr("loads")(value.dump());
}

auto buildExecutionGlobals() -> py::dict {
    py::dict globals;
    globals["__builtins__"] = py::module_::import("builtins");
    globals["opengeolab_app"] = py::module_::import("opengeolab_app");
    return globals;
}

struct ActiveControllerScope {
    explicit ActiveControllerScope(OGL::App::OpenGeoLabController& controller)
        : previous(active_controller_instance) {
        active_controller_instance = &controller;
    }

    ~ActiveControllerScope() { active_controller_instance = previous; }

    OGL::App::OpenGeoLabController* previous;
};

void ensureEmbeddedInterpreterStarted() {
    static auto* interpreter = new py::scoped_interpreter();
    static_cast<void>(interpreter);
}

PYBIND11_EMBEDDED_MODULE(opengeolab_app, module) {
    module.doc() = "Embedded OpenGeoLab application control API";

    const auto call_command = [](const std::string& module_name, const py::object& params) {
        return toPythonJson(activeController().executeCommand(
            module_name, parsePythonJsonArgument(params), "embedded-python"));
    };

    module.def("call", call_command, py::arg("module_name"), py::arg("params") = py::dict());
    module.def("run_command", call_command, py::arg("module_name"), py::arg("params") = py::dict());

    module.def("replay_commands",
               []() { return toPythonJson(activeController().replayRecordedCommandsJson()); });
    module.def("clear_commands",
               []() { return toPythonJson(activeController().clearRecordedCommandsJson()); });
    module.def("get_state",
               []() { return toPythonJson(activeController().applicationStateJson()); });
}

} // namespace

namespace OGL::App {

class EmbeddedPythonRuntime::Impl {
public:
    explicit Impl(OpenGeoLabController& controller) : m_controller(controller) {
        ensureEmbeddedInterpreterStarted();
        auto sys = py::module_::import("sys");
        sys.attr("path").attr("insert")(0, QCoreApplication::applicationDirPath().toStdString());
    }

    ~Impl() = default;

    auto executeScript(const std::string& script) -> std::string {
        ActiveControllerScope active_scope(m_controller);
        std::ostringstream output;

        try {
            auto sys = py::module_::import("sys");
            py::scoped_ostream_redirect stdout_redirect(output, sys.attr("stdout"));
            py::scoped_estream_redirect stderr_redirect(output, sys.attr("stderr"));
            py::dict globals = buildExecutionGlobals();
            py::exec(script, globals);
        } catch(const py::error_already_set& error) {
            output << error.what();
        }

        return output.str();
    }

    auto executeCommandLine(const std::string& command_line) -> std::string {
        ActiveControllerScope active_scope(m_controller);
        std::ostringstream output;
        std::string expression_result;

        try {
            auto sys = py::module_::import("sys");
            auto builtins = py::module_::import("builtins");
            py::scoped_ostream_redirect stdout_redirect(output, sys.attr("stdout"));
            py::scoped_estream_redirect stderr_redirect(output, sys.attr("stderr"));
            py::dict globals = buildExecutionGlobals();

            try {
                auto code = builtins.attr("compile")(command_line, "<opengeolab-cli>", "eval");
                py::object value = builtins.attr("eval")(code, globals, globals);
                if(!value.is_none()) {
                    expression_result = py::repr(value).cast<std::string>();
                }
            } catch(py::error_already_set& error) {
                if(!error.matches(PyExc_SyntaxError)) {
                    throw;
                }

                PyErr_Clear();
                auto code = builtins.attr("compile")(command_line, "<opengeolab-cli>", "exec");
                builtins.attr("exec")(code, globals, globals);
            }
        } catch(const py::error_already_set& error) {
            output << error.what();
        }

        return expression_result.empty() ? output.str() : expression_result;
    }

private:
    OpenGeoLabController& m_controller;
};

EmbeddedPythonRuntime::EmbeddedPythonRuntime(OpenGeoLabController& controller)
    : m_impl(std::make_unique<Impl>(controller)) {}

EmbeddedPythonRuntime::~EmbeddedPythonRuntime() = default;

auto EmbeddedPythonRuntime::executeScript(const std::string& script) -> std::string {
    return m_impl->executeScript(script);
}

auto EmbeddedPythonRuntime::executeCommandLine(const std::string& command_line) -> std::string {
    return m_impl->executeCommandLine(command_line);
}

} // namespace OGL::App