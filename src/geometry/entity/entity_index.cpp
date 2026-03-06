/**
 * @file entity_index.cpp
 * @brief High-performance geometry entity index with per-type slot buckets
 */

#include "entity_index.hpp"

#include <TopoDS_Shape.hxx>

namespace OpenGeoLab::Geometry {

// =============================================================================
// Lookup
// =============================================================================

GeometryEntityImplPtr EntityIndex::findByUIDAndType(EntityUID entity_uid,
                                                    EntityType entity_type) const {
    const size_t bi = bucketIndex(entity_type);
    if(bi >= K_BUCKET_COUNT || entity_uid == INVALID_ENTITY_UID) {
        return nullptr;
    }

    const auto slot_idx = static_cast<size_t>(entity_uid - 1);
    const auto& bucket = m_typeBuckets[bi];
    if(slot_idx >= bucket.size()) {
        return nullptr;
    }

    return bucket[slot_idx].m_entity; // nullptr if slot is empty
}

GeometryEntityImplPtr EntityIndex::findById(EntityId entity_id) const {
    if(entity_id == INVALID_ENTITY_ID) {
        return nullptr;
    }

    const auto it = m_idToRef.find(entity_id);
    if(it == m_idToRef.end()) {
        return nullptr;
    }

    return findByUIDAndType(it->second.m_uid, it->second.m_type);
}

GeometryEntityImplPtr EntityIndex::findByShape(const TopoDS_Shape& shape) const {
    if(shape.IsNull()) {
        return nullptr;
    }

    const auto it = m_byShape.find(shape);
    if(it == m_byShape.end()) {
        return nullptr;
    }

    const auto& handle = it->second;
    const size_t bi = bucketIndex(handle.m_type);
    if(bi >= K_BUCKET_COUNT || handle.m_uid == INVALID_ENTITY_UID) {
        m_byShape.erase(it);
        return nullptr;
    }

    const auto slot_idx = static_cast<size_t>(handle.m_uid - 1);
    const auto& bucket = m_typeBuckets[bi];
    if(slot_idx >= bucket.size()) {
        m_byShape.erase(it);
        return nullptr;
    }

    const Slot& slot = bucket[slot_idx];
    if(slot.m_generation != handle.m_generation || !slot.m_entity) {
        m_byShape.erase(it);
        return nullptr;
    }

    return slot.m_entity;
}

GeometryEntityImplPtr EntityIndex::findByKey(const EntityKey& key) const {
    if(!key.isValid()) {
        return nullptr;
    }
    // Use the type+uid path for O(1) lookup (faster than id-based).
    return findByUIDAndType(key.m_uid, key.m_type);
}

GeometryEntityImplPtr EntityIndex::findByRef(const EntityRef& ref) const {
    if(!ref.isValid()) {
        return nullptr;
    }
    return findByUIDAndType(ref.m_uid, ref.m_type);
}

EntityRef EntityIndex::resolveId(EntityId entity_id) const {
    const auto it = m_idToRef.find(entity_id);
    if(it != m_idToRef.end()) {
        return it->second;
    }
    return EntityRef{};
}

EntityKey EntityIndex::resolveIdToKey(EntityId entity_id) const {
    const auto it = m_idToRef.find(entity_id);
    if(it == m_idToRef.end()) {
        return EntityKey{};
    }
    return EntityKey{entity_id, it->second.m_uid, it->second.m_type};
}

EntityKey EntityIndex::resolveRefToKey(const EntityRef& ref) const {
    if(!ref.isValid()) {
        return EntityKey{};
    }

    const size_t bi = bucketIndex(ref.m_type);
    if(bi >= K_BUCKET_COUNT) {
        return EntityKey{};
    }

    const auto slot_idx = static_cast<size_t>(ref.m_uid - 1);
    const auto& bucket = m_typeBuckets[bi];
    if(slot_idx >= bucket.size()) {
        return EntityKey{};
    }

    const auto& entity = bucket[slot_idx].m_entity;
    if(!entity) {
        return EntityKey{};
    }

    return EntityKey{entity->entityId(), ref.m_uid, ref.m_type};
}

// =============================================================================
// Mutation
// =============================================================================

bool EntityIndex::addEntity(const GeometryEntityImplPtr& entity) {
    if(!entity) {
        return false;
    }

    const auto type = entity->entityType();
    const auto uid = entity->entityUID();
    const auto id = entity->entityId();

    const size_t bi = bucketIndex(type);
    if(bi >= K_BUCKET_COUNT || uid == INVALID_ENTITY_UID || id == INVALID_ENTITY_ID) {
        return false;
    }

    // Reject duplicate by id.
    if(m_idToRef.count(id) > 0) {
        return false;
    }

    auto& bucket = m_typeBuckets[bi];
    const auto slot_idx = static_cast<size_t>(uid - 1);

    // Grow the bucket if needed.
    if(slot_idx >= bucket.size()) {
        bucket.resize(slot_idx + 1);
    }

    Slot& slot = bucket[slot_idx];
    // Reject duplicate by (type, uid) – slot already occupied.
    if(slot.m_entity) {
        return false;
    }

    slot.m_entity = entity;

    m_idToRef.emplace(id, EntityRef{uid, type});

    const TopoDS_Shape& shape = entity->shape();
    if(!shape.IsNull()) {
        m_byShape.emplace(shape, ShapeHandle{type, uid, slot.m_generation});
    }

    ++m_aliveCount;
    ++m_countByType[bi];

    return true;
}

bool EntityIndex::removeEntity(const GeometryEntityImplPtr& entity) {
    if(!entity) {
        return false;
    }
    return removeEntity(entity->entityId());
}

bool EntityIndex::removeEntity(EntityUID entity_uid, EntityType entity_type) {
    auto entity = findByUIDAndType(entity_uid, entity_type);
    if(!entity) {
        return false;
    }
    return removeEntity(entity->entityId());
}

bool EntityIndex::removeEntity(EntityId entity_id) {
    const auto ref_it = m_idToRef.find(entity_id);
    if(ref_it == m_idToRef.end()) {
        return false;
    }

    const EntityRef ref = ref_it->second;
    const size_t bi = bucketIndex(ref.m_type);
    if(bi >= K_BUCKET_COUNT || ref.m_uid == INVALID_ENTITY_UID) {
        m_idToRef.erase(ref_it);
        return false;
    }

    auto& bucket = m_typeBuckets[bi];
    const auto slot_idx = static_cast<size_t>(ref.m_uid - 1);
    if(slot_idx >= bucket.size()) {
        m_idToRef.erase(ref_it);
        return false;
    }

    Slot& slot = bucket[slot_idx];
    if(!slot.m_entity) {
        m_idToRef.erase(ref_it);
        return false;
    }

    const GeometryEntityImplPtr entity = slot.m_entity;

    // Eagerly detach relationship edges before unindexing.
    entity->detachAllRelations();

    m_idToRef.erase(ref_it);

    const TopoDS_Shape& shape = entity->shape();
    if(!shape.IsNull()) {
        m_byShape.erase(shape);
    }

    // Invalidate the slot and bump generation for stale handle detection.
    slot.m_entity.reset();
    ++slot.m_generation;

    --m_aliveCount;
    if(m_countByType[bi] > 0) {
        --m_countByType[bi];
    }

    return true;
}

void EntityIndex::clear() {
    for(auto& bucket : m_typeBuckets) {
        bucket.clear();
    }
    m_idToRef.clear();
    m_byShape.clear();
    m_countByType.fill(0);
    m_aliveCount = 0;
}

// =============================================================================
// Enumeration
// =============================================================================

std::vector<GeometryEntityImplPtr> EntityIndex::snapshotEntities() const {
    std::vector<GeometryEntityImplPtr> result;
    result.reserve(m_aliveCount);
    for(const auto& bucket : m_typeBuckets) {
        for(const auto& slot : bucket) {
            if(slot.m_entity) {
                result.push_back(slot.m_entity);
            }
        }
    }
    return result;
}

size_t EntityIndex::entityCount() const { return m_aliveCount; }

size_t EntityIndex::entityCountByType(EntityType entity_type) const {
    const size_t bi = bucketIndex(entity_type);
    if(bi >= K_BUCKET_COUNT) {
        return 0;
    }
    return m_countByType[bi];
}

std::vector<GeometryEntityImplPtr> EntityIndex::entitiesByType(EntityType entity_type) const {
    const size_t bi = bucketIndex(entity_type);
    if(bi >= K_BUCKET_COUNT) {
        return {};
    }

    const auto& bucket = m_typeBuckets[bi];
    std::vector<GeometryEntityImplPtr> result;
    result.reserve(m_countByType[bi]);
    for(const auto& slot : bucket) {
        if(slot.m_entity) {
            result.push_back(slot.m_entity);
        }
    }
    return result;
}

} // namespace OpenGeoLab::Geometry
