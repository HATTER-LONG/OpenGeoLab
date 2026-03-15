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
     * @brief Forward a high-level request object into the shared command service.
     * @param request_json Parsed request containing module, action, and param.
     * @return JSON response suitable for UI or Python consumption.
     */
    [[nodiscard]] auto process(const nlohmann::json& request_json) const -> nlohmann::json;
};

} // namespace OGL::PythonWrapper
