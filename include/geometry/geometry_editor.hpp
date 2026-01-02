/**
 * @file geometry_editor.hpp
 * @brief Geometry editing service for CAD operations
 *
 * Provides geometry manipulation operations including:
 * - Trim (cut geometry by plane/surface)
 * - Offset (shell/offset operations)
 * - Boolean operations (future)
 */

#pragma once

#include "app/service.hpp"

#include <TopoDS_Shape.hxx>

#include <kangaroo/util/component_factory.hpp>

namespace OpenGeoLab {
namespace Geometry {

/**
 * @brief Service for editing and modifying geometry
 *
 * Handles trim, offset, and other geometry modification operations.
 * Uses OpenCASCADE for shape manipulation.
 */
class GeometryEditor : public App::ServiceBase {
public:
    GeometryEditor() = default;
    virtual ~GeometryEditor() = default;

    /**
     * @brief Process geometry editing requests
     * @param moduleName Module name (Trim, Offset, etc.)
     * @param params Editing parameters
     * @param reporter Progress reporter
     * @return JSON with operation result
     */
    nlohmann::json processRequest(const std::string& moduleName,
                                  const nlohmann::json& params,
                                  App::ProgressReporterPtr reporter) override;

private:
    /**
     * @brief Perform trim operation
     * @param params Trim parameters (targetId, toolId, mode, keepOriginal)
     * @param reporter Progress reporter
     * @return JSON result
     */
    nlohmann::json performTrim(const nlohmann::json& params, App::ProgressReporterPtr reporter);

    /**
     * @brief Perform offset operation
     * @param params Offset parameters (targetId, distance, etc.)
     * @param reporter Progress reporter
     * @return JSON result
     */
    nlohmann::json performOffset(const nlohmann::json& params, App::ProgressReporterPtr reporter);
};

/**
 * @brief Singleton factory for GeometryEditor service
 */
class GeometryEditorFactory : public App::ServiceBaseSingletonFactory {
public:
    GeometryEditorFactory() = default;
    virtual ~GeometryEditorFactory() = default;

    tObjectSharedPtr instance() const override;
};

} // namespace Geometry
} // namespace OpenGeoLab
