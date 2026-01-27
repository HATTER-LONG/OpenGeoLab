/**
 * @file geometry_creator.hpp
 * @brief Geometry creation component for primitive shapes
 *
 * GeometryCreator handles creation of basic geometric entities (point, line, box)
 * using OpenCASCADE and adds them to the active document.
 */

#pragma once

#include "geometry/geometry_document.hpp"
#include "geometry/part_entity.hpp"

#include <nlohmann/json.hpp>

#include <gp_Pnt.hxx>

namespace OpenGeoLab::Geometry {

/**
 * @brief Component for creating geometry primitives
 *
 * Provides static methods to create various geometric entities and add them
 * to the specified GeometryDocument. Each creation method:
 * - Creates the OCC shape
 * - Wraps it in a PartEntity
 * - Adds to the document with parent-child relationships
 */
class GeometryCreator {
public:
    /**
     * @brief Create a point entity
     * @param doc Target document
     * @param name Display name for the entity
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @return Created PartEntity, or nullptr on failure
     */
    [[nodiscard]] static PartEntityPtr
    createPoint(GeometryDocumentPtr doc, const std::string& name, double x, double y, double z);

    /**
     * @brief Create a point entity from gp_Pnt
     * @param doc Target document
     * @param name Display name for the entity
     * @param point OCC point
     * @return Created PartEntity, or nullptr on failure
     */
    [[nodiscard]] static PartEntityPtr
    createPoint(GeometryDocumentPtr doc, const std::string& name, const gp_Pnt& point);

    /**
     * @brief Create a line entity between two points
     * @param doc Target document
     * @param name Display name for the entity
     * @param start_x Start X coordinate
     * @param start_y Start Y coordinate
     * @param start_z Start Z coordinate
     * @param end_x End X coordinate
     * @param end_y End Y coordinate
     * @param end_z End Z coordinate
     * @return Created PartEntity, or nullptr on failure
     */
    [[nodiscard]] static PartEntityPtr createLine(GeometryDocumentPtr doc,
                                                  const std::string& name,
                                                  double start_x,
                                                  double start_y,
                                                  double start_z,
                                                  double end_x,
                                                  double end_y,
                                                  double end_z);

    /**
     * @brief Create a box solid entity
     * @param doc Target document
     * @param name Display name for the entity
     * @param origin_x Origin X coordinate
     * @param origin_y Origin Y coordinate
     * @param origin_z Origin Z coordinate
     * @param dim_x Width (X dimension)
     * @param dim_y Height (Y dimension)
     * @param dim_z Depth (Z dimension)
     * @return Created PartEntity, or nullptr on failure
     */
    [[nodiscard]] static PartEntityPtr createBox(GeometryDocumentPtr doc,
                                                 const std::string& name,
                                                 double origin_x,
                                                 double origin_y,
                                                 double origin_z,
                                                 double dim_x,
                                                 double dim_y,
                                                 double dim_z);

    /**
     * @brief Create geometry from JSON parameters
     * @param doc Target document
     * @param action The creation action (createPoint, createLine, createBox)
     * @param params JSON parameters containing coordinates and name
     * @return Created PartEntity, or nullptr on failure
     *
     * Expected JSON formats:
     *
     * createPoint:
     * {
     *   "name": "Point_1",
     *   "coordinates": { "x": 0.0, "y": 0.0, "z": 0.0 }
     * }
     *
     * createLine:
     * {
     *   "name": "Line_1",
     *   "start": { "x": 0.0, "y": 0.0, "z": 0.0 },
     *   "end": { "x": 10.0, "y": 0.0, "z": 0.0 }
     * }
     *
     * createBox:
     * {
     *   "name": "Box_1",
     *   "origin": { "x": 0.0, "y": 0.0, "z": 0.0 },
     *   "dimensions": { "x": 10.0, "y": 10.0, "z": 10.0 }
     * }
     */
    [[nodiscard]] static PartEntityPtr createFromJson(GeometryDocumentPtr doc,
                                                      const std::string& action,
                                                      const nlohmann::json& params);

private:
    GeometryCreator() = delete; ///< Static-only class
};

} // namespace OpenGeoLab::Geometry
