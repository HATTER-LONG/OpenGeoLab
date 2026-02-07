/**
 * @file geometry_documentImpl.hpp
 * @brief Geometry document implementation for entity management
 *
 * GeometryDocumentImpl is the primary container for geometry entities within
 * the application. Each document represents an independent model or assembly
 * with its own entity index and relationship graph.
 */

#pragma once

#include "entity/entity_index.hpp"
#include "entity/geometry_entity.hpp"
#include "entity/relationship_index.hpp"
#include "geometry/geometry_document.hpp"

#include <memory>
#include <mutex>
#include <vector>

namespace OpenGeoLab::Geometry {

class GeometryDocumentImpl;
using GeometryDocumentImplPtr = std::shared_ptr<GeometryDocumentImpl>;
/**
 * @brief Geometry document holding the authoritative entity index.
 *
 * The document is the only owner and user of EntityIndex. Entities keep a
 * weak back-reference to the document for relationship resolution.
 */
class GeometryDocumentImpl : public GeometryDocument,
                             public std::enable_shared_from_this<GeometryDocumentImpl> {
public:
    GeometryDocumentImpl();
    virtual ~GeometryDocumentImpl() = default;

    // =========================================================================
    // Shape Operations (GeometryDocument interface)
    // =========================================================================

    [[nodiscard]] LoadResult loadFromShape(const TopoDS_Shape& shape,
                                           const std::string& name,
                                           Util::ProgressCallback progress) override;

    [[nodiscard]] LoadResult appendShape(const TopoDS_Shape& shape,
                                         const std::string& name,
                                         Util::ProgressCallback progress) override;

    // -------------------------------------------------------------------------
    // Entity Management
    // -------------------------------------------------------------------------

    /**
     * @brief Add an entity to the document index.
     * @param entity Entity to add.
     * @return true if added; false if null or duplicates exist.
     * @note On success, the entity receives a weak back-reference to this document.
     */
    [[nodiscard]] bool addEntity(const GeometryEntityPtr& entity);

    /**
     * @brief Remove an entity from the document by id.
     * @param entity_id Entity id to remove.
     * @return true if removed; false if not found.
     * @note Removal eagerly detaches relationship edges so remaining entities do not
     *       retain stale parent/child ids.
     */
    [[nodiscard]] bool removeEntity(EntityId entity_id);

    /**
     * @brief Remove an entity and all its descendants recursively.
     * @param entity_id Entity id to remove.
     * @return Number of entities removed (including the root entity).
     * @note This performs a depth-first traversal to remove all child entities
     *       before removing the parent, ensuring proper cleanup.
     */
    [[nodiscard]] size_t removeEntityWithChildren(EntityId entity_id);

    /**
     * @brief Clear all entities from this document.
     * @note Fast-path: assumes the document holds the only strong references to entities.
     *       If entities may be owned elsewhere (external shared_ptr), use a safe clear
     *       that detaches document refs and clears local relation sets to avoid stale IDs.
     * @warning This fast clear relies on exclusive ownership. If violated, leftover
     *          entities will keep stale parent/child sets and a dangling document pointer.
     */
    void clear();

    // -------------------------------------------------------------------------
    // Entity Lookup
    // -------------------------------------------------------------------------

    [[nodiscard]] GeometryEntityPtr findById(EntityId entity_id) const;

    [[nodiscard]] GeometryEntityPtr findByUIDAndType(EntityUID entity_uid,
                                                     EntityType entity_type) const;

    [[nodiscard]] GeometryEntityPtr findByShape(const TopoDS_Shape& shape) const;

    [[nodiscard]] size_t entityCount() const;

    [[nodiscard]] size_t entityCountByType(EntityType entity_type) const;

    /**
     * @brief Get all entities of a specific type.
     * @param entity_type Type to filter by.
     * @return Vector of entities matching the type.
     */
    [[nodiscard]] std::vector<GeometryEntityPtr> entitiesByType(EntityType entity_type) const;

    /**
     * @brief Get a snapshot of all entities in the document.
     * @return Vector of all entities.
     */
    [[nodiscard]] std::vector<GeometryEntityPtr> allEntities() const;

    // -------------------------------------------------------------------------
    // Entity Relationship Queries
    // -------------------------------------------------------------------------

    [[nodiscard]] std::vector<EntityId> findRelatedEntities(EntityId edge_entity_id,
                                                            EntityType related_type) const;

    // -------------------------------------------------------------------------
    // Relationship Edge Management
    // -------------------------------------------------------------------------

    /**
     * @brief Add a directed parent->child edge.
     * @param parent Parent entity.
     * @param child Child entity.
     * @return true if the edge is added; false if ids are invalid, entities are missing,
     *         or the edge would create a cycle.
     */
    [[nodiscard]] bool addChildEdge(const GeometryEntity& parent, const GeometryEntity& child);

    // =========================================================================
    // Render Data Access (GeometryDocument interface)
    // =========================================================================

    [[nodiscard]] Render::DocumentRenderData
    getRenderData(const Render::TessellationOptions& options) override;

    void invalidateRenderData() override;

    // =========================================================================
    // Change Notification (GeometryDocument interface)
    // =========================================================================

    [[nodiscard]] Util::ScopedConnection
    subscribeToChanges(std::function<void(const GeometryChangeEvent&)> callback) override;

    // -------------------------------------------------------------------------
    // Relationship Access
    // -------------------------------------------------------------------------

    [[nodiscard]] EntityRelationshipIndex& relationships() { return m_relationshipIndex; }
    [[nodiscard]] const EntityRelationshipIndex& relationships() const {
        return m_relationshipIndex;
    }

private:
    /**
     * @brief Helper for recursive entity removal.
     * @param entity_id Entity to remove.
     * @param removed_count Counter for removed entities.
     */
    void removeEntityRecursive(EntityId entity_id, size_t& removed_count);

    /**
     * @brief Generate render mesh for a face entity
     * @param entity Face entity
     * @param options Tessellation options
     * @return Render mesh for the face
     */
    [[nodiscard]] Render::RenderMesh generateFaceMesh(const GeometryEntityPtr& entity,
                                                      const Render::TessellationOptions& options);

    /**
     * @brief Generate render mesh for an edge entity
     * @param entity Edge entity
     * @param options Tessellation options
     * @return Render mesh for the edge
     */
    [[nodiscard]] Render::RenderMesh generateEdgeMesh(const GeometryEntityPtr& entity,
                                                      const Render::TessellationOptions& options);

    /**
     * @brief Generate render mesh for a vertex entity
     * @param entity Vertex entity
     * @return Render mesh for the vertex
     */
    [[nodiscard]] Render::RenderMesh generateVertexMesh(const GeometryEntityPtr& entity);

    /**
     * @brief Emit a change notification to all subscribers
     * @param event Change event to emit
     */
    void emitChangeEvent(const GeometryChangeEvent& event);

private:
    EntityIndex m_entityIndex;

    EntityRelationshipIndex m_relationshipIndex;

    /// Signal for geometry change notifications
    Util::Signal<const GeometryChangeEvent&> m_changeSignal;

    /// Cached render data
    mutable Render::DocumentRenderData m_cachedRenderData;
    mutable bool m_renderDataValid{false};
    mutable std::mutex m_renderDataMutex;
};

} // namespace OpenGeoLab::Geometry