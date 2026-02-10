/**
 * @file entity_index.hpp
 * @brief High-performance index for geometry entity lookup
 *
 * EntityIndex provides O(1) lookup of entities by various keys:
 * - EntityId  -> via m_idToRef hash map + per-type bucket (amortized O(1))
 * - EntityUID + EntityType -> direct array access O(1)
 * - EntityKey / EntityRef  -> delegates to the above
 * - TopoDS_Shape           -> hash map with generation-validated handle
 *
 * Storage uses per-type slot buckets: each EntityType has its own
 * `vector<Slot>` indexed by (uid - 1). Generation counters on slots
 * allow safe UID recycling in the future.
 */

#pragma once

#include "geometry_entityImpl.hpp"
#include <TopTools_ShapeMapHasher.hxx>
#include <array>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Geometry {

/**
 * @brief High-performance entity index with per-type slot buckets
 *
 * Each EntityType has a dedicated `vector<Slot>` bucket. EntityUID (1-based)
 * maps directly to slot index (uid - 1), giving true O(1) lookup by
 * (type, uid) without any hash table overhead.
 *
 * EntityId -> entity lookup resolves through an `id -> EntityRef` hash map
 * followed by the O(1) bucket access.
 *
 * Shape -> entity lookup uses a hash map with generation-validated handles
 * to detect stale entries.
 */
class EntityIndex : public Kangaroo::Util::NonCopyMoveable {
public:
    explicit EntityIndex() = default;
    ~EntityIndex() = default;

    [[nodiscard]] bool addEntity(const GeometryEntityImplPtr& entity);
    [[nodiscard]] bool removeEntity(EntityId entity_id);
    [[nodiscard]] bool removeEntity(const GeometryEntityImplPtr& entity);
    [[nodiscard]] bool removeEntity(EntityUID entity_uid, EntityType entity_type);
    void clear();

    [[nodiscard]] GeometryEntityImplPtr findById(EntityId entity_id) const;
    [[nodiscard]] GeometryEntityImplPtr findByUIDAndType(EntityUID entity_uid,
                                                         EntityType entity_type) const;
    [[nodiscard]] GeometryEntityImplPtr findByShape(const TopoDS_Shape& shape) const;

    [[nodiscard]] GeometryEntityImplPtr findByKey(const EntityKey& key) const;
    [[nodiscard]] GeometryEntityImplPtr findByRef(const EntityRef& ref) const;

    /**
     * @brief Fast id -> (uid, type) lookup without returning the full entity.
     * @param entity_id Global entity id to look up.
     * @return EntityRef with uid+type, or invalid EntityRef if not found.
     *
     * @note O(1) lightweight lookup. Use instead of findById() when only the
     *       entity reference is needed, to avoid shared_ptr ref-count overhead.
     */
    [[nodiscard]] EntityRef resolveId(EntityId entity_id) const;

    /**
     * @brief Fast id -> EntityKey lookup without shared_ptr overhead.
     * @param entity_id Global entity id to look up.
     * @return EntityKey with (id, uid, type), or invalid EntityKey if not found.
     *
     * @note O(1) lightweight lookup that avoids atomic ref-count overhead.
     *       Use instead of findById() when only identity information is needed.
     */
    [[nodiscard]] EntityKey resolveIdToKey(EntityId entity_id) const;

    /**
     * @brief Resolve (uid, type) to a full EntityKey including the global id.
     * @param ref Entity reference with uid+type.
     * @return EntityKey with (id, uid, type), or invalid EntityKey if not found.
     *
     * @note O(1) lookup via direct array access. Reads the entity's id without
     *       copying the shared_ptr, avoiding atomic ref-count overhead.
     */
    [[nodiscard]] EntityKey resolveRefToKey(const EntityRef& ref) const;

    [[nodiscard]] size_t entityCount() const;
    [[nodiscard]] size_t entityCountByType(EntityType entity_type) const;

    /// Snapshot of currently alive entities (order unspecified).
    [[nodiscard]] std::vector<GeometryEntityImplPtr> snapshotEntities() const;

    /// Get all entities of a specific type (iterates the type's bucket).
    [[nodiscard]] std::vector<GeometryEntityImplPtr> entitiesByType(EntityType entity_type) const;

private:
    /// Number of distinct EntityType values (None=0 through Part=9).
    static constexpr size_t kBucketCount = 10;

    /// A single storage slot within a per-type bucket.
    struct Slot {
        GeometryEntityImplPtr m_entity;
        uint32_t m_generation{1}; ///< Bumped on each removal to invalidate stale handles.
    };

    /// Handle stored in the shape map for generation-based stale detection.
    struct ShapeHandle {
        EntityType m_type{EntityType::None};
        EntityUID m_uid{INVALID_ENTITY_UID};
        uint32_t m_generation{0};
    };

    /// Per-type slot buckets. Slot at index [uid - 1] holds the entity with that uid.
    std::array<std::vector<Slot>, kBucketCount> m_typeBuckets;

    /// Fast id -> (uid, type) for EntityId-based lookup.
    std::unordered_map<EntityId, EntityRef, std::hash<EntityId>> m_idToRef;

    /// Shape -> handle for shape-based lookup (generation-validated).
    mutable std::unordered_map<TopoDS_Shape, ShapeHandle, TopTools_ShapeMapHasher> m_byShape;

    /// Per-type alive entity counts.
    std::array<size_t, kBucketCount> m_countByType{};
    size_t m_aliveCount{0};

    /// Convert EntityType to bucket index, returns kBucketCount on invalid type.
    [[nodiscard]] static constexpr size_t bucketIndex(EntityType type) noexcept {
        const auto idx = static_cast<size_t>(type);
        return (idx < kBucketCount) ? idx : kBucketCount;
    }
};
} // namespace OpenGeoLab::Geometry
