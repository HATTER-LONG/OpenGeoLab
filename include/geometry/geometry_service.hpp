/**
 * @file geometry_service.hpp
 * @brief Geometry service interface for backend geometry operations
 *
 * GeometryService is the main entry point for geometry operations from QML/UI.
 * It routes requests to specialized components (creator, editor) and manages
 * the active GeometryDocument.
 */

#pragma once

#include "app/service.hpp"
#include "geometry/geometry_document.hpp"

#include <kangaroo/util/component_factory.hpp>
#include <nlohmann/json.hpp>

namespace OpenGeoLab::Geometry {

/**
 * @brief Service for geometry operations from QML frontend
 *
 * Handles requests for:
 * - Document management (new, clear)
 * - Geometry creation (point, line, box, etc.)
 * - Geometry modification (trim, offset, etc.)
 *
 * All operations work with the current document from GeometryDocumentManager.
 */
class GeometryService : public App::IService {
public:
    GeometryService() = default;
    ~GeometryService() override = default;

    /**
     * @brief Process a geometry operation request
     * @param module_name Service identifier (should be "GeometryService")
     * @param params JSON parameters with "action" key specifying the operation
     * @param progress_reporter Progress callback interface
     * @return JSON result with operation status and any created entity info
     *
     * Supported actions:
     * - "newDocument": Create a new empty document
     * - "createPoint": Create a point entity
     * - "createLine": Create a line entity
     * - "createBox": Create a box solid entity
     * - "trim": Trim geometry (not yet implemented)
     * - "offset": Offset geometry (not yet implemented)
     */
    [[nodiscard]] nlohmann::json
    processRequest(const std::string& module_name,
                   const nlohmann::json& params,
                   App::IProgressReporterPtr progress_reporter) override;

private:
    /**
     * @brief Create a new empty document
     * @param progress_reporter Progress callback
     * @return JSON result with status
     */
    [[nodiscard]] nlohmann::json handleNewDocument(App::IProgressReporterPtr progress_reporter);

    /**
     * @brief Route to geometry creator component
     * @param action The creation action (createPoint, createLine, etc.)
     * @param params Parameters for the creation
     * @param progress_reporter Progress callback
     * @return JSON result with created entity info
     */
    [[nodiscard]] nlohmann::json handleCreateAction(const std::string& action,
                                                    const nlohmann::json& params,
                                                    App::IProgressReporterPtr progress_reporter);

    /**
     * @brief Route to geometry editor component
     * @param action The edit action (trim, offset, etc.)
     * @param params Parameters for the edit operation
     * @param progress_reporter Progress callback
     * @return JSON result with operation status
     */
    [[nodiscard]] nlohmann::json handleEditAction(const std::string& action,
                                                  const nlohmann::json& params,
                                                  App::IProgressReporterPtr progress_reporter);
};

/**
 * @brief Singleton factory for GeometryService
 */
class GeometryServiceFactory : public App::IServiceSigletonFactory {
public:
    GeometryServiceFactory() = default;
    ~GeometryServiceFactory() override = default;

    tObjectSharedPtr instance() const override;
};

} // namespace OpenGeoLab::Geometry
