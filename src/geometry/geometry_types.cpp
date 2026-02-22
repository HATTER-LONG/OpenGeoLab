/**
 * @file geometry_types.cpp
 * @brief Implementation of geometry ID generation and utility functions
 */

#include "geometry/geometry_types.hpp"

#include <array>
#include <atomic>
#include <optional>
#include <stdexcept>
#include <unordered_map>

namespace OpenGeoLab::Geometry {

namespace {

/// Global atomic counter for EntityId generation
std::atomic<EntityId> g_next_entity_id{1};

/// Per-type atomic counters for EntityUID generation
/// Index corresponds to EntityType enum value
std::array<std::atomic<EntityUID>, 10> g_next_entity_uids = {
    std::atomic<EntityUID>{1}, // None
    std::atomic<EntityUID>{1}, // Vertex
    std::atomic<EntityUID>{1}, // Edge
    std::atomic<EntityUID>{1}, // Wire
    std::atomic<EntityUID>{1}, // Face
    std::atomic<EntityUID>{1}, // Shell
    std::atomic<EntityUID>{1}, // Solid
    std::atomic<EntityUID>{1}, // CompSolid
    std::atomic<EntityUID>{1}, // Compound
    std::atomic<EntityUID>{1}, // Part
};

} // namespace
std::optional<EntityType> entityTypeFromString(std::string_view value) noexcept {
    static const std::unordered_map<std::string_view, EntityType> type_map = {
        {"None", EntityType::None},         {"Vertex", EntityType::Vertex},
        {"Edge", EntityType::Edge},         {"Wire", EntityType::Wire},
        {"Face", EntityType::Face},         {"Shell", EntityType::Shell},
        {"Solid", EntityType::Solid},       {"CompSolid", EntityType::CompSolid},
        {"Compound", EntityType::Compound}, {"Part", EntityType::Part},
    };
    auto it = type_map.find(value);
    if(it != type_map.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<std::string> entityTypeToString(EntityType type) noexcept {
    switch(type) {
    case EntityType::None:
        return "None";
    case EntityType::Vertex:
        return "Vertex";
    case EntityType::Edge:
        return "Edge";
    case EntityType::Wire:
        return "Wire";
    case EntityType::Face:
        return "Face";
    case EntityType::Shell:
        return "Shell";
    case EntityType::Solid:
        return "Solid";
    case EntityType::CompSolid:
        return "CompSolid";
    case EntityType::Compound:
        return "Compound";
    case EntityType::Part:
        return "Part";
    default:
        break;
    }
    return std::nullopt;
}
EntityId generateEntityId() { return g_next_entity_id.fetch_add(1, std::memory_order_relaxed); }

EntityUID generateEntityUID(EntityType type) {
    const auto index = static_cast<size_t>(type);
    if(index >= g_next_entity_uids.size()) {
        throw std::invalid_argument("Invalid EntityType for UID generation");
    }
    return g_next_entity_uids[index].fetch_add(1, std::memory_order_relaxed);
}

uint32_t getMaxIdByType(EntityType type) {
    const auto index = static_cast<size_t>(type);
    if(index >= g_next_entity_uids.size()) {
        throw std::invalid_argument("Invalid EntityType for max UID retrieval");
    }
    return static_cast<uint32_t>(g_next_entity_uids[index].load(std::memory_order_relaxed) - 1);
}

void resetEntityUIDGenerator(EntityType type) {
    const auto index = static_cast<size_t>(type);
    if(index >= g_next_entity_uids.size()) {
        throw std::invalid_argument("Invalid EntityType for UID reset");
    }
    g_next_entity_uids[index].store(1, std::memory_order_relaxed);
}

void resetAllEntityUIDGenerators() {
    for(auto& counter : g_next_entity_uids) {
        counter.store(1, std::memory_order_relaxed);
    }
}

void resetEntityIdGenerator() { g_next_entity_id.store(1, std::memory_order_relaxed); }

} // namespace OpenGeoLab::Geometry