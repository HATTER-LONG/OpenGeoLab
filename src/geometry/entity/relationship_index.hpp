/**
 * @file relationship_index.hpp
 * @brief Relationship index for fast cross-entity lookups
 */

#pragma once

#include "geometry/geometry_types.hpp"
#include "geometry_entity.hpp"
#include <kangaroo/util/noncopyable.hpp>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace OpenGeoLab::Geometry {

class EntityIndex;

class EntityRelationshipIndex : public Kangaroo::Util::NonCopyMoveable {
public:
    explicit EntityRelationshipIndex(EntityIndex& entity_index);
    ~EntityRelationshipIndex() = default;

    void clear();

    [[nodiscard]] bool addRelationshipInfo(const GeometryEntity& parent,
                                           const GeometryEntity& child);

    /// Remove all edges incident to entity_id (best-effort, safe if missing).
    void detachEntity(const GeometryEntity& entity);

    [[nodiscard]] bool buildRelationships();

    [[nodiscard]] std::vector<EntityKey> findRelatedEntities(EntityId source_id,
                                                             EntityType target_type) const;

    [[nodiscard]] std::vector<EntityKey>
    findRelatedEntities(EntityUID source_uid, EntityType source_type, EntityType target_type) const;

    [[nodiscard]] std::vector<EntityKey> findRelatedEntities(const GeometryEntity& source,
                                                             EntityType target_type) const;

    /// Direct adjacency (order unspecified).
    [[nodiscard]] std::vector<EntityId> directChildren(EntityId parent_id) const;
    [[nodiscard]] std::vector<EntityId> directParents(EntityId child_id) const;
    [[nodiscard]] std::vector<EntityId> directChildren(const GeometryEntity& parent) const;
    [[nodiscard]] std::vector<EntityId> directParents(const GeometryEntity& child) const;

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