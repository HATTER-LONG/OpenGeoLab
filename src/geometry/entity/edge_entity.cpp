/**
 * @file edge_entity.cpp
 * @brief Implementation of EdgeEntity curve operations
 */

#include "edge_entity.hpp"
#include "util/point_vector3d.hpp"
#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <Geom_Curve.hxx>
#include <TopExp.hxx>
#include <TopoDS_Vertex.hxx>

namespace OpenGeoLab::Geometry {
EdgeEntity::EdgeEntity(const TopoDS_Edge& edge)
    : GeometryEntityImpl(EntityType::Edge), m_edge(edge) {}

Handle(Geom_Curve) EdgeEntity::curve() const {
    double first, last;
    return BRep_Tool::Curve(m_edge, first, last);
}
void EdgeEntity::parameterRange(double& first, double& last) const {
    BRep_Tool::Curve(m_edge, first, last);
}
Util::Pt3d EdgeEntity::pointAt(double u) const {
    Handle(Geom_Curve) crv = curve();
    if(crv.IsNull()) {
        return Util::Pt3d();
    }
    gp_Pnt p = crv->Value(u);
    return Util::Pt3d(p.X(), p.Y(), p.Z());
}
Util::Vec3d EdgeEntity::tangentAt(double u) const {
    Handle(Geom_Curve) crv = curve();
    if(crv.IsNull()) {
        return Util::Vec3d();
    }

    gp_Pnt p;
    gp_Vec tangent;
    crv->D1(u, p, tangent);

    if(tangent.Magnitude() > 1e-10) {
        tangent.Normalize();
        return Util::Vec3d(tangent.X(), tangent.Y(), tangent.Z());
    }

    return Util::Vec3d();
}
double EdgeEntity::length() const {
    GProp_GProps props;
    BRepGProp::LinearProperties(m_edge, props);
    return props.Mass();
}

bool EdgeEntity::isClosed() const {
    TopoDS_Vertex v1, v2;
    TopExp::Vertices(m_edge, v1, v2);
    return v1.IsSame(v2);
}

bool EdgeEntity::isDegenerated() const { return BRep_Tool::Degenerated(m_edge); }

Util::Pt3d EdgeEntity::startPoint() const {
    double first, last;
    parameterRange(first, last);
    return pointAt(first);
}

Util::Pt3d EdgeEntity::endPoint() const {
    double first, last;
    parameterRange(first, last);
    return pointAt(last);
}

Util::Pt3d EdgeEntity::midPoint() const {
    double first, last;
    parameterRange(first, last);
    return pointAt((first + last) / 2.0);
}

} // namespace OpenGeoLab::Geometry