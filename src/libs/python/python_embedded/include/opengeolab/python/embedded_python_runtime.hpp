/**
 * @file embedded_python_runtime.hpp
 * @brief Hosts an embedded Python interpreter for the lightweight QML shell.
 */

#pragma once

#include <opengeolab/python/python_export.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace OpenGeoLab::Python {

/**
 * @brief Provides a reusable embedded Python runtime that routes JSON through
 * opengeolab_runtime.py.
 */
class OPENGEOLAB_PYTHON_EXPORT EmbeddedPythonRuntime {
public:
    /**
     * @brief Create the embedded runtime using explicit application, runtime,
     * and plugin roots.
     * @param application_root Root directory of the hosting application.
     * @param runtime_root Directory that contains Python runtime modules such as
     * opengeolab_runtime.py.
     * @param plugin_root Directory that contains Python plugin modules.
     */
    EmbeddedPythonRuntime(std::filesystem::path application_root,
                          std::filesystem::path runtime_root,
                          std::filesystem::path plugin_root);

    ~EmbeddedPythonRuntime();

    /**
     * @brief Forward a JSON request into the embedded Python runtime.
     * @param request_json JSON envelope consumed by opengeolab_runtime.process.
     * @return JSON response returned by the Python runtime entry point.
     */
    [[nodiscard]] std::string process(std::string_view request_json);

private:
    struct Impl;

    void initialize();

    std::filesystem::path m_applicationRoot;
    std::filesystem::path m_runtimeRoot;
    std::filesystem::path m_pluginRoot;
    std::unique_ptr<Impl> m_impl;
};

} // namespace OpenGeoLab::Python
