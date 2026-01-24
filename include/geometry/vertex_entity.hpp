#pragma once

#include "geometry/geometry_entity.hpp"
#include <TopoDS_Vertex.hxx>

namespace OpenGeoLab::Geometry {
class VertexEntity;
using VertexEntityPtr = std::shared_ptr<VertexEntity>;

class VertexEntity : public GeometryEntity {
public:
    explicit VertexEntity(const TopoDS_Vertex& vertex);
    ~VertexEntity() override = default;

    [[nodiscard]] EntityType entityType() const override { return EntityType::Vertex; }

    [[nodiscard]] const char* typeName() const override { return "Vertex"; }

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