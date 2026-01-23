/**
 * @file geometry_types.cpp
 * @brief Implementation of geometry ID generation and utility functions
 */

#include "geometry/geometry_types.hpp"

#include <array>
#include <atomic>

namespace OpenGeoLab::Geometry {

namespace {

/// Global atomic counter for EntityId generation
std::atomic<EntityId> g_next_entity_id{1};

/// Per-type atomic counters for EntityUID generation
/// Index corresponds to EntityType enum value
std::array<std::atomic<EntityUID>, 8> g_next_entity_uids = {
    std::atomic<EntityUID>{1}, // None
    std::atomic<EntityUID>{1}, // Vertex
    std::atomic<EntityUID>{1}, // Edge
    std::atomic<EntityUID>{1}, // Wire
    std::atomic<EntityUID>{1}, // Face
    std::atomic<EntityUID>{1}, // Shell
    std::atomic<EntityUID>{1}, // Solid
    std::atomic<EntityUID>{1}  // Compound
};

} // namespace

EntityId generateEntityId() { return g_next_entity_id.fetch_add(1, std::memory_order_relaxed); }

EntityUID generateEntityUID(EntityType type) {
    const auto index = static_cast<size_t>(type);
    if(index >= g_next_entity_uids.size()) {
        return INVALID_ENTITY_UID;
    }
    return g_next_entity_uids[index].fetch_add(1, std::memory_order_relaxed);
}

void resetEntityUIDGenerator(EntityType type) {
    const auto index = static_cast<size_t>(type);
    if(index < g_next_entity_uids.size()) {
        g_next_entity_uids[index].store(1, std::memory_order_relaxed);
    }
}

void resetEntityIdGenerator() { g_next_entity_id.store(1, std::memory_order_relaxed); }

} // namespace OpenGeoLab::Geometry