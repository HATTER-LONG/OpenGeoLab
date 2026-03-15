/**
 * @file GeometryModel.hpp
 * @brief Geometry domain model used to validate the modular OpenGeoLab pipeline.
 */

#pragma once

#include <ogl/geometry/export.hpp>

#include <nlohmann/json.hpp>

#include <string>

namespace OGL::Geometry {

/**
 * @brief Descriptor used to construct a geometry model.
 */
struct OGL_GEOMETRY_EXPORT GeometryDescriptor {
    std::string modelName;
    int bodyCount{1};
    std::string source;
};

/**
 * @brief Lightweight stand-in for a future OCC-backed geometry model.
 */
class OGL_GEOMETRY_EXPORT GeometryModel {
public:
    /**
     * @brief Construct a geometry model from a normalized descriptor.
     * @param descriptor Model metadata used across UI, Python, and component layers.
     */
    explicit GeometryModel(GeometryDescriptor descriptor);

    /**
     * @brief Get the model name visible to upper layers.
     * @return Stable model name.
     */
    [[nodiscard]] auto modelName() const -> const std::string&;

    /**
     * @brief Get the conceptual body count carried by this model.
     * @return Conceptual body count.
     */
    [[nodiscard]] auto bodyCount() const -> int;

    /**
     * @brief Get the request source that produced this model.
     * @return Source marker such as qml-ui or python.
     */
    [[nodiscard]] auto source() const -> const std::string&;

    /**
     * @brief Produce a human-readable description for UI and script output.
     * @return Concise summary string.
     */
    [[nodiscard]] auto summary() const -> std::string;

    /**
     * @brief Serialize the geometry model for cross-layer transport.
     * @return JSON representation of the model.
     */
    [[nodiscard]] auto toJson() const -> nlohmann::json;

private:
    GeometryDescriptor m_descriptor;
};

} // namespace OGL::Geometry
