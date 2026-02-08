/**
 * @file relationship_index.hpp
 * @brief Relationship index for fast cross-entity lookups
 */

#pragma once

#include "geometry/geometry_types.hpp"
#include "geometry_entityImpl.hpp"
#include <kangaroo/util/noncopyable.hpp>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace OpenGeoLab::Geometry {

class EntityIndex;

/**
 * @brief DAG-based relationship index for fast cross-entity topology queries
 *
 * Maintains a directed graph of parent→child edges between geometry entities.
 * Supports both direct adjacency lookups and transitive ancestor/descendant
 * queries with a lazily-built cache. Thread-safe via shared_mutex.
 */
class EntityRelationshipIndex : public Kangaroo::Util::NonCopyMoveable {
public:
    explicit EntityRelationshipIndex(EntityIndex& entity_index);
    ~EntityRelationshipIndex() = default;

    /**
     * @brief Remove all edges and invalidate caches
     */
    void clear();

    /**
     * @brief Add a parent→child relationship edge
     * @param parent Parent entity
     * @param child Child entity
     * @return true if edge was added (not a duplicate)
     */
    [[nodiscard]] bool addRelationshipInfo(const GeometryEntityImpl& parent,
                                           const GeometryEntityImpl& child);

    /// Remove all edges incident to entity_id (best-effort, safe if missing).
    void detachEntity(const GeometryEntityImpl& entity);

    /**
     * @brief Rebuild the transitive closure caches
     * @return true if rebuild succeeded
     */
    [[nodiscard]] bool buildRelationships();

    /**
     * @brief Find entities related to source_id of the given target_type
     * @param source_id Global entity ID of source
     * @param target_type Desired related entity type
     * @return Vector of matching entity keys
     */
    [[nodiscard]] std::vector<EntityKey> findRelatedEntities(EntityId source_id,
                                                             EntityType target_type) const;

    /**
     * @brief Find entities related to (uid, type) handle of the given target_type
     * @param source_uid Source entity UID
     * @param source_type Source entity type
     * @param target_type Desired related entity type
     * @return Vector of matching entity keys
     */
    [[nodiscard]] std::vector<EntityKey>
    findRelatedEntities(EntityUID source_uid, EntityType source_type, EntityType target_type) const;

    /**
     * @brief Find entities related to a given entity of the given target_type
     * @param source Source entity
     * @param target_type Desired related entity type
     * @return Vector of matching entity keys
     */
    [[nodiscard]] std::vector<EntityKey> findRelatedEntities(const GeometryEntityImpl& source,
                                                             EntityType target_type) const;

    /**
     * @brief Get direct child entity IDs
     * @param parent_id Parent entity ID
     * @return Vector of child entity IDs
     */
    [[nodiscard]] std::vector<EntityId> directChildren(EntityId parent_id) const;

    /**
     * @brief Get direct parent entity IDs
     * @param child_id Child entity ID
     * @return Vector of parent entity IDs
     */
    [[nodiscard]] std::vector<EntityId> directParents(EntityId child_id) const;

    /**
     * @brief Get direct child entity IDs from entity reference
     * @param parent Parent entity
     * @return Vector of child entity IDs
     */
    [[nodiscard]] std::vector<EntityId> directChildren(const GeometryEntityImpl& parent) const;

    /**
     * @brief Get direct parent entity IDs from entity reference
     * @param child Child entity
     * @return Vector of parent entity IDs
     */
    [[nodiscard]] std::vector<EntityId> directParents(const GeometryEntityImpl& child) const;

private:
    struct RelatedTargets {
        std::unordered_set<EntityKey, EntityKeyHash> m_nodes;
        std::unordered_set<EntityKey, EntityKeyHash> m_edges;
        std::unordered_set<EntityKey, EntityKeyHash> m_wires;
        std::unordered_set<EntityKey, EntityKeyHash> m_faces;
        std::unordered_set<EntityKey, EntityKeyHash> m_shells;
        std::unordered_set<EntityKey, EntityKeyHash> m_solids;
        std::unordered_set<EntityKey, EntityKeyHash> m_compSolids;
        std::unordered_set<EntityKey, EntityKeyHash> m_compounds;
        std::unordered_set<EntityKey, EntityKeyHash> m_parts;
    };

    void invalidateCache();

    [[nodiscard]] bool addChildNoCache(const EntityKey& parent, const EntityKey& child);

    void buildReverseRecursive(const EntityKey& entity_key,
                               RelatedTargets& targets,
                               std::unordered_set<EntityKey, EntityKeyHash>& visited);
    void buildDescendantsRecursive(const EntityKey& entity_key,
                                   RelatedTargets& targets,
                                   std::unordered_set<EntityKey, EntityKeyHash>& visited);

    [[nodiscard]] std::vector<EntityKey> findDescendantsNoCache(const EntityKey& source,
                                                                EntityType target_type) const;
    [[nodiscard]] std::vector<EntityKey> findAncestorsNoCache(const EntityKey& source,
                                                              EntityType target_type) const;

    [[nodiscard]] static std::vector<EntityKey> selectByType(const RelatedTargets& targets,
                                                             EntityType target_type);
    mutable std::shared_mutex m_indexMutex;

    EntityIndex& m_entityIndex;

    // Base graph adjacency (direct edges only).
    std::unordered_map<EntityKey, std::unordered_set<EntityKey, EntityKeyHash>, EntityKeyHash>
        m_directChildren;
    std::unordered_map<EntityKey, std::unordered_set<EntityKey, EntityKeyHash>, EntityKeyHash>
        m_directParents;

    // Derived caches for fast queries.
    std::unordered_map<EntityKey, RelatedTargets, EntityKeyHash> m_reverseEntityToRelatedTargets;
    std::unordered_map<EntityKey, RelatedTargets, EntityKeyHash> m_fullDescendants;
    bool m_cacheValid{false};
};
} // namespace OpenGeoLab::Geometry