#pragma once

#include "wire_entity.hpp"
#include <Geom_Surface.hxx>
#include <TopoDS_Face.hxx>

namespace OpenGeoLab::Geometry {
class FaceEntity;
using FaceEntityPtr = std::shared_ptr<FaceEntity>;

class FaceEntity : public GeometryEntity {
public:
    explicit FaceEntity(const TopoDS_Face& face);
    ~FaceEntity() override = default;

    [[nodiscard]] EntityType entityType() const override { return EntityType::Face; }

    [[nodiscard]] const char* typeName() const override { return "Face"; }

    [[nodiscard]] const TopoDS_Shape& shape() const override { return m_face; }

    /**
     * @brief Get the typed OCC face
     * @return Const reference to TopoDS_Face
     */
    [[nodiscard]] const TopoDS_Face& face() const { return m_face; }

    // -------------------------------------------------------------------------
    // Geometry Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get the underlying surface geometry
     * @return Handle to Geom_Surface
     */
    [[nodiscard]] Handle(Geom_Surface) surface() const;

    /**
     * @brief Get UV parameter bounds
     * @param uMin, uMax U parameter range
     * @param vMin, vMax V parameter range
     */
    void parameterBounds(double& u_min, double& u_max, double& v_min, double& v_max) const;

    /**
     * @brief Evaluate point on face at UV parameters
     * @param u U parameter
     * @param v V parameter
     * @return Point3D at (u,v)
     */
    [[nodiscard]] Point3D pointAt(double u, double v) const;

    /**
     * @brief Get surface normal at UV parameters
     * @param u U parameter
     * @param v V parameter
     * @return Unit normal vector
     */
    [[nodiscard]] Vector3D normalAt(double u, double v) const;

    /**
     * @brief Get face area
     * @return Surface area
     */
    [[nodiscard]] double area() const;

    /**
     * @brief Check face orientation
     * @return true if forward orientation
     */
    [[nodiscard]] bool isForward() const;

    // -------------------------------------------------------------------------
    // Topology Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get outer wire (boundary) of face
     * @return Outer wire entity or nullptr
     */
    [[nodiscard]] WireEntityPtr outerWire() const;

    /**
     * @brief Get all wires (outer + holes)
     * @return Vector of wire entities
     */
    [[nodiscard]] std::vector<WireEntityPtr> allWires() const;

    /**
     * @brief Get number of holes in face
     * @return Number of inner wires
     */
    [[nodiscard]] size_t holeCount() const;

    /**
     * @brief Find adjacent faces (sharing an edge)
     * @return Vector of face entities sharing edges with this face
     */
    [[nodiscard]] std::vector<FaceEntityPtr> adjacentFaces() const;

private:
    TopoDS_Face m_face;
};
} // namespace OpenGeoLab::Geometry