/**
 * @file geometry_entity.cpp
 * @brief Implementation of GeometryEntity and GeometryManager
 */

#include "geometry/geometry_entity.hpp"

#include "geometry/geometry_document.hpp"

#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Iterator.hxx>

#include <unordered_set>
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
    if(m_parentIds.empty() || !doc) {
        return GeometryEntityWeakPtr{};
    }

    // Return any valid parent.
    for(auto it = m_parentIds.begin(); it != m_parentIds.end();) {
        const EntityId parent_id = *it;
        if(auto p = doc->findById(parent_id)) {
            return GeometryEntityWeakPtr{p};
        }

        // Best-effort local cleanup for stale ids.
        it = m_parentIds.erase(it);
    }
    return GeometryEntityWeakPtr{};
}

GeometryEntityWeakPtr GeometryEntity::singleParent() const {
    const auto doc = document();
    if(!doc) {
        return GeometryEntityWeakPtr{};
    }

    if(m_parentIds.size() != 1) {
        return GeometryEntityWeakPtr{};
    }

    const EntityId parent_id = *m_parentIds.begin();
    const auto p = doc->findById(parent_id);
    if(!p) {
        // stale; best-effort local cleanup
        (void)m_parentIds.erase(parent_id);
        return GeometryEntityWeakPtr{};
    }
    return GeometryEntityWeakPtr{p};
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
    return m_parentIds.find(parent_id) != m_parentIds.end();
}

bool GeometryEntity::hasChildId(EntityId child_id) const {
    return m_childIds.find(child_id) != m_childIds.end();
}

bool GeometryEntity::isRoot() const { return m_parentIds.empty(); }

bool GeometryEntity::hasChildren() const { return !m_childIds.empty(); }

size_t GeometryEntity::parentCount() const { return m_parentIds.size(); }

size_t GeometryEntity::childCount() const { return m_childIds.size(); }

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
        // best-effort local cleanup
        return removeChildNoSync(child_id);
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

    for(auto it = m_childIds.begin(); it != m_childIds.end();) {
        const EntityId child_id = *it;
        const auto child_entity = doc->findById(child_id);
        if(!child_entity) {
            it = m_childIds.erase(it);
            continue;
        }
        ++it;
        visitor(child_entity);
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

    for(auto it = m_parentIds.begin(); it != m_parentIds.end();) {
        const EntityId parent_id = *it;
        const auto parent_entity = doc->findById(parent_id);
        if(!parent_entity) {
            it = m_parentIds.erase(it);
            continue;
        }
        ++it;
        visitor(parent_entity);
    }
}

void GeometryEntity::pruneExpiredRelations() const {
    const auto doc = document();
    if(!doc) {
        return;
    }

    for(auto it = m_parentIds.begin(); it != m_parentIds.end();) {
        if(!doc->findById(*it)) {
            it = m_parentIds.erase(it);
        } else {
            ++it;
        }
    }

    for(auto it = m_childIds.begin(); it != m_childIds.end();) {
        if(!doc->findById(*it)) {
            it = m_childIds.erase(it);
        } else {
            ++it;
        }
    }
}

bool GeometryEntity::addChildNoSync(EntityId child_id) {
    return m_childIds.insert(child_id).second;
}

bool GeometryEntity::removeChildNoSync(EntityId child_id) { return m_childIds.erase(child_id) > 0; }

bool GeometryEntity::addParentNoSync(EntityId parent_id) {
    return m_parentIds.insert(parent_id).second;
}

bool GeometryEntity::removeParentNoSync(EntityId parent_id) {
    return m_parentIds.erase(parent_id) > 0;
}

void GeometryEntity::detachAllRelations() {
    const auto doc = document();
    if(!doc) {
        m_parentIds.clear();
        m_childIds.clear();
        return;
    }

    // Detach from parents
    for(const EntityId parent_id : m_parentIds) {
        if(const auto parent_entity = doc->findById(parent_id)) {
            (void)parent_entity->removeChildNoSync(entityId());
        }
    }
    m_parentIds.clear();

    // Detach children
    for(const EntityId child_id : m_childIds) {
        if(const auto child_entity = doc->findById(child_id)) {
            (void)child_entity->removeParentNoSync(entityId());
        }
    }
    m_childIds.clear();
}

} // namespace OpenGeoLab::Geometry
