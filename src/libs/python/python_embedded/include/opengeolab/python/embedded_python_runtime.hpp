/**
 * @file EmbeddedPythonRuntime.hpp
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
     * @brief Constructs and initializes the embedded runtime.
     * @param application_root Directory containing the executable and native Python wrapper module.
     * @param runtime_root Directory containing opengeolab_runtime.py.
     * @param plugin_root Directory containing Python plugins.
     */
    EmbeddedPythonRuntime(std::filesystem::path application_root,
                          std::filesystem::path runtime_root,
                          std::filesystem::path plugin_root);

    ~EmbeddedPythonRuntime();

    /**
     * @brief Processes a JSON request by calling the Python runtime.
     * @param request_json Raw JSON request string.
     * @return JSON response string from the Python runtime.
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
