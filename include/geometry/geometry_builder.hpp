/**
 * @file geometry_builder.hpp
 * @brief Service for programmatic geometry creation.
 *
 * Provides functionality to create primitive geometries (box, cylinder, etc.)
 * using OpenCASCADE and store them in GeometryStore.
 */
#pragma once

#include "app/service.hpp"

#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab {
namespace Geometry {

/**
 * @brief Service for creating primitive geometry shapes.
 *
 * Handles creation of boxes, cylinders, spheres, etc.
 * Uses OpenCASCADE for shape generation and tessellation.
 */
class GeometryBuilder : public App::ServiceBase {
public:
    GeometryBuilder() = default;
    virtual ~GeometryBuilder() = default;

    /**
     * @brief Process geometry creation requests.
     * @param module_name Module name (AddBox, AddCylinder, etc.).
     * @param params Creation parameters (dimensions, position, name).
     * @param reporter Optional progress reporter.
     * @return JSON with geometry summary on success.
     */
    nlohmann::json processRequest(const std::string& module_name,
                                  const nlohmann::json& params,
                                  App::ProgressReporterPtr reporter) override;

private:
    /**
     * @brief Create a box shape and add to GeometryStore.
     * @param params Box parameters (name, originX/Y/Z, width, height, depth).
     * @return JSON result with success status and geometry info.
     */
    nlohmann::json createBox(const nlohmann::json& params);
};

/**
 * @brief Singleton factory for GeometryBuilder service.
 */
class GeometryBuilderFactory : public App::ServiceBaseSingletonFactory {
public:
    GeometryBuilderFactory() = default;
    virtual ~GeometryBuilderFactory() = default;

    tObjectSharedPtr instance() const override;
};

} // namespace Geometry
} // namespace OpenGeoLab
