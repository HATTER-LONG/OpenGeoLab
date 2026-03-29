/**
 * @file embedded_python_runtime.hpp
 * @brief Hosts an embedded Python interpreter.
 */

#pragma once

#include <opengeolab/core/progress_callback.hpp>
#include <opengeolab/python_embed/python_embed_export.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace OpenGeoLab::PythonEmbed {
/** @brief Reuse Core progress callback; Python bridge ignores the cancellation return. */
using ProgressCallback = Core::ProgressCallback;

/**
 * @brief Provides a reusable embedded Python runtime that routes JSON through
 * opengeolab_runtime.py.
 */
class OPENGEOLAB_PYTHON_EMBED_EXPORT EmbeddedPythonRuntime {
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
     * @param progress_callback Optional callback for progress reporting.
     *   Constructed outside GIL scope; internally wrapped as py::cpp_function
     *   within GIL scope for safe Python invocation.
     * @return JSON response returned by the Python runtime entry point.
     */
    [[nodiscard]] std::string process(std::string_view request_json,
                                      ProgressCallback progress_callback = nullptr);

private:
    void initialize();

private:
    struct Impl;

    std::filesystem::path m_applicationRoot;
    std::filesystem::path m_runtimeRoot;
    std::filesystem::path m_pluginRoot;
    std::unique_ptr<Impl> m_impl;
};
} // namespace OpenGeoLab::PythonEmbed