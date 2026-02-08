/**
 * @file edge_entity.hpp
 * @brief Edge (curve) geometry entity
 *
 * EdgeEntity wraps an OpenCASCADE TopoDS_Edge and provides access to
 * its underlying 3D curve geometry and topological properties.
 */

#pragma once

#include "geometry_entityImpl.hpp"
#include <Geom_Curve.hxx>
#include <TopoDS_Edge.hxx>

namespace OpenGeoLab::Geometry {

class EdgeEntity;
using EdgeEntityPtr = std::shared_ptr<EdgeEntity>;

/**
 * @brief Geometry entity representing an edge (curve segment)
 *
 * EdgeEntity represents a bounded curve in 3D space, typically bounded
 * by vertices at its endpoints. Edges form the boundaries of faces and
 * can be combined into wires.
 */
class EdgeEntity : public GeometryEntityImpl {
public:
    explicit EdgeEntity(const TopoDS_Edge& edge);
    ~EdgeEntity() override = default;

    [[nodiscard]] bool canAddChildType(EntityType child_type) const override {
        return child_type == EntityType::Vertex;
    }

    [[nodiscard]] bool canAddParentType(EntityType parent_type) const override {
        return parent_type == EntityType::Wire || parent_type == EntityType::Compound;
    }

    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_edge; }

    [[nodiscard]] bool hasShape() const override { return !m_edge.IsNull(); }
    /**
     * @brief Get the typed OCC edge
     * @return Const reference to TopoDS_Edge
     */
    [[nodiscard]] const TopoDS_Edge& edge() const { return m_edge; }

    // -------------------------------------------------------------------------
    // Geometry Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get the underlying 3D curve
     * @return Handle to Geom_Curve, may be null for degenerated edges
     */
    [[nodiscard]] Handle(Geom_Curve) curve() const;
    /**
     * @brief Get curve parameter range
     * @param first Output first parameter
     * @param last Output last parameter
     */
    void parameterRange(double& first, double& last) const;
    /**
     * @brief Evaluate point on edge at parameter
     * @param u Parameter value
     * @return Point3D at parameter
     */
    [[nodiscard]] Point3D pointAt(double u) const;
    /**
     * @brief Get tangent vector at parameter
     * @param u Parameter value
     * @return Tangent direction (normalized)
     */
    [[nodiscard]] Vector3D tangentAt(double u) const;

    /**
     * @brief Get edge length
     * @return Curve length
     */
    [[nodiscard]] double length() const;
    /**
     * @brief Check if edge is degenerated (zero length)
     * @return true if degenerated
     */
    [[nodiscard]] bool isDegenerated() const;
    /**
     * @brief Check if edge is closed (forms a loop)
     * @return true if start and end vertex are the same
     */
    [[nodiscard]] bool isClosed() const;
    /**
     * @brief Get start point of edge
     * @return Start point coordinates
     */
    [[nodiscard]] Point3D startPoint() const;

    /**
     * @brief Get end point of edge
     * @return End point coordinates
     */
    [[nodiscard]] Point3D endPoint() const;

    /**
     * @brief Get mid point of edge
     * @return Mid point coordinates
     */
    [[nodiscard]] Point3D midPoint() const;

private:
    TopoDS_Edge m_edge;
};
} // namespace OpenGeoLab::Geometry