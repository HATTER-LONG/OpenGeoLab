/**
 * @file geometry_entity.cpp
 * @brief Implementation of GeometryEntity and GeometryManager
 */

#include "geometry_entity.hpp"

#include "../geometry_documentImpl.hpp"

#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Iterator.hxx>

namespace OpenGeoLab::Geometry {

// =============================================================================
// GeometryEntity Implementation
// =============================================================================

GeometryEntity::~GeometryEntity() { detachAllRelations(); }

GeometryEntity::GeometryEntity(EntityType type)
    : m_entityId(generateEntityId()), m_entityUID(generateEntityUID(type)), m_entityType(type) {}

EntityType GeometryEntity::detectEntityType(const TopoDS_Shape& shape) {
    if(shape.IsNull()) {
        return EntityType::None;
    }

    switch(shape.ShapeType()) {
    case TopAbs_VERTEX:
        return EntityType::Vertex;
    case TopAbs_EDGE:
        return EntityType::Edge;
    case TopAbs_WIRE:
        return EntityType::Wire;
    case TopAbs_FACE:
        return EntityType::Face;
    case TopAbs_SHELL:
        return EntityType::Shell;
    case TopAbs_SOLID:
        return EntityType::Solid;
    case TopAbs_COMPSOLID:
        return EntityType::CompSolid;
    case TopAbs_COMPOUND:
        return EntityType::Compound;
    default:
        return EntityType::None;
    }
}

BoundingBox3D GeometryEntity::boundingBox() const {
    if(!m_boundingBoxValid) {
        computeBoundingBox();
    }
    return m_boundingBox;
}

void GeometryEntity::computeBoundingBox() const {
    if(shape().IsNull()) {
        m_boundingBox = BoundingBox3D();
        m_boundingBoxValid = false;
        return;
    }

    Bnd_Box occ_box;
    BRepBndLib::Add(shape(), occ_box);

    if(occ_box.IsVoid()) {
        m_boundingBox = BoundingBox3D();
        m_boundingBoxValid = false;
        return;
    }

    double xmin, ymin, zmin, xmax, ymax, zmax;
    occ_box.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    m_boundingBox = BoundingBox3D(Point3D(xmin, ymin, zmin), Point3D(xmax, ymax, zmax));
    m_boundingBoxValid = true;
}

void GeometryEntity::invalidateBoundingBox() { m_boundingBoxValid = false; }

void GeometryEntity::detachAllRelations() {
    const auto doc = document();
    if(!doc) {
        return;
    }
    doc->relationships().detachEntity(*this);
}

} // namespace OpenGeoLab::Geometry
