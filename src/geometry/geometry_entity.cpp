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

GeometryEntity::GeometryEntity()
    : m_entityId(generateEntityId()), m_entityUID(INVALID_ENTITY_UID),
      m_entityType(EntityType::None) {}

GeometryEntity::GeometryEntity(const TopoDS_Shape& shape, EntityType type)
    : m_entityId(generateEntityId()), m_shape(shape) {
    // Auto-detect type if not specified
    if(type == EntityType::None && !shape.IsNull()) {
        m_entityType = detectEntityType(shape);
    } else {
        m_entityType = type;
    }

    // Generate type-scoped UID
    m_entityUID = generateEntityUID(m_entityType);
}

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
    case TopAbs_COMPOUND:
    case TopAbs_COMPSOLID:
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
    if(m_shape.IsNull()) {
        m_boundingBox = BoundingBox3D();
        m_boundingBoxValid = false;
        return;
    }

    Bnd_Box occ_box;
    BRepBndLib::Add(m_shape, occ_box);

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

// =============================================================================
// GeometryManager Implementation
// =============================================================================

void GeometryManager::registerEntity(const GeometryEntityPtr& entity) {
    if(!entity || entity->entityId() == INVALID_ENTITY_ID) {
        return;
    }

    // Register in primary map
    m_entitiesById[entity->entityId()] = entity;

    // Register in type-UID map
    if(entity->entityUID() != INVALID_ENTITY_UID) {
        const uint64_t type_uid_key =
            (static_cast<uint64_t>(entity->entityType()) << 56) | entity->entityUID();
        m_entitiesByTypeUID[type_uid_key] = entity;
    }

    // Register in shape hash map
    if(entity->hasShape()) {
        const size_t shape_hash = computeShapeHash(entity->shape());
        m_entitiesByShapeHash[shape_hash] = entity;
    }
}

bool GeometryManager::unregisterEntity(const GeometryEntityPtr& entity) {
    if(!entity) {
        return false;
    }

    auto it = m_entitiesById.find(entity->entityId());
    if(it == m_entitiesById.end()) {
        return false;
    }

    // Remove from primary map
    m_entitiesById.erase(it);

    // Remove from type-UID map
    if(entity->entityUID() != INVALID_ENTITY_UID) {
        const uint64_t type_uid_key =
            (static_cast<uint64_t>(entity->entityType()) << 56) | entity->entityUID();
        m_entitiesByTypeUID.erase(type_uid_key);
    }

    // Remove from shape hash map
    if(entity->hasShape()) {
        const size_t shape_hash = computeShapeHash(entity->shape());
        m_entitiesByShapeHash.erase(shape_hash);
    }

    return true;
}

void GeometryManager::clear() {
    m_entitiesById.clear();
    m_entitiesByTypeUID.clear();
    m_entitiesByShapeHash.clear();
}

GeometryEntityPtr GeometryManager::findById(EntityId id) const {
    auto it = m_entitiesById.find(id);
    return (it != m_entitiesById.end()) ? it->second : nullptr;
}

GeometryEntityPtr GeometryManager::findByTypeAndUID(EntityType type, EntityUID uid) const {
    const uint64_t type_uid_key = (static_cast<uint64_t>(type) << 56) | uid;
    auto it = m_entitiesByTypeUID.find(type_uid_key);
    return (it != m_entitiesByTypeUID.end()) ? it->second : nullptr;
}

GeometryEntityPtr GeometryManager::findByShape(const TopoDS_Shape& shape) const {
    if(shape.IsNull()) {
        return nullptr;
    }
    const size_t shape_hash = computeShapeHash(shape);
    auto it = m_entitiesByShapeHash.find(shape_hash);
    return (it != m_entitiesByShapeHash.end()) ? it->second : nullptr;
}

std::vector<GeometryEntityPtr> GeometryManager::getEntitiesByType(EntityType type) const {
    std::vector<GeometryEntityPtr> result;
    result.reserve(m_entitiesById.size() / 4); // Rough estimate

    for(const auto& [id, entity] : m_entitiesById) {
        if(entity->entityType() == type) {
            result.push_back(entity);
        }
    }

    return result;
}

std::vector<GeometryEntityPtr> GeometryManager::getAllEntities() const {
    std::vector<GeometryEntityPtr> result;
    result.reserve(m_entitiesById.size());

    for(const auto& [id, entity] : m_entitiesById) {
        result.push_back(entity);
    }

    return result;
}

size_t GeometryManager::entityCountByType(EntityType type) const {
    return std::count_if(m_entitiesById.begin(), m_entitiesById.end(),
                         [type](const auto& pair) { return pair.second->entityType() == type; });
}

GeometryEntityPtr GeometryManager::createEntityFromShape(const TopoDS_Shape& shape,
                                                         const GeometryEntityPtr& parent) {
    auto entity = std::make_shared<GeometryEntity>(shape);

    if(parent) {
        parent->addChild(entity);
    }

    registerEntity(entity);
    return entity;
}

GeometryEntityPtr GeometryManager::importShape(const TopoDS_Shape& shape) {
    if(shape.IsNull()) {
        return nullptr;
    }
    return importShapeRecursive(shape, nullptr);
}

GeometryEntityPtr GeometryManager::importShapeRecursive(const TopoDS_Shape& shape, // NOLINT
                                                        const GeometryEntityPtr& parent) {
    // Create entity for current shape
    auto entity = createEntityFromShape(shape, parent);

    // Process children for compound shapes
    if(shape.ShapeType() == TopAbs_COMPOUND || shape.ShapeType() == TopAbs_COMPSOLID) {
        for(TopoDS_Iterator it(shape); it.More(); it.Next()) {
            importShapeRecursive(it.Value(), entity);
        }
    }

    return entity;
}

size_t GeometryManager::computeShapeHash(const TopoDS_Shape& shape) {
    // Use std::hash with shape's TShape pointer for hashing
    // This provides a unique hash based on the underlying shape data
    return std::hash<void*>{}(shape.TShape().get());
}

} // namespace OpenGeoLab::Geometry
