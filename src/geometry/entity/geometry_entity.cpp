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

#include <vector>

namespace OpenGeoLab::Geometry {

// =============================================================================
// GeometryEntity Implementation
// =============================================================================

GeometryEntity::~GeometryEntity() { detachAllRelations(); }

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

GeometryEntityWeakPtr GeometryEntity::anyParent() const {
    const auto doc = document();
    if(!doc) {
        return GeometryEntityWeakPtr{};
    }

    // Return any valid parent.
    for(const EntityId parent_id : doc->parentIds(entityId())) {
        if(auto p = doc->findById(parent_id)) {
            return GeometryEntityWeakPtr{p};
        }
    }
    return GeometryEntityWeakPtr{};
}

GeometryEntityWeakPtr GeometryEntity::singleParent() const {
    const auto doc = document();
    if(!doc) {
        return GeometryEntityWeakPtr{};
    }

    if(doc->parentEdgeCount(entityId()) != 1) {
        return GeometryEntityWeakPtr{};
    }

    const auto parents = doc->parentIds(entityId());
    if(parents.empty()) {
        return GeometryEntityWeakPtr{};
    }

    const EntityId parent_id = parents.front();
    const auto p = doc->findById(parent_id);
    return p ? GeometryEntityWeakPtr{p} : GeometryEntityWeakPtr{};
}

std::vector<GeometryEntityPtr> GeometryEntity::parents() const {
    std::vector<GeometryEntityPtr> result;
    visitParents([&](const GeometryEntityPtr& p) { result.push_back(p); });
    return result;
}

std::vector<GeometryEntityPtr> GeometryEntity::children() const {
    std::vector<GeometryEntityPtr> result;
    visitChildren([&](const GeometryEntityPtr& c) { result.push_back(c); });
    return result;
}

bool GeometryEntity::hasParentId(EntityId parent_id) const {
    const auto doc = document();
    if(!doc) {
        return false;
    }
    return doc->hasParentEdge(entityId(), parent_id);
}

bool GeometryEntity::hasChildId(EntityId child_id) const {
    const auto doc = document();
    if(!doc) {
        return false;
    }
    return doc->hasChildEdge(entityId(), child_id);
}

bool GeometryEntity::isRoot() const { return parentCount() == 0; }

bool GeometryEntity::hasChildren() const { return childCount() > 0; }

bool GeometryEntity::hasShape() const { return !shape().IsNull(); }

size_t GeometryEntity::parentCount() const {
    const auto doc = document();
    if(!doc) {
        return 0;
    }
    return doc->parentEdgeCount(entityId());
}

size_t GeometryEntity::childCount() const {
    const auto doc = document();
    if(!doc) {
        return 0;
    }
    return doc->childEdgeCount(entityId());
}

bool GeometryEntity::addChild(const GeometryEntityPtr& child) {
    if(!child) {
        return false;
    }
    return addChild(child->entityId());
}

bool GeometryEntity::removeChild(const GeometryEntityPtr& child) {
    if(!child) {
        return false;
    }
    return removeChild(child->entityId());
}

bool GeometryEntity::addParent(EntityId parent_id) {
    const auto doc = document();
    if(!doc) {
        return false;
    }
    return doc->addChildEdge(parent_id, entityId());
}

bool GeometryEntity::removeParent(EntityId parent_id) {
    const auto doc = document();
    if(!doc) {
        return false;
    }
    return doc->removeChildEdge(parent_id, entityId());
}

bool GeometryEntity::addChild(EntityId child_id) {
    const auto doc = document();
    if(!doc) {
        return false;
    }

    return doc->addChildEdge(entityId(), child_id);
}

bool GeometryEntity::removeChild(EntityId child_id) {
    if(child_id == INVALID_ENTITY_ID) {
        return false;
    }

    const auto doc = document();
    if(!doc) {
        return false;
    }
    return doc->removeChildEdge(entityId(), child_id);
}

void GeometryEntity::visitChildren(
    const std::function<void(const GeometryEntityPtr&)>& visitor) const {
    if(!visitor) {
        return;
    }

    const auto doc = document();
    if(!doc) {
        // Not indexed: nothing to resolve.
        return;
    }

    for(const EntityId child_id : doc->childIds(entityId())) {
        const auto child_entity = doc->findById(child_id);
        if(child_entity) {
            visitor(child_entity);
        }
    }
}

void GeometryEntity::visitParents(
    const std::function<void(const GeometryEntityPtr&)>& visitor) const {
    if(!visitor) {
        return;
    }

    const auto doc = document();
    if(!doc) {
        return;
    }

    for(const EntityId parent_id : doc->parentIds(entityId())) {
        const auto parent_entity = doc->findById(parent_id);
        if(parent_entity) {
            visitor(parent_entity);
        }
    }
}

void GeometryEntity::pruneExpiredRelations() const {
    const auto doc = document();
    if(!doc) {
        return;
    }
    for(const EntityId parent_id : doc->parentIds(entityId())) {
        if(!doc->findById(parent_id)) {
            (void)doc->removeChildEdge(parent_id, entityId());
        }
    }

    for(const EntityId child_id : doc->childIds(entityId())) {
        if(!doc->findById(child_id)) {
            (void)doc->removeChildEdge(entityId(), child_id);
        }
    }
}

void GeometryEntity::detachAllRelations() {
    const auto doc = document();
    if(!doc) {
        return;
    }

    doc->detachEntityRelations(entityId());
}

} // namespace OpenGeoLab::Geometry
