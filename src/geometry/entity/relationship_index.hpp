/**
 * @file relationship_index.hpp
 * @brief Relationship index for fast cross-entity lookups
 */

#pragma once

#include "geometry/geometry_types.hpp"

#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace OpenGeoLab::Geometry {

/**
 * @brief Thread-safe index for cross-entity relationships
 *
 * Stores parent/child edges and precomputed related-entity mappings to support
 * O(1) queries across topology levels (node/edge/wire/face/solid/part).
 */
class RelationshipIndex {
public:
    RelationshipIndex() = default;
    ~RelationshipIndex() = default;

    /**
     * @brief Clear all indexed relationships and entity metadata.
     */
    void clear();

    /**
     * @brief Register a new entity in the index.
     * @param entity_id Entity id to register.
     * @param entity_type Entity type for the id.
     */
    void addEntity(EntityId entity_id, EntityType entity_type);

    /**
     * @brief Remove an entity from the index and rebuild related mappings.
     * @param entity_id Entity id to remove.
     */
    void removeEntity(EntityId entity_id);

    /**
     * @brief Add a directed parent->child edge.
     * @param parent_id Parent entity id.
     * @param child_id Child entity id.
     * @return true if the edge is added; false if already exists or ids missing.
     */
    [[nodiscard]] bool addEdge(EntityId parent_id, EntityId child_id);

    /**
     * @brief Remove a directed parent->child edge and rebuild related mappings.
     * @param parent_id Parent entity id.
     * @param child_id Child entity id.
     * @return true if removed; false if not found.
     */
    [[nodiscard]] bool removeEdge(EntityId parent_id, EntityId child_id);

    /**
     * @brief Detach all relations for an entity without removing it.
     * @param entity_id Entity id whose edges should be removed.
     */
    void detachAllRelations(EntityId entity_id);

    /**
     * @brief Get parent ids for an entity.
     * @param entity_id Target entity id.
     * @return Snapshot of parent ids.
     */
    [[nodiscard]] std::vector<EntityId> parentIds(EntityId entity_id) const;

    /**
     * @brief Get child ids for an entity.
     * @param entity_id Target entity id.
     * @return Snapshot of child ids.
     */
    [[nodiscard]] std::vector<EntityId> childIds(EntityId entity_id) const;

    /**
     * @brief Check if a parent edge exists.
     * @param child_id Child entity id.
     * @param parent_id Parent entity id.
     * @return true if the parent edge exists.
     */
    [[nodiscard]] bool hasParent(EntityId child_id, EntityId parent_id) const;

    /**
     * @brief Check if a child edge exists.
     * @param parent_id Parent entity id.
     * @param child_id Child entity id.
     * @return true if the child edge exists.
     */
    [[nodiscard]] bool hasChild(EntityId parent_id, EntityId child_id) const;

    /**
     * @brief Get parent count for an entity.
     * @param entity_id Target entity id.
     * @return Number of parents.
     */
    [[nodiscard]] size_t parentCount(EntityId entity_id) const;

    /**
     * @brief Get child count for an entity.
     * @param entity_id Target entity id.
     * @return Number of children.
     */
    [[nodiscard]] size_t childCount(EntityId entity_id) const;

    /**
     * @brief Get related entity ids by target type.
     * @param source_id Source entity id.
     * @param target_type Target entity type.
     * @return Snapshot of related entity ids.
     */
    [[nodiscard]] std::vector<EntityId> findRelated(EntityId source_id,
                                                    EntityType target_type) const;

    /**
     * @brief Get grouped members belonging to a part.
     * @param part_id Part entity id.
     * @return Part member groups.
     */
    [[nodiscard]] PartMembers getPartMembers(EntityId part_id) const;

private:
    struct RelatedTargets {
        std::unordered_set<EntityId> m_nodes;
        std::unordered_set<EntityId> m_edges;
        std::unordered_set<EntityId> m_wires;
        std::unordered_set<EntityId> m_faces;
        std::unordered_set<EntityId> m_solids;
        std::unordered_set<EntityId> m_parts;
    };

    static bool isIndexedType(EntityType type);

    void rebuildRelatedMappingsLocked();

    std::unordered_set<EntityId> collectAncestorsLocked(EntityId entity_id) const;
    std::unordered_set<EntityId> collectDescendantsLocked(EntityId entity_id) const;

    void addRelationLocked(EntityId source_id, EntityId target_id);

    static void appendSnapshot(const std::unordered_set<EntityId>& source,
                               std::vector<EntityId>& output);

    mutable std::shared_mutex m_indexMutex;

    std::unordered_map<EntityId, EntityType> m_typeById;
    std::unordered_map<EntityId, std::unordered_set<EntityId>> m_childrenByParent;
    std::unordered_map<EntityId, std::unordered_set<EntityId>> m_parentsByChild;
    std::unordered_map<EntityId, RelatedTargets> m_relatedById;
};

} // namespace OpenGeoLab::Geometry
