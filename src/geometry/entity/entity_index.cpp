/**
 * @file entity_index.cpp
 * @brief High-performance geometry entity index
 */

#include "entity_index.hpp"

#include <TopoDS_Shape.hxx>

#include <utility>

namespace OpenGeoLab::Geometry {
namespace {
[[nodiscard]] std::pair<EntityType, EntityUID> makeTypeUidKey(const GeometryEntity& entity) {
    return {entity.entityType(), entity.entityUID()};
}
} // namespace

GeometryEntityPtr EntityIndex::findById(EntityId entity_id) const {
    const auto it = m_byId.find(entity_id);
    if(it == m_byId.end()) {
        return nullptr;
    }

    const auto handle = it->second;
    if(handle.m_slot >= m_slots.size()) {
        // Stale handle (should not happen, but keep safe)
        m_byId.erase(it);
        return nullptr;
    }

    const Slot& slot = m_slots[handle.m_slot];
    if(slot.m_generation != handle.m_generation || !slot.m_entity) {
        m_byId.erase(it);
        return nullptr;
    }

    return slot.m_entity;
}

GeometryEntityPtr EntityIndex::findByUIDAndType(EntityUID entity_uid,
                                                EntityType entity_type) const {
    const auto key = std::make_pair(entity_type, entity_uid);
    const auto it = m_byTypeAndUID.find(key);
    if(it == m_byTypeAndUID.end()) {
        return nullptr;
    }

    const auto handle = it->second;
    if(handle.m_slot >= m_slots.size()) {
        m_byTypeAndUID.erase(it);
        return nullptr;
    }

    const Slot& slot = m_slots[handle.m_slot];
    if(slot.m_generation != handle.m_generation || !slot.m_entity) {
        m_byTypeAndUID.erase(it);
        return nullptr;
    }

    return slot.m_entity;
}

GeometryEntityPtr EntityIndex::findByShape(const TopoDS_Shape& shape) const {
    if(shape.IsNull()) {
        return nullptr;
    }

    const auto it = m_byShape.find(shape);
    if(it == m_byShape.end()) {
        return nullptr;
    }

    const auto handle = it->second;
    if(handle.m_slot >= m_slots.size()) {
        m_byShape.erase(it);
        return nullptr;
    }

    const Slot& slot = m_slots[handle.m_slot];
    if(slot.m_generation != handle.m_generation || !slot.m_entity) {
        m_byShape.erase(it);
        return nullptr;
    }

    return slot.m_entity;
}

bool EntityIndex::addEntity(const GeometryEntityPtr& entity) {
    if(!entity) {
        return false;
    }

    // Reject duplicates by id
    if(findById(entity->entityId())) {
        return false;
    }

    // Reject duplicates by (type, uid)
    if(findByUIDAndType(entity->entityUID(), entity->entityType())) {
        return false;
    }

    // Allocate a slot (reuse if possible)
    size_t slot_index = 0;
    if(!m_freeSlots.empty()) {
        slot_index = m_freeSlots.back();
        m_freeSlots.pop_back();
    } else {
        slot_index = m_slots.size();
        m_slots.emplace_back();
    }

    Slot& slot = m_slots[slot_index];
    // Slot might have been used before; bump generation if it still contains something.
    // (Normally it is cleared on removal.)
    if(slot.m_entity) {
        ++slot.m_generation;
    }
    slot.m_entity = entity;

    const IndexHandle handle{slot_index, slot.m_generation};

    m_byId.emplace(entity->entityId(), handle);
    m_byTypeAndUID.emplace(makeTypeUidKey(*entity), handle);

    const TopoDS_Shape& shape = entity->shape();
    if(!shape.IsNull()) {
        m_byShape.emplace(shape, handle);
    }

    ++m_aliveCount;
    ++m_countByType[entity->entityType()];

    return true;
}

bool EntityIndex::removeEntity(const GeometryEntityPtr& entity) {
    if(!entity) {
        return false;
    }
    return removeEntity(entity->entityId());
}

bool EntityIndex::removeEntity(EntityUID entity_uid, EntityType entity_type) {
    const auto key = std::make_pair(entity_type, entity_uid);
    const auto it = m_byTypeAndUID.find(key);
    if(it == m_byTypeAndUID.end()) {
        return false;
    }

    const auto handle = it->second;
    if(handle.m_slot >= m_slots.size()) {
        m_byTypeAndUID.erase(it);
        return false;
    }

    Slot& slot = m_slots[handle.m_slot];
    if(slot.m_generation != handle.m_generation || !slot.m_entity) {
        m_byTypeAndUID.erase(it);
        return false;
    }

    return removeEntity(slot.m_entity->entityId());
}

bool EntityIndex::removeEntity(EntityId entity_id) {
    const auto it = m_byId.find(entity_id);
    if(it == m_byId.end()) {
        return false;
    }

    const auto handle = it->second;
    if(handle.m_slot >= m_slots.size()) {
        m_byId.erase(it);
        return false;
    }

    Slot& slot = m_slots[handle.m_slot];
    if(slot.m_generation != handle.m_generation || !slot.m_entity) {
        m_byId.erase(it);
        return false;
    }

    const GeometryEntityPtr entity = slot.m_entity;

    // Remove all index entries (best-effort; OK if already missing)
    m_byId.erase(it);
    m_byTypeAndUID.erase(makeTypeUidKey(*entity));

    const TopoDS_Shape& shape = entity->shape();
    if(!shape.IsNull()) {
        m_byShape.erase(shape);
    }

    // Invalidate the slot and recycle it
    slot.m_entity.reset();
    ++slot.m_generation;
    m_freeSlots.push_back(handle.m_slot);

    --m_aliveCount;
    auto type_count_it = m_countByType.find(entity->entityType());
    if(type_count_it != m_countByType.end() && type_count_it->second > 0) {
        --type_count_it->second;
        if(type_count_it->second == 0) {
            m_countByType.erase(type_count_it);
        }
    }

    return true;
}

void EntityIndex::clear() {
    m_byId.clear();
    m_byTypeAndUID.clear();
    m_byShape.clear();

    m_slots.clear();
    m_freeSlots.clear();

    m_countByType.clear();
    m_aliveCount = 0;
}

std::vector<GeometryEntityPtr> EntityIndex::snapshotEntities() const {
    std::vector<GeometryEntityPtr> result;
    result.reserve(m_aliveCount);
    for(const auto& slot : m_slots) {
        if(slot.m_entity) {
            result.push_back(slot.m_entity);
        }
    }
    return result;
}

size_t EntityIndex::entityCount() const { return m_aliveCount; }

size_t EntityIndex::entityCountByType(EntityType entity_type) const {
    const auto it = m_countByType.find(entity_type);
    if(it == m_countByType.end()) {
        return 0;
    }
    return it->second;
}

std::vector<GeometryEntityPtr> EntityIndex::entitiesByType(EntityType entity_type) const {
    std::vector<GeometryEntityPtr> result;
    result.reserve(entityCountByType(entity_type));
    for(const auto& slot : m_slots) {
        if(slot.m_entity && slot.m_entity->entityType() == entity_type) {
            result.push_back(slot.m_entity);
        }
    }
    return result;
}

} // namespace OpenGeoLab::Geometry
