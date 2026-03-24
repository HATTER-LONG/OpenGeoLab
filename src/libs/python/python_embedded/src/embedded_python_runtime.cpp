/**
 * @file embedded_python_runtime.cpp
 * @brief Implements the embedded Python interpreter bootstrap and request bridge.
 */

#include <opengeolab/python/embedded_python_runtime.hpp>

// pybind11/embed.h MUST precede any direct Python.h include so that pybind11
// can suppress the _DEBUG → Py_DEBUG redirection on MSVC Debug builds.
#include <pybind11/embed.h>

#include <cstdlib>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Py = pybind11;

namespace OpenGeoLab::Python {

struct EmbeddedPythonRuntime::Impl {
    explicit Impl(PyConfig* config) : interpreter(config, 0, nullptr, false) {}

    Py::scoped_interpreter interpreter;
    Py::object processFunction;
};

[[nodiscard]] static std::filesystem::path environmentPath(const char* variable_name) {
#if defined(_WIN32)
    char* raw_value = nullptr;
    size_t raw_size = 0;
    const errno_t result = _dupenv_s(&raw_value, &raw_size, variable_name);
    if(result != 0 || raw_value == nullptr || raw_size == 0) {
        if(raw_value != nullptr) {
            free(raw_value);
        }
        return {};
    }

    const std::string value{raw_value};
    free(raw_value);
    return value;
#else
    const char* raw_value = std::getenv(variable_name);
    return raw_value == nullptr ? std::filesystem::path{} : std::filesystem::path{raw_value};
#endif
}

[[nodiscard]] static std::filesystem::path compiledPath(const char* value) {
    if(value == nullptr) {
        return {};
    }

    const std::string path_value{value};
    return path_value.empty() ? std::filesystem::path{} : std::filesystem::path{path_value};
}

[[nodiscard]] static std::filesystem::path resolvePythonHome() {
    if(const auto python_home = environmentPath("OPENGEOLAB_PYTHON_HOME"); !python_home.empty()) {
        return python_home;
    }

    if(const auto python_home = environmentPath("PYTHONHOME"); !python_home.empty()) {
        return python_home;
    }

#ifdef OPENGEOLAB_PYTHON_HOME
    if(const auto python_home = compiledPath(OPENGEOLAB_PYTHON_HOME); !python_home.empty()) {
        return python_home;
    }
#endif

    return {};
}

[[nodiscard]] static std::filesystem::path
resolvePythonExecutable(const std::filesystem::path& python_home) {
    if(const auto python_executable = environmentPath("OPENGEOLAB_PYTHON_EXECUTABLE");
       !python_executable.empty()) {
        return python_executable;
    }

#ifdef OPENGEOLAB_PYTHON_EXECUTABLE
    if(const auto python_executable = compiledPath(OPENGEOLAB_PYTHON_EXECUTABLE);
       !python_executable.empty()) {
        return python_executable;
    }
#endif

    if(python_home.empty()) {
        return {};
    }

#if defined(_WIN32)
    return python_home / "python.exe";
#else
    return python_home / "bin/python3";
#endif
}

[[nodiscard]] static std::wstring pathToWideString(const std::filesystem::path& path) {
    return path.wstring();
}

[[nodiscard]] static std::string pathToString(const std::filesystem::path& path) {
    return path.generic_string();
}

[[nodiscard]] static std::vector<std::filesystem::path>
buildModuleSearchPaths(const std::filesystem::path& application_root,
                       const std::filesystem::path& runtime_root,
                       const std::filesystem::path& plugin_root,
                       const std::filesystem::path& python_home) {
    const std::string python_version_tag =
        std::to_string(PY_MAJOR_VERSION) + std::to_string(PY_MINOR_VERSION);

    std::vector<std::filesystem::path> module_search_paths;
    module_search_paths.reserve(9);
    module_search_paths.emplace_back(python_home / ("python" + python_version_tag + ".zip"));
    module_search_paths.emplace_back(python_home / "DLLs");
    module_search_paths.emplace_back(python_home / "Lib");
    module_search_paths.emplace_back(application_root);
    module_search_paths.emplace_back(runtime_root);
    module_search_paths.emplace_back(plugin_root);
    module_search_paths.emplace_back(python_home);
    module_search_paths.emplace_back(python_home / "Lib" / "site-packages");
#ifdef OPENGEOLAB_PYVENV_SITE_PACKAGES
    module_search_paths.emplace_back(compiledPath(OPENGEOLAB_PYVENV_SITE_PACKAGES));
#endif
    return module_search_paths;
}

[[noreturn]] static void
throwPythonStatusError(const char* operation, const PyStatus& status, const std::string& context) {
    std::ostringstream message;
    message << operation << " failed";
    if(status.err_msg != nullptr) {
        message << ": " << status.err_msg;
    }
    if(!context.empty()) {
        message << '\n' << context;
    }
    throw std::runtime_error(message.str());
}

static void setConfigString(PyConfig& config, wchar_t*& field, const std::filesystem::path& value) {
    if(value.empty()) {
        return;
    }

    const std::wstring wide_value = pathToWideString(value);
    const PyStatus status = PyConfig_SetString(&config, &field, wide_value.c_str());
    if(PyStatus_Exception(status) != 0) {
        throwPythonStatusError("PyConfig_SetString", status, pathToString(value));
    }
}

static void appendModuleSearchPath(PyConfig& config, const std::filesystem::path& value) {
    if(value.empty()) {
        return;
    }

    const std::wstring wide_value = pathToWideString(value);
    const PyStatus status =
        PyWideStringList_Append(&config.module_search_paths, wide_value.c_str());
    if(PyStatus_Exception(status) != 0) {
        throwPythonStatusError("PyWideStringList_Append", status, pathToString(value));
    }
}

struct InitializationContext {
    std::filesystem::path applicationRoot;
    std::filesystem::path runtimeRoot;
    std::filesystem::path pluginRoot;
    std::filesystem::path pythonHome;
    std::filesystem::path pythonExecutable;
    std::vector<std::filesystem::path> moduleSearchPaths;
};

[[nodiscard]] static std::string
describeInitializationContext(const InitializationContext& context) {
    std::ostringstream description;
    description << "application_root=" << pathToString(context.applicationRoot) << '\n'
                << "runtime_root=" << pathToString(context.runtimeRoot) << '\n'
                << "plugin_root=" << pathToString(context.pluginRoot) << '\n'
                << "python_home=" << pathToString(context.pythonHome) << '\n'
                << "python_executable=" << pathToString(context.pythonExecutable) << '\n'
                << "module_search_paths:";
    for(const auto& module_search_path : context.moduleSearchPaths) {
        description << '\n' << "  - " << pathToString(module_search_path);
    }
    return description.str();
}

EmbeddedPythonRuntime::EmbeddedPythonRuntime(std::filesystem::path application_root,
                                             std::filesystem::path runtime_root,
                                             std::filesystem::path plugin_root)
    : m_applicationRoot(std::move(application_root)), m_runtimeRoot(std::move(runtime_root)),
      m_pluginRoot(std::move(plugin_root)) {
    initialize();
}

EmbeddedPythonRuntime::~EmbeddedPythonRuntime() = default;

void EmbeddedPythonRuntime::initialize() {
    const auto python_home = resolvePythonHome();
    const auto python_executable = resolvePythonExecutable(python_home);
    const auto module_search_paths =
        buildModuleSearchPaths(m_applicationRoot, m_runtimeRoot, m_pluginRoot, python_home);
    const InitializationContext context{.applicationRoot = m_applicationRoot,
                                        .runtimeRoot = m_runtimeRoot,
                                        .pluginRoot = m_pluginRoot,
                                        .pythonHome = python_home,
                                        .pythonExecutable = python_executable,
                                        .moduleSearchPaths = module_search_paths};

    if(python_home.empty()) {
        throw std::runtime_error("Unable to resolve embedded Python home.\n" +
                                 describeInitializationContext(context));
    }

    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    try {
        config.install_signal_handlers = 0;
        config.parse_argv = 0;
        config.module_search_paths_set = 1;

        setConfigString(config, config.home, python_home);
        setConfigString(config, config.program_name, python_executable);
        setConfigString(config, config.executable, python_executable);

        for(const auto& module_search_path : module_search_paths) {
            appendModuleSearchPath(config, module_search_path);
        }
    } catch(...) {
        PyConfig_Clear(&config);
        throw;
    }

    m_impl = std::make_unique<Impl>(&config);

    auto os = Py::module_::import("os");
    auto environment = os.attr("environ");
    environment["OPENGEOLAB_APPLICATION_ROOT"] = pathToString(m_applicationRoot);
    environment["OPENGEOLAB_RUNTIME_ROOT"] = pathToString(m_runtimeRoot);
    environment["OPENGEOLAB_PLUGIN_ROOT"] = pathToString(m_pluginRoot);
    environment["OPENGEOLAB_PYTHON_HOME"] = pathToString(python_home);
    environment["OPENGEOLAB_PYTHON_EXECUTABLE"] = pathToString(python_executable);
#ifdef OPENGEOLAB_PYVENV_SITE_PACKAGES
    environment["OPENGEOLAB_PYVENV_SITE_PACKAGES"] =
        pathToString(compiledPath(OPENGEOLAB_PYVENV_SITE_PACKAGES));
#endif

    auto runtime_module = Py::module_::import("opengeolab_runtime");
    m_impl->processFunction = runtime_module.attr("process");
}

[[nodiscard]] std::string EmbeddedPythonRuntime::process(std::string_view request_json) {
    try {
        const Py::gil_scoped_acquire acquire;
        return Py::cast<std::string>(m_impl->processFunction(Py::str(request_json)));
    } catch(const Py::error_already_set& error) {
        throw std::runtime_error(error.what());
    }
}

} // namespace OpenGeoLab::Python
