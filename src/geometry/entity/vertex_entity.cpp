/**
 * @file vertex_entity.cpp
 * @brief Implementation of VertexEntity point operations
 */

#include "vertex_entity.hpp"
#include "util/point_vector3d.hpp"
#include <BRep_Tool.hxx>
namespace OpenGeoLab::Geometry {
VertexEntity::VertexEntity(const TopoDS_Vertex& vertex)
    : GeometryEntityImpl(EntityType::Vertex), m_vertex(vertex) {}

Util::Pt3d VertexEntity::point() const {
    gp_Pnt occ_point = BRep_Tool::Pnt(m_vertex);
    return Util::Pt3d(occ_point.X(), occ_point.Y(), occ_point.Z());
}

gp_Pnt VertexEntity::occPoint() const { return BRep_Tool::Pnt(m_vertex); }
} // namespace OpenGeoLab::Geometry