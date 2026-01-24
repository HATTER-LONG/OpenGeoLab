/**
 * @file geometry_entity.cpp
 * @brief Implementation of GeometryEntity and GeometryManager
 */

#include "geometry/geometry_entity.hpp"

#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Iterator.hxx>

#include <algorithm>

namespace OpenGeoLab::Geometry {

// =============================================================================
// GeometryEntity Implementation
// =============================================================================

GeometryEntity::GeometryEntity(EntityType type)
    : m_entityId(generateEntityId()), m_entityUID(generateEntityUID(type)) {}

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

void GeometryEntity::addChild(const GeometryEntityPtr& child) {
    if(child) {
        m_children.push_back(child);
        child->setParent(weak_from_this());
    }
}

bool GeometryEntity::removeChild(const GeometryEntityPtr& child) {
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if(it != m_children.end()) {
        (*it)->setParent(GeometryEntityWeakPtr{});
        m_children.erase(it);
        return true;
    }
    return false;
}

} // namespace OpenGeoLab::Geometry
