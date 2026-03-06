/**
 * @file vertex_entity.hpp
 * @brief Vertex (point) geometry entity
 *
 * VertexEntity wraps an OpenCASCADE TopoDS_Vertex and provides
 * access to its 3D point coordinates.
 */

#pragma once

#include "geometry_entityImpl.hpp"
#include "util/point_vector3d.hpp"
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
class VertexEntity : public GeometryEntityImpl {
public:
    explicit VertexEntity(const TopoDS_Vertex& vertex);
    ~VertexEntity() override = default;

    [[nodiscard]] bool canAddChildType(EntityType /*child_type*/) const override { return false; }

    [[nodiscard]] bool canAddParentType(EntityType parent_type) const override {
        return parent_type == EntityType::Edge || parent_type == EntityType::Compound;
    }

    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_vertex; }

    [[nodiscard]] bool hasShape() const override { return !m_vertex.IsNull(); }
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
     * @return pt3d coordinates
     */
    [[nodiscard]] Util::Pt3d point() const;

    /**
     * @brief Get the OCC gp_Pnt
     * @return gp_Pnt from BRep_Tool
     */
    [[nodiscard]] gp_Pnt occPoint() const;

private:
    TopoDS_Vertex m_vertex;
};
} // namespace OpenGeoLab::Geometry