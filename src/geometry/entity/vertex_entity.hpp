/**
 * @file vertex_entity.hpp
 * @brief Vertex (point) geometry entity
 *
 * VertexEntity wraps an OpenCASCADE TopoDS_Vertex and provides
 * access to its 3D point coordinates.
 */

#pragma once

#include "geometry_entity.hpp"
#include <TopoDS_Vertex.hxx>

namespace OpenGeoLab::Geometry {

class VertexEntity;
using VertexEntityPtr = std::shared_ptr<VertexEntity>;

/**
 * @brief Geometry entity representing a vertex (point)
 *
 * VertexEntity is the simplest topological entity, representing a
 * single point in 3D space. Vertices are typically endpoints of edges.
 */
class VertexEntity : public GeometryEntity {
public:
    explicit VertexEntity(const TopoDS_Vertex& vertex);
    ~VertexEntity() override = default;

    [[nodiscard]] EntityType entityType() const override { return EntityType::Vertex; }

    [[nodiscard]] const char* typeName() const override { return "Vertex"; }

    [[nodiscard]] bool canAddChildType(EntityType /*child_type*/) const override { return false; }

    [[nodiscard]] bool canAddParentType(EntityType parent_type) const override {
        return parent_type == EntityType::Edge;
    }

    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_vertex; }
    /**
     * @brief Get the typed OCC vertex
     * @return Const reference to TopoDS_Vertex
     */
    [[nodiscard]] const TopoDS_Vertex& vertex() const { return m_vertex; }

    // -------------------------------------------------------------------------
    // Geometry Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get the 3D point location
     * @return Point3D coordinates
     */
    [[nodiscard]] Point3D point() const;

    /**
     * @brief Get the OCC gp_Pnt
     * @return gp_Pnt from BRep_Tool
     */
    [[nodiscard]] gp_Pnt occPoint() const;

private:
    TopoDS_Vertex m_vertex;
};
} // namespace OpenGeoLab::Geometry