#include "opengeolab/python/embedded_python_runtime.hpp"

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace Py = pybind11;

namespace OpenGeoLab::Python {

struct EmbeddedPythonRuntime::Impl {
    explicit Impl(PyConfig* config) : interpreter(config, 0, nullptr, false) {}

    Py::scoped_interpreter interpreter;
    Py::object processFunction;
};

#ifdef OPENGEOLAB_PYTHON_HOME
static constexpr auto DEFAULT_PYTHON_HOME = OPENGEOLAB_PYTHON_HOME;
#else
static constexpr auto DEFAULT_PYTHON_HOME = "";
#endif

#ifdef OPENGEOLAB_PYTHON_EXECUTABLE
static constexpr auto DEFAULT_PYTHON_EXECUTABLE = OPENGEOLAB_PYTHON_EXECUTABLE;
#else
static constexpr auto DEFAULT_PYTHON_EXECUTABLE = "";
#endif

// NOLINTBEGIN(misc-use-anonymous-namespace)
[[nodiscard]] static std::filesystem::path environmentPath(const char* variable_name) {
#ifdef _WIN32
    char* raw_value = nullptr;
    std::size_t len = 0;
    if(_dupenv_s(&raw_value, &len, variable_name) != 0 || raw_value == nullptr) {
        return {};
    }
    std::filesystem::path result(raw_value);
    std::free(raw_value);
    return result;
#else
    const char* raw_value = std::getenv(variable_name);
    if(raw_value == nullptr || std::string_view(raw_value).empty()) {
        return {};
    }
    return std::filesystem::path(raw_value);
#endif
}

[[nodiscard]] static std::filesystem::path compiledPath(const char* value) {
    if(value == nullptr || std::string_view(value).empty()) {
        return {};
    }

    return std::filesystem::path(value);
}

[[nodiscard]] static std::filesystem::path resolvePythonHome() {
    if(const auto configured_home = environmentPath("OPENGEOLAB_PYTHON_HOME");
       !configured_home.empty()) {
        return configured_home;
    }

    if(const auto pythonhome = environmentPath("PYTHONHOME"); !pythonhome.empty()) {
        return pythonhome;
    }

    return compiledPath(DEFAULT_PYTHON_HOME);
}

[[nodiscard]] static std::filesystem::path
resolvePythonExecutable(const std::filesystem::path& python_home) {
    if(const auto configured_executable = environmentPath("OPENGEOLAB_PYTHON_EXECUTABLE");
       !configured_executable.empty()) {
        return configured_executable;
    }

    if(const auto compiled_executable = compiledPath(DEFAULT_PYTHON_EXECUTABLE);
       !compiled_executable.empty()) {
        return compiled_executable;
    }

#ifdef _WIN32
    return python_home / "python.exe";
#else
    return python_home / "bin" / "python3";
#endif
}

[[nodiscard]] static std::wstring toWideString(const std::filesystem::path& path) {
    return path.wstring();
}

[[nodiscard]] static std::string pathToString(const std::filesystem::path& path) {
    return path.empty() ? std::string("<empty>") : path.string();
}

[[nodiscard]] static std::vector<std::filesystem::path>
buildModuleSearchPaths(const std::filesystem::path& python_home,
                       const std::filesystem::path& application_root,
                       const std::filesystem::path& runtime_root,
                       const std::filesystem::path& plugin_root) {
    std::vector<std::filesystem::path> search_paths;
    search_paths.reserve(9);
    search_paths.emplace_back(python_home / ("python" + std::to_string(PY_MAJOR_VERSION) +
                                             std::to_string(PY_MINOR_VERSION) + ".zip"));
    search_paths.emplace_back(python_home / "DLLs");
    search_paths.emplace_back(python_home / "Lib");
    search_paths.emplace_back(application_root);
    search_paths.emplace_back(runtime_root);
    search_paths.emplace_back(plugin_root);
    search_paths.emplace_back(python_home);
    search_paths.emplace_back(python_home / "Lib" / "site-packages");
#ifdef OPENGEOLAB_PYVENV_SITE_PACKAGES
    search_paths.emplace_back(OPENGEOLAB_PYVENV_SITE_PACKAGES);
#endif
    return search_paths;
}

static void throwPythonStatusError(const std::string& context, const PyStatus& status) {
    if(PyStatus_IsError(status) != 0 && status.err_msg != nullptr) {
        throw std::runtime_error(context + ": " + status.err_msg);
    }

    throw std::runtime_error(context + ": CPython reported an unknown initialization error.");
}

static void setConfigString(PyConfig& config,
                            wchar_t** destination,
                            const std::filesystem::path& value,
                            const char* description) {
    const std::wstring wide_value = toWideString(value);
    const PyStatus status = PyConfig_SetString(&config, destination, wide_value.c_str());
    if(PyStatus_Exception(status) != 0) {
        PyConfig_Clear(&config);
        throwPythonStatusError(std::string("Failed to configure Python ") + description, status);
    }
}

static void appendModuleSearchPath(PyConfig& config, const std::filesystem::path& path) {
    if(path.empty() || !std::filesystem::exists(path)) {
        return;
    }

    const std::wstring wide_path = toWideString(path);
    const PyStatus status = PyWideStringList_Append(&config.module_search_paths, wide_path.c_str());
    if(PyStatus_Exception(status) != 0) {
        PyConfig_Clear(&config);
        throwPythonStatusError("Failed to extend Python module_search_paths", status);
    }
}

[[nodiscard]] static std::string
describeInitializationContext(const std::filesystem::path& python_home,
                              const std::filesystem::path& python_executable,
                              const std::filesystem::path& application_root,
                              const std::filesystem::path& runtime_root,
                              const std::filesystem::path& plugin_root) {
    std::ostringstream stream;
    stream << "pythonHome=" << pathToString(python_home)
           << ", pythonExecutable=" << pathToString(python_executable)
           << ", applicationRoot=" << pathToString(application_root)
           << ", runtimeRoot=" << pathToString(runtime_root)
           << ", pluginRoot=" << pathToString(plugin_root);
    return stream.str();
}
// NOLINTEND(misc-use-anonymous-namespace)

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
    const auto search_paths =
        buildModuleSearchPaths(python_home, m_applicationRoot, m_runtimeRoot, m_pluginRoot);

    if(python_home.empty() || !std::filesystem::exists(python_home)) {
        throw std::runtime_error(
            "Embedded Python runtime could not resolve a usable Python home. Set "
            "OPENGEOLAB_PYTHON_HOME or ensure CMake discovers Python3. " +
            describeInitializationContext(python_home, python_executable, m_applicationRoot,
                                          m_runtimeRoot, m_pluginRoot));
    }

    try {
        PyConfig config;
        PyConfig_InitPythonConfig(&config);
        config.parse_argv = 0;
        config.install_signal_handlers = 0;
        config.module_search_paths_set = 1;

        setConfigString(config, &config.home, python_home, "home");
        if(!python_executable.empty() && std::filesystem::exists(python_executable)) {
            setConfigString(config, &config.program_name, python_executable, "program_name");
        }

        for(const auto& search_path : search_paths) {
            appendModuleSearchPath(config, search_path);
        }

        m_impl = std::make_unique<Impl>(&config);

        const Py::gil_scoped_acquire acquire;
        // Mutated through item assignment below to populate os.environ for the runtime.
        // NOLINTNEXTLINE(misc-const-correctness)
        Py::dict environment_map = Py::module_::import("os").attr("environ");
        environment_map["OPENGEOLAB_APPLICATION_ROOT"] = m_applicationRoot.string();
        environment_map["OPENGEOLAB_RUNTIME_ROOT"] = m_runtimeRoot.string();
        environment_map["OPENGEOLAB_PYTHON_HOME"] = python_home.string();
        environment_map["OPENGEOLAB_PLUGIN_ROOT"] = m_pluginRoot.string();

        m_impl->processFunction = Py::module_::import("opengeolab_runtime").attr("process");
    } catch(const Py::error_already_set& error) {
        throw std::runtime_error(
            std::string("Embedded Python runtime failed during module import: ") + error.what() +
            " (" +
            describeInitializationContext(python_home, python_executable, m_applicationRoot,
                                          m_runtimeRoot, m_pluginRoot) +
            ")");
    } catch(const std::exception& error) {
        throw std::runtime_error(
            std::string("Embedded Python runtime initialization failed: ") + error.what() + " (" +
            describeInitializationContext(python_home, python_executable, m_applicationRoot,
                                          m_runtimeRoot, m_pluginRoot) +
            ")");
    }
}

std::string EmbeddedPythonRuntime::process(std::string_view request_json) {
    try {
        const Py::gil_scoped_acquire acquire;
        return Py::cast<std::string>(m_impl->processFunction(Py::str(request_json)));
    } catch(const Py::error_already_set& error) {
        throw std::runtime_error(error.what());
    }
}

} // namespace OpenGeoLab::Python
