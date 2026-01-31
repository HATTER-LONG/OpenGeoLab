/**
 * @file geometry_documentImpl.hpp
 * @brief Implementation of GeometryDocument interface
 *
 * GeometryDocumentImpl is the concrete implementation of the GeometryDocument
 * interface, providing entity management and render data generation for
 * OpenGL visualization.
 *
 * Geometry editing operations are handled by Action classes that operate
 * on the document through its public interface.
 */

#pragma once

#include "entity/entity_index.hpp"
#include "entity/geometry_entity.hpp"
#include "geometry/geometry_document.hpp"

#include <memory>
#include <mutex>
#include <vector>

namespace OpenGeoLab::Geometry {
class GeometryDocumentImpl;
class PartEntity;
using GeometryDocumentImplPtr = std::shared_ptr<GeometryDocumentImpl>;
using PartEntityPtr = std::shared_ptr<PartEntity>;

/**
 * @brief Concrete implementation of GeometryDocument
 *
 * The document is the primary owner of EntityIndex and all geometry entities.
 * Entities keep a weak back-reference to the document for relationship resolution.
 *
 * This implementation provides:
 * - Shape loading from OCC TopoDS_Shape
 * - Render data generation for visualization
 * - Entity hierarchy management
 * - Observer notification for geometry changes
 */
class GeometryDocumentImpl : public GeometryDocument,
                             public std::enable_shared_from_this<GeometryDocumentImpl> {
public:
    GeometryDocumentImpl() = default;
    ~GeometryDocumentImpl() override = default;

    // =========================================================================
    // Shape Loading (GeometryDocument interface)
    // =========================================================================

    [[nodiscard]] LoadResult
    loadFromShape(const TopoDS_Shape& shape,
                  const std::string& name = "Part",
                  DocumentProgressCallback progress = noProgressCallback) override;

    [[nodiscard]] bool buildShapes() override;

    // =========================================================================
    // Entity Management (GeometryDocument interface)
    // =========================================================================

    [[nodiscard]] bool deleteEntities(const std::vector<EntityId>& entityIds,
                                      bool deleteChildren = true) override;

    // =========================================================================
    // Observer Pattern (GeometryDocument interface)
    // =========================================================================

    void addChangeObserver(IGeometryChangeObserver* observer) override;
    void removeChangeObserver(IGeometryChangeObserver* observer) override;
    void notifyGeometryChanged(const GeometryChangeEvent& event) override;

    // =========================================================================
    // Render Data Interface (GeometryDocument interface)
    // =========================================================================

    [[nodiscard]] const RenderContext* getRenderContext() const override;

    [[nodiscard]] bool
    rebuildRenderData(DocumentProgressCallback progress = noProgressCallback) override;

    [[nodiscard]] RenderMesh getRenderMeshForFace(EntityId faceEntityId) const override;

    [[nodiscard]] RenderEdge getRenderEdgeForEdge(EntityId edgeEntityId) const override;

    // =========================================================================
    // Entity Queries (GeometryDocument interface)
    // =========================================================================

    [[nodiscard]] size_t entityCount() const override;

    [[nodiscard]] size_t entityCountByType(EntityType type) const override;

    [[nodiscard]] BoundingBox3D boundingBox() const override;

    [[nodiscard]] bool isEmpty() const override;

    void clear() override;

    // =========================================================================
    // Selection Support (GeometryDocument interface)
    // =========================================================================

    [[nodiscard]] EntityId pickEntity(int screenX, int screenY, SelectionMode mode) const override;

    [[nodiscard]] std::vector<EntityId>
    pickEntitiesInRect(int x1, int y1, int x2, int y2, SelectionMode mode) const override;

    // =========================================================================
    // Internal Entity Management (Implementation-specific)
    // =========================================================================

    /**
     * @brief Add an entity to the document index
     * @param entity Entity to add
     * @return true if added; false if null or duplicates exist
     * @note On success, the entity receives a weak back-reference to this document
     */
    [[nodiscard]] bool addEntity(const GeometryEntityPtr& entity);

    /**
     * @brief Remove an entity from the document by id
     * @param entity_id Entity id to remove
     * @return true if removed; false if not found
     * @note Removal eagerly detaches relationship edges
     */
    [[nodiscard]] bool removeEntity(EntityId entity_id);

    /**
     * @brief Remove an entity and all its descendants recursively
     * @param entity_id Entity id to remove
     * @return Number of entities removed (including the root entity)
     */
    [[nodiscard]] size_t removeEntityWithChildren(EntityId entity_id);

    // =========================================================================
    // Entity Lookup (Implementation-specific)
    // =========================================================================

    [[nodiscard]] GeometryEntityPtr findById(EntityId entity_id) const;

    [[nodiscard]] GeometryEntityPtr findByUIDAndType(EntityUID entity_uid,
                                                     EntityType entity_type) const;

    [[nodiscard]] GeometryEntityPtr findByShape(const TopoDS_Shape& shape) const;

    /**
     * @brief Get all entities of a specific type
     * @param entity_type Type to filter by
     * @return Vector of entities matching the type
     */
    [[nodiscard]] std::vector<GeometryEntityPtr> entitiesByType(EntityType entity_type) const;

    /**
     * @brief Get a snapshot of all entities in the document
     * @return Vector of all entities
     */
    [[nodiscard]] std::vector<GeometryEntityPtr> allEntities() const;

    // =========================================================================
    // Entity Relationship Queries (Implementation-specific)
    // =========================================================================

    /**
     * @brief Find ancestor entities of a specific type
     * @param entity_id Source entity id
     * @param ancestor_type Type of ancestors to find
     * @return Vector of ancestor entities of the specified type
     */
    [[nodiscard]] std::vector<GeometryEntityPtr> findAncestors(EntityId entity_id,
                                                               EntityType ancestor_type) const;

    /**
     * @brief Find descendant entities of a specific type
     * @param entity_id Source entity id
     * @param descendant_type Type of descendants to find
     * @return Vector of descendant entities of the specified type
     */
    [[nodiscard]] std::vector<GeometryEntityPtr> findDescendants(EntityId entity_id,
                                                                 EntityType descendant_type) const;

    /**
     * @brief Find the owning Part entity for a given entity
     * @param entity_id Source entity id
     * @return Part entity that owns this entity, or nullptr if not found
     */
    [[nodiscard]] GeometryEntityPtr findOwningPart(EntityId entity_id) const;

    /**
     * @brief Find related entities of a specific type for an edge entity
     * @param edge_entity_id Edge entity id
     * @param related_type Type of related entities (e.g., Face)
     * @return Vector of related entities
     */
    [[nodiscard]] std::vector<GeometryEntityPtr> findRelatedEntities(EntityId edge_entity_id,
                                                                     EntityType related_type) const;

    // =========================================================================
    // Relationship Edge Management (Implementation-specific)
    // =========================================================================

    /**
     * @brief Add a directed parent->child edge
     * @param parent_id Parent entity id
     * @param child_id Child entity id
     * @return true if the edge is added; false on invalid ids or cycle detection
     */
    [[nodiscard]] bool addChildEdge(EntityId parent_id, EntityId child_id);

    /**
     * @brief Remove a directed parent->child edge
     * @param parent_id Parent entity id
     * @param child_id Child entity id
     * @return true if the edge existed and was removed
     */
    [[nodiscard]] bool removeChildEdge(EntityId parent_id, EntityId child_id);

private:
    /**
     * @brief Helper for recursive entity removal
     */
    void removeEntityRecursive(EntityId entity_id, size_t& removed_count);

    /**
     * @brief Build render mesh for a face shape
     */
    RenderMesh buildMeshForFace(const GeometryEntityPtr& faceEntity) const;

    /**
     * @brief Build render edge for an edge shape
     */
    RenderEdge buildEdgeForEdge(const GeometryEntityPtr& edgeEntity) const;

    /**
     * @brief Build render vertex for a vertex shape
     */
    RenderVertex buildVertexForVertex(const GeometryEntityPtr& vertexEntity) const;

    /**
     * @brief Mark render data as needing rebuild
     */
    void invalidateRenderData();

private:
    EntityIndex m_entityIndex;

    /// Registered change observers
    std::vector<IGeometryChangeObserver*> m_changeObservers;

    /// Cached render context
    mutable RenderContext m_renderContext;
    mutable bool m_renderDataValid{false};

    /// Cached bounding box
    mutable BoundingBox3D m_boundingBox;
    mutable bool m_boundingBoxValid{false};
};

} // namespace OpenGeoLab::Geometry