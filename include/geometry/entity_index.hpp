/**
 * @file entity_index.hpp
 * @brief High-performance index for geometry entity lookup
 *
 * EntityIndex provides O(1) lookup of entities by various keys:
 * - EntityId (global unique)
 * - EntityUID + EntityType (type-scoped unique)
 * - TopoDS_Shape (OCC shape reference)
 */

#pragma once

#include "geometry_entity.hpp"
#include <TopTools_ShapeMapHasher.hxx>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Geometry {

/**
 * @brief High-performance entity index with generational slot allocation
 *
 * EntityIndex maintains multiple lookup tables for fast entity retrieval.
 * It uses a slot-based allocation scheme with generation counters to detect
 * stale references and safely recycle storage.
 *
 * Features:
 * - O(1) lookup by EntityId, (EntityType, EntityUID), or TopoDS_Shape
 * - Automatic cleanup of stale index entries on lookup
 * - Thread-safe for concurrent reads (writes must be externally synchronized)
 */
class EntityIndex : public Kangaroo::Util::NonCopyMoveable {
public:
    explicit EntityIndex() = default;
    ~EntityIndex() = default;

    [[nodiscard]] bool addEntity(const GeometryEntityPtr& entity);
    [[nodiscard]] bool removeEntity(EntityId entity_id);
    [[nodiscard]] bool removeEntity(const GeometryEntityPtr& entity);
    [[nodiscard]] bool removeEntity(EntityUID entity_uid, EntityType entity_type);
    void clear();

    [[nodiscard]] GeometryEntityPtr findById(EntityId entity_id) const;
    [[nodiscard]] GeometryEntityPtr findByUIDAndType(EntityUID entity_uid,
                                                     EntityType entity_type) const;
    [[nodiscard]] GeometryEntityPtr findByShape(const TopoDS_Shape& shape) const;

    [[nodiscard]] size_t entityCount() const;
    [[nodiscard]] size_t entityCountByType(EntityType entity_type) const;

    /// Snapshot of currently alive entities (order unspecified).
    [[nodiscard]] std::vector<GeometryEntityPtr> snapshotEntities() const;

    /// Get all entities of a specific type.
    [[nodiscard]] std::vector<GeometryEntityPtr> entitiesByType(EntityType entity_type) const;

private:
    struct EntityTypeHash {
        size_t operator()(EntityType type) const noexcept {
            using Underlying = std::underlying_type_t<EntityType>;
            return std::hash<Underlying>{}(static_cast<Underlying>(type));
        }
    };

    struct TypeUIDHash {
        static void hashCombine(size_t& seed, size_t value) {
            seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        }

        size_t operator()(const std::pair<EntityType, EntityUID>& key) const noexcept {
            size_t seed = 0;
            hashCombine(seed, EntityTypeHash{}(key.first));
            hashCombine(seed, std::hash<EntityUID>()(key.second));
            return seed;
        }
    };

    struct IndexHandle {
        size_t m_slot{0};
        uint32_t m_generation{0};
    };

    struct Slot {
        GeometryEntityPtr m_entity;
        uint32_t m_generation{1};
    };

    std::vector<Slot> m_slots;
    std::vector<size_t> m_freeSlots;

    mutable std::unordered_map<EntityId, IndexHandle> m_byId;
    mutable std::unordered_map<std::pair<EntityType, EntityUID>, IndexHandle, TypeUIDHash>
        m_byTypeAndUID;
    mutable std::unordered_map<TopoDS_Shape, IndexHandle, TopTools_ShapeMapHasher> m_byShape;

    std::unordered_map<EntityType, size_t, EntityTypeHash> m_countByType;
    size_t m_aliveCount{0};
};
} // namespace OpenGeoLab::Geometry