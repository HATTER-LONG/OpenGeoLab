#include <ogl/app/EmbeddedPythonRuntime.hpp>

#include <pybind11/embed.h>
#include <pybind11/eval.h>
#include <pybind11/iostream.h>

#include <ogl/python_wrapper/PythonJsonInterop.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include <sstream>
#include <stdexcept>
#include <utility>

namespace py = pybind11;

namespace {

OGL::App::EmbeddedPythonRuntime::ProcessRequestHandler* active_process_request_handler = nullptr;

auto activeProcessRequestHandler() -> OGL::App::EmbeddedPythonRuntime::ProcessRequestHandler& {
    if(active_process_request_handler == nullptr) {
        throw std::runtime_error(
            "No active OpenGeoLab request handler is bound to the embedded Python runtime.");
    }

    return *active_process_request_handler;
}

auto buildExecutionGlobals() -> py::dict {
    py::dict globals;
    globals["__builtins__"] = py::module_::import("builtins");
    globals["opengeolab_app"] = py::module_::import("opengeolab_app");
    return globals;
}

class PythonIoCapture {
public:
    PythonIoCapture()
        : m_stdoutHandle(py::module_::import("io").attr("StringIO")()),
          m_stderrHandle(py::module_::import("io").attr("StringIO")()),
          m_stdoutRedirect(
              py::module_::import("contextlib").attr("redirect_stdout")(m_stdoutHandle)),
          m_stderrRedirect(
              py::module_::import("contextlib").attr("redirect_stderr")(m_stderrHandle)) {
        m_stdoutRedirect.attr("__enter__")();
        m_stderrRedirect.attr("__enter__")();
    }

    ~PythonIoCapture() {
        m_stderrRedirect.attr("__exit__")(py::none(), py::none(), py::none());
        m_stdoutRedirect.attr("__exit__")(py::none(), py::none(), py::none());
    }

    auto capturedText() const -> std::string {
        return m_stdoutHandle.attr("getvalue")().cast<std::string>() +
               m_stderrHandle.attr("getvalue")().cast<std::string>();
    }

private:
    py::object m_stdoutHandle;
    py::object m_stderrHandle;
    py::object m_stdoutRedirect;
    py::object m_stderrRedirect;
};

void prependSysPathIfExists(const QString& path) {
    const QString normalizedPath = QDir::cleanPath(path);
    const QFileInfo candidateInfo(normalizedPath);
    if(!candidateInfo.exists()) {
        return;
    }

    auto sys = py::module_::import("sys");
    py::list sysPath = sys.attr("path");
    const std::string pathValue = normalizedPath.toStdString();
    for(const auto& entry : sysPath) {
        if(py::str(entry).cast<std::string>() == pathValue) {
            return;
        }
    }

    sysPath.attr("insert")(0, pathValue);
}

struct ActiveRequestHandlerScope {
    explicit ActiveRequestHandlerScope(
        OGL::App::EmbeddedPythonRuntime::ProcessRequestHandler& processRequest)
        : previous(active_process_request_handler) {
        active_process_request_handler = &processRequest;
    }

    ~ActiveRequestHandlerScope() { active_process_request_handler = previous; }

    OGL::App::EmbeddedPythonRuntime::ProcessRequestHandler* previous;
};

void ensureEmbeddedInterpreterStarted() {
    static auto* interpreter = new py::scoped_interpreter();
    static_cast<void>(interpreter);
}

PYBIND11_EMBEDDED_MODULE(opengeolab_app, module) {
    module.doc() = "Embedded OpenGeoLab application control API";

    const auto processRequest = [](const py::object& request) {
        return OGL::PythonWrapper::toPythonJson(
            activeProcessRequestHandler()(OGL::PythonWrapper::parsePythonJsonArgument(request)));
    };

    module.def("process", processRequest, py::arg("request"));
}

} // namespace

namespace OGL::App {

class EmbeddedPythonRuntime::Impl {
public:
    explicit Impl(ProcessRequestHandler processRequestHandler)
        : m_processRequestHandler(std::move(processRequestHandler)) {
        ensureEmbeddedInterpreterStarted();
        py::gil_scoped_acquire gil;
        const QString applicationDir = QCoreApplication::applicationDirPath();
        prependSysPathIfExists(applicationDir);
        prependSysPathIfExists(QDir(applicationDir).filePath(QStringLiteral("../lib/python")));
        m_replGlobals = std::make_unique<py::dict>(buildExecutionGlobals());
    }

    ~Impl() {
        py::gil_scoped_acquire gil;
        m_replGlobals.reset();
    }

    auto executeScript(const std::string& script) -> std::string {
        ActiveRequestHandlerScope activeScope(m_processRequestHandler);
        py::gil_scoped_acquire gil;
        PythonIoCapture capture;

        try {
            py::dict globals = buildExecutionGlobals();
            py::exec(script, globals);
        } catch(const py::error_already_set& error) {
            return capture.capturedText() + error.what();
        }

        return capture.capturedText();
    }

    auto executeCommandLine(const std::string& commandLine) -> std::string {
        ActiveRequestHandlerScope activeScope(m_processRequestHandler);
        py::gil_scoped_acquire gil;
        PythonIoCapture capture;
        std::string expressionResult;

        try {
            auto builtins = py::module_::import("builtins");

            try {
                auto code = builtins.attr("compile")(commandLine, "<opengeolab-cli>", "eval");
                py::object value = builtins.attr("eval")(code, *m_replGlobals, *m_replGlobals);
                if(!value.is_none()) {
                    expressionResult = py::repr(value).cast<std::string>();
                }
            } catch(py::error_already_set& error) {
                if(!error.matches(PyExc_SyntaxError)) {
                    throw;
                }

                PyErr_Clear();
                auto code = builtins.attr("compile")(commandLine, "<opengeolab-cli>", "exec");
                builtins.attr("exec")(code, *m_replGlobals, *m_replGlobals);
            }
        } catch(const py::error_already_set& error) {
            return capture.capturedText() + error.what();
        }

        return expressionResult.empty() ? capture.capturedText() : expressionResult;
    }

private:
    ProcessRequestHandler m_processRequestHandler;
    std::unique_ptr<py::dict> m_replGlobals;
};

EmbeddedPythonRuntime::EmbeddedPythonRuntime(ProcessRequestHandler processRequestHandler)
    : m_impl(std::make_unique<Impl>(std::move(processRequestHandler))) {}

EmbeddedPythonRuntime::~EmbeddedPythonRuntime() = default;

auto EmbeddedPythonRuntime::executeScript(const std::string& script) -> std::string {
    return m_impl->executeScript(script);
}

auto EmbeddedPythonRuntime::executeCommandLine(const std::string& commandLine) -> std::string {
    return m_impl->executeCommandLine(commandLine);
}

} // namespace OGL::App
