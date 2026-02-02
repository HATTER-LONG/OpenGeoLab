/**
 * @file solid_entity.cpp
 * @brief Implementation of SolidEntity geometry computations
 */

#include "solid_entity.hpp"
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

namespace OpenGeoLab::Geometry {
SolidEntity::SolidEntity(const TopoDS_Solid& solid)
    : GeometryEntity(EntityType::Solid), m_solid(solid) {}

double SolidEntity::volume() const {
    GProp_GProps props;
    BRepGProp::VolumeProperties(m_solid, props);
    return props.Mass();
}
double SolidEntity::surfaceArea() const {
    GProp_GProps props;
    BRepGProp::SurfaceProperties(m_solid, props);
    return props.Mass();
}

Point3D SolidEntity::centerOfMass() const {
    GProp_GProps props;
    BRepGProp::VolumeProperties(m_solid, props);
    gp_Pnt center = props.CentreOfMass();
    return Point3D(center.X(), center.Y(), center.Z());
}

size_t SolidEntity::faceCount() const {
    size_t count = 0;
    for(TopExp_Explorer exp(m_solid, TopAbs_FACE); exp.More(); exp.Next()) {
        ++count;
    }
    return count;
}

size_t SolidEntity::edgeCount() const {
    TopTools_IndexedMapOfShape edges;
    TopExp::MapShapes(m_solid, TopAbs_EDGE, edges);
    return edges.Extent();
}

size_t SolidEntity::vertexCount() const {
    TopTools_IndexedMapOfShape vertices;
    TopExp::MapShapes(m_solid, TopAbs_VERTEX, vertices);
    return vertices.Extent();
}
} // namespace OpenGeoLab::Geometry