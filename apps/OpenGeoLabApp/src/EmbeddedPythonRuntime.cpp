#include <ogl/app/EmbeddedPythonRuntime.hpp>

#include <pybind11/embed.h>
#include <pybind11/eval.h>
#include <pybind11/iostream.h>

#include <ogl/app/OpenGeoLabController.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

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

void prependSysPathIfExists(const QString& path) {
    const QString normalized_path = QDir::cleanPath(path);
    const QFileInfo candidate_info(normalized_path);
    if(!candidate_info.exists()) {
        return;
    }

    auto sys = py::module_::import("sys");
    py::list sys_path = sys.attr("path");
    const std::string path_value = normalized_path.toStdString();
    for(const auto& entry : sys_path) {
        if(py::str(entry).cast<std::string>() == path_value) {
            return;
        }
    }

    sys_path.attr("insert")(0, path_value);
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

    const auto process_request = [](const py::object& request) {
        return toPythonJson(
            activeController().executeCommand(parsePythonJsonArgument(request), "embedded-python"));
    };

    module.def("process", process_request, py::arg("request"));
}

} // namespace

namespace OGL::App {

class EmbeddedPythonRuntime::Impl {
public:
    explicit Impl(OpenGeoLabController& controller) : m_controller(controller) {
        ensureEmbeddedInterpreterStarted();
        py::gil_scoped_acquire gil;
        const QString application_dir = QCoreApplication::applicationDirPath();
        prependSysPathIfExists(application_dir);
        prependSysPathIfExists(QDir(application_dir).filePath(QStringLiteral("../lib/python")));
        m_replGlobals = std::make_unique<py::dict>(buildExecutionGlobals());
    }

    ~Impl() {
        py::gil_scoped_acquire gil;
        m_replGlobals.reset();
    }

    auto executeScript(const std::string& script) -> std::string {
        ActiveControllerScope active_scope(m_controller);
        std::ostringstream output;

        try {
            py::gil_scoped_acquire gil;
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
            py::gil_scoped_acquire gil;
            auto sys = py::module_::import("sys");
            auto builtins = py::module_::import("builtins");
            py::scoped_ostream_redirect stdout_redirect(output, sys.attr("stdout"));
            py::scoped_estream_redirect stderr_redirect(output, sys.attr("stderr"));

            try {
                auto code = builtins.attr("compile")(command_line, "<opengeolab-cli>", "eval");
                py::object value = builtins.attr("eval")(code, *m_replGlobals, *m_replGlobals);
                if(!value.is_none()) {
                    expression_result = py::repr(value).cast<std::string>();
                }
            } catch(py::error_already_set& error) {
                if(!error.matches(PyExc_SyntaxError)) {
                    throw;
                }

                PyErr_Clear();
                auto code = builtins.attr("compile")(command_line, "<opengeolab-cli>", "exec");
                builtins.attr("exec")(code, *m_replGlobals, *m_replGlobals);
            }
        } catch(const py::error_already_set& error) {
            output << error.what();
        }

        return expression_result.empty() ? output.str() : expression_result;
    }

private:
    OpenGeoLabController& m_controller;
    std::unique_ptr<py::dict> m_replGlobals;
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
