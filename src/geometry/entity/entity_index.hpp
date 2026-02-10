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
    /**
     * @brief Add a new entity to the index.
     * @param entity Shared pointer to the entity to add.
     * @return True if the entity was added successfully, false if an entity
     *         with the same (uid, type) already exists.
     */
    [[nodiscard]] bool addEntity(const GeometryEntityImplPtr& entity);

    /**

     * @brief Remove an entity from the index by its global EntityId.
     * @param entity_id Global EntityId of the entity to remove.
     * @return True if the entity was found and removed, false otherwise.
     */
    [[nodiscard]] bool removeEntity(EntityId entity_id);

    /**
     * @brief Remove an entity from the index.
     * @param entity Shared pointer to the entity to remove.
     * @return True if the entity was found and removed, false otherwise.
     */
    [[nodiscard]] bool removeEntity(const GeometryEntityImplPtr& entity);

    /**
     * @brief Remove an entity from the index by its (uid, type).
     * @param entity_uid UID of the entity to remove.
     * @param entity_type Type of the entity to remove.
     * @return True if the entity was found and removed, false otherwise.
     */
    [[nodiscard]] bool removeEntity(EntityUID entity_uid, EntityType entity_type);

    /**
     * @brief Clear all entities from the index.
     */
    void clear();

    /**
     * @brief Find an entity by its global EntityId.
     * @param entity_id Global EntityId of the entity to find.
     * @return Shared pointer to the found entity, or nullptr if not found.
     */
    [[nodiscard]] GeometryEntityImplPtr findById(EntityId entity_id) const;

    /**
     * @brief Find an entity by its (uid, type).
     * @param entity_uid UID of the entity to find.
     * @param entity_type Type of the entity to find.
     * @return Shared pointer to the found entity, or nullptr if not found.
     */
    [[nodiscard]] GeometryEntityImplPtr findByUIDAndType(EntityUID entity_uid,
                                                         EntityType entity_type) const;
    /**
     * @brief Find an entity by its TopoDS_Shape.
     * @param shape TopoDS_Shape of the entity to find.
     * @return Shared pointer to the found entity, or nullptr if not found.
     */
    [[nodiscard]] GeometryEntityImplPtr findByShape(const TopoDS_Shape& shape) const;

    /**
     * @brief Find an entity by its EntityKey.
     * @param key EntityKey of the entity to find.
     * @return Shared pointer to the found entity, or nullptr if not found.
     */
    [[nodiscard]] GeometryEntityImplPtr findByKey(const EntityKey& key) const;

    /**
     * @brief Find an entity by its EntityRef.
     * @param ref EntityRef of the entity to find.
     * @return Shared pointer to the found entity, or nullptr if not found.
     */
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
    /**
     * @brief Get the total number of alive entities in the index.
     * @return Total count of alive entities.
     */
    [[nodiscard]] size_t entityCount() const;

    /**
     * @brief Get the number of alive entities of a specific type.
     * @param entity_type Type of entities to count.
     * @return Count of alive entities of the specified type.
     */
    [[nodiscard]] size_t entityCountByType(EntityType entity_type) const;

    /**
     * @brief Get a snapshot of all alive entities in the index.
     * @return Vector of shared pointers to all alive entities.
     */
    [[nodiscard]] std::vector<GeometryEntityImplPtr> snapshotEntities() const;

    /**
     * @brief Get all alive entities of a specific type.
     * @param entity_type Type of entities to retrieve.
     * @return Vector of shared pointers to alive entities of the specified type.
     */
    [[nodiscard]] std::vector<GeometryEntityImplPtr> entitiesByType(EntityType entity_type) const;

private:
    /// Number of distinct EntityType values (None=0 through Part=9).
    static constexpr size_t K_BUCKET_COUNT = 10;

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
    std::array<std::vector<Slot>, K_BUCKET_COUNT> m_typeBuckets;

    /// Fast id -> (uid, type) for EntityId-based lookup.
    std::unordered_map<EntityId, EntityRef, std::hash<EntityId>> m_idToRef;

    /// Shape -> handle for shape-based lookup (generation-validated).
    mutable std::unordered_map<TopoDS_Shape, ShapeHandle, TopTools_ShapeMapHasher> m_byShape;

    /// Per-type alive entity counts.
    std::array<size_t, K_BUCKET_COUNT> m_countByType{};
    size_t m_aliveCount{0};

    /// Convert EntityType to bucket index, returns K_BUCKET_COUNT on invalid type.
    [[nodiscard]] static constexpr size_t bucketIndex(EntityType type) noexcept {
        const auto idx = static_cast<size_t>(type);
        return (idx < K_BUCKET_COUNT) ? idx : K_BUCKET_COUNT;
    }
};
} // namespace OpenGeoLab::Geometry
