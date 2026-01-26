/**
 * @file primitive_factory.hpp
 * @brief Factory for creating basic geometric primitives
 *
 * PrimitiveFactory provides methods for creating standard geometric shapes
 * (box, sphere, cylinder, cone, torus) as PartEntity objects that are
 * automatically added to the current document.
 */

#pragma once

#include "geometry/geometry_document.hpp"
#include "geometry/part_entity.hpp"

namespace OpenGeoLab::Geometry {

/**
 * @brief Factory for creating primitive geometric shapes
 *
 * All creation methods return a PartEntity that wraps the created shape
 * and its entity hierarchy. The entities are automatically registered
 * with the specified or current GeometryDocument.
 */
class PrimitiveFactory {
public:
    /**
     * @brief Create a box (rectangular parallelepiped)
     * @param dx Length along X axis (must be > 0)
     * @param dy Length along Y axis (must be > 0)
     * @param dz Length along Z axis (must be > 0)
     * @param document Target document (uses current if null)
     * @return PartEntity containing the box, or nullptr on failure
     *
     * @note The box is centered at origin with corners at (-dx/2, -dy/2, -dz/2)
     *       and (dx/2, dy/2, dz/2).
     */
    [[nodiscard]] static PartEntityPtr
    createBox(double dx, double dy, double dz, GeometryDocumentPtr document = nullptr);

    /**
     * @brief Create a box from two corner points
     * @param p1 First corner point
     * @param p2 Opposite corner point
     * @param document Target document (uses current if null)
     * @return PartEntity containing the box, or nullptr on failure
     */
    [[nodiscard]] static PartEntityPtr
    createBox(const Point3D& p1, const Point3D& p2, GeometryDocumentPtr document = nullptr);

    /**
     * @brief Create a sphere
     * @param radius Sphere radius (must be > 0)
     * @param document Target document (uses current if null)
     * @return PartEntity containing the sphere, or nullptr on failure
     *
     * @note The sphere is centered at origin.
     */
    [[nodiscard]] static PartEntityPtr createSphere(double radius,
                                                    GeometryDocumentPtr document = nullptr);

    /**
     * @brief Create a sphere at a specified center
     * @param center Center point of the sphere
     * @param radius Sphere radius (must be > 0)
     * @param document Target document (uses current if null)
     * @return PartEntity containing the sphere, or nullptr on failure
     */
    [[nodiscard]] static PartEntityPtr
    createSphere(const Point3D& center, double radius, GeometryDocumentPtr document = nullptr);

    /**
     * @brief Create a cylinder along Z axis
     * @param radius Cylinder radius (must be > 0)
     * @param height Cylinder height (must be > 0)
     * @param document Target document (uses current if null)
     * @return PartEntity containing the cylinder, or nullptr on failure
     *
     * @note The cylinder is centered at origin, extending from z=-height/2 to z=height/2.
     */
    [[nodiscard]] static PartEntityPtr
    createCylinder(double radius, double height, GeometryDocumentPtr document = nullptr);

    /**
     * @brief Create a cone along Z axis
     * @param radius_bottom Bottom radius (must be >= 0)
     * @param radius_top Top radius (must be >= 0, different from radius_bottom)
     * @param height Cone height (must be > 0)
     * @param document Target document (uses current if null)
     * @return PartEntity containing the cone, or nullptr on failure
     *
     * @note The cone base is at z=0, apex/top at z=height.
     */
    [[nodiscard]] static PartEntityPtr createCone(double radius_bottom,
                                                  double radius_top,
                                                  double height,
                                                  GeometryDocumentPtr document = nullptr);

    /**
     * @brief Create a torus (donut shape)
     * @param major_radius Distance from center to tube center (must be > minor_radius)
     * @param minor_radius Tube radius (must be > 0)
     * @param document Target document (uses current if null)
     * @return PartEntity containing the torus, or nullptr on failure
     *
     * @note The torus is centered at origin in the XY plane.
     */
    [[nodiscard]] static PartEntityPtr
    createTorus(double major_radius, double minor_radius, GeometryDocumentPtr document = nullptr);

    /**
     * @brief Create a wedge (tapered box)
     * @param dx Base length along X (must be > 0)
     * @param dy Height along Y (must be > 0)
     * @param dz Depth along Z (must be > 0)
     * @param ltx Top length along X (must be >= 0 and <= dx)
     * @param document Target document (uses current if null)
     * @return PartEntity containing the wedge, or nullptr on failure
     */
    [[nodiscard]] static PartEntityPtr createWedge(
        double dx, double dy, double dz, double ltx, GeometryDocumentPtr document = nullptr);

private:
    /**
     * @brief Get or create the target document
     * @param document Specified document (may be null)
     * @return Valid document pointer
     */
    [[nodiscard]] static GeometryDocumentPtr ensureDocument(GeometryDocumentPtr document);

    /**
     * @brief Build a part from a shape
     * @param shape OCC shape to wrap
     * @param name Part name
     * @param document Target document
     * @return Created PartEntity or nullptr on failure
     */
    [[nodiscard]] static PartEntityPtr
    buildPart(const TopoDS_Shape& shape, const std::string& name, GeometryDocumentPtr document);
};

} // namespace OpenGeoLab::Geometry
