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

GeometryEntityWeakPtr GeometryEntity::parent() const {
    for(const auto& p : m_parents) {
        if(!p.expired()) {
            return p;
        }
    }
    return GeometryEntityWeakPtr{};
}

size_t GeometryEntity::parentCount() const {
    // Count only alive parents
    size_t count = 0;
    for(const auto& p : m_parents) {
        if(!p.expired()) {
            ++count;
        }
    }
    return count;
}

size_t GeometryEntity::childReferenceCount(const GeometryEntityPtr& child) const {
    if(!child) {
        return 0;
    }

    const auto it = m_childRefCounts.find(child.get());
    if(it == m_childRefCounts.end()) {
        return 0;
    }
    return it->second;
}

size_t GeometryEntity::totalChildReferenceCount() const {
    size_t total = 0;
    for(const auto& [_, cnt] : m_childRefCounts) {
        total += cnt;
    }
    return total;
}

void GeometryEntity::addChild(const GeometryEntityPtr& child) {
    if(!child) {
        return;
    }

    auto it = m_childRefCounts.find(child.get());
    if(it != m_childRefCounts.end()) {
        ++it->second;
        return;
    }

    m_children.push_back(child);
    m_childRefCounts.emplace(child.get(), 1);

    child->addParentRef(weak_from_this());
}

bool GeometryEntity::removeChild(const GeometryEntityPtr& child) {
    if(!child) {
        return false;
    }

    auto count_it = m_childRefCounts.find(child.get());
    if(count_it == m_childRefCounts.end()) {
        return false;
    }

    if(count_it->second > 1) {
        --count_it->second;
        return true;
    }

    // Last reference: remove child entry
    m_childRefCounts.erase(count_it);

    auto it = std::find(m_children.begin(), m_children.end(), child);
    if(it != m_children.end()) {
        m_children.erase(it);
    }

    child->removeParentRef(this);
    return true;
}

void GeometryEntity::setParent(const GeometryEntityWeakPtr& parent) {
    m_parents.clear();
    if(!parent.expired()) {
        m_parents.push_back(parent);
    }
}

void GeometryEntity::addParentRef(const GeometryEntityWeakPtr& parent) {
    if(parent.expired()) {
        return;
    }

    pruneExpiredParents();

    const auto parent_ptr = parent.lock();
    if(!parent_ptr) {
        return;
    }

    for(const auto& existing : m_parents) {
        if(auto ex = existing.lock(); ex && ex.get() == parent_ptr.get()) {
            return;
        }
    }

    m_parents.push_back(parent);
}

void GeometryEntity::removeParentRef(const GeometryEntity* parent) {
    if(!parent) {
        return;
    }

    m_parents.erase(
        std::remove_if(m_parents.begin(), m_parents.end(),
                       [parent](const GeometryEntityWeakPtr& p) {
                           auto sp = p.lock();
                           return !sp || sp.get() == parent;
                       }),
        m_parents.end());
}

void GeometryEntity::pruneExpiredParents() {
    m_parents.erase(
        std::remove_if(m_parents.begin(), m_parents.end(),
                       [](const GeometryEntityWeakPtr& p) { return p.expired(); }),
        m_parents.end());
}

} // namespace OpenGeoLab::Geometry
