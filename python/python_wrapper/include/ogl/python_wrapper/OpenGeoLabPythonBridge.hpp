/**
 * @file OpenGeoLabPythonBridge.hpp
 * @brief High-level Python-facing bridge that routes requests into OpenGeoLab components.
 */

#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace ogl::python_wrapper {

/**
 * @brief Shared C++ bridge used by both the application layer and pybind11 module.
 */
class OpenGeoLabPythonBridge {
public:
    /**
     * @brief Construct a bridge and ensure placeholder geometry components are registered.
     */
    OpenGeoLabPythonBridge();

    /**
     * @brief Forward a high-level call into the component dispatcher.
     * @param module_name Logical service module.
     * @param params Parsed request payload.
     * @return JSON response suitable for UI or Python consumption.
     */
    [[nodiscard]] auto call(const std::string& module_name, const nlohmann::json& params) const
        -> nlohmann::json;

    /**
     * @brief Produce a ready-to-run Python snippet for the placeholder geometry request.
     * @param model_name Placeholder model name.
     * @param body_count Conceptual body count.
     * @return Example Python automation script.
     */
    [[nodiscard]] auto suggestPlaceholderGeometryScript(const std::string& model_name,
                                                        int body_count) const -> std::string;
};

} // namespace ogl::python_wrapper