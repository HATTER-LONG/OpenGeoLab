/**
 * @file solid_entity.hpp
 * @brief Solid (3D body) geometry entity
 *
 * SolidEntity wraps an OpenCASCADE TopoDS_Solid, representing a
 * watertight 3D volume bounded by shells.
 */

#pragma once

#include "geometry_entity.hpp"
#include <TopoDS_Solid.hxx>

namespace OpenGeoLab::Geometry {

class SolidEntity;
using SolidEntityPtr = std::shared_ptr<SolidEntity>;

/**
 * @brief Geometry entity representing a solid (3D volume)
 *
 * SolidEntity represents a watertight 3D volume bounded by one or
 * more shells. The outer shell defines the solid boundary, while
 * inner shells define cavities.
 */
class SolidEntity : public GeometryEntity {
public:
    explicit SolidEntity(const TopoDS_Solid& solid);
    ~SolidEntity() override = default;

    [[nodiscard]] EntityType entityType() const override { return EntityType::Solid; }

    [[nodiscard]] const char* typeName() const override { return "Solid"; }

    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_solid; }

    /**
     * @brief Get the typed OCC solid
     * @return Const reference to TopoDS_Solid
     */
    [[nodiscard]] const TopoDS_Solid& solid() const { return m_solid; }
    // -------------------------------------------------------------------------
    // Geometry Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get solid volume
     * @return Volume value
     */
    [[nodiscard]] double volume() const;

    /**
     * @brief Get surface area of solid
     * @return Total surface area
     */
    [[nodiscard]] double surfaceArea() const;

    /**
     * @brief Get center of mass
     * @return Center point
     */
    [[nodiscard]] Point3D centerOfMass() const;

    // -------------------------------------------------------------------------
    // Topology Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get number of faces
     * @return Face count
     */
    [[nodiscard]] size_t faceCount() const;

    /**
     * @brief Get number of edges
     * @return Edge count
     */
    [[nodiscard]] size_t edgeCount() const;

    /**
     * @brief Get number of vertices
     * @return Vertex count
     */
    [[nodiscard]] size_t vertexCount() const;

private:
    TopoDS_Solid m_solid;
};
} // namespace OpenGeoLab::Geometry