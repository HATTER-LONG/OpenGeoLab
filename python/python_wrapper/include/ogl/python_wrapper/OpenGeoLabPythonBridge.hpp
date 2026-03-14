/**
 * @file OpenGeoLabPythonBridge.hpp
 * @brief High-level Python-facing bridge that routes requests into the shared command layer.
 */

#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace OGL::PythonWrapper {

/**
 * @brief Shared C++ bridge used by Python bindings to access the command pipeline.
 */
class OpenGeoLabPythonBridge {
public:
    /**
     * @brief Construct a bridge.
     */
    OpenGeoLabPythonBridge();

    /**
     * @brief Forward a high-level call into the shared command service.
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

} // namespace OGL::PythonWrapper