/**
 * @file geometry_document.hpp
 * @brief Geometry document container for entity management
 *
 * GeometryDocument is the primary container for geometry entities within
 * the application. Each document represents an independent model or assembly
 * with its own entity index and relationship graph.
 */

#pragma once

#include <kangaroo/util/noncopyable.hpp>

#include "geometry/entity_index.hpp"

#include <memory>

namespace OpenGeoLab::Geometry {

using GeometryDocumentPtr = std::shared_ptr<GeometryDocument>;
/**
 * @brief Geometry document holding the authoritative entity index.
 *
 * The document is the only owner and user of EntityIndex. Entities keep a
 * weak back-reference to the document for relationship resolution.
 */
class GeometryDocument : public Kangaroo::Util::NonCopyMoveable,
                         public std::enable_shared_from_this<GeometryDocument> {
public:
    [[nodiscard]] static GeometryDocumentPtr create() {
        struct MakeSharedEnabler final : public GeometryDocument {
            MakeSharedEnabler() = default;
        };
        return std::make_shared<MakeSharedEnabler>();
    }

    ~GeometryDocument() = default;

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
     * @brief Clear all entities from this document.
     * @note Fast-path: assumes the document holds the only strong references to entities.
     *       If entities may be owned elsewhere (external shared_ptr), use a safe clear
     *       that detaches document refs and clears local relation sets to avoid stale IDs.
     * @warning This fast clear relies on exclusive ownership. If violated, leftover
     *          entities will keep stale parent/child sets and a dangling document pointer.
     */
    void clear();

    [[nodiscard]] GeometryEntityPtr findById(EntityId entity_id) const;

    [[nodiscard]] GeometryEntityPtr findByUIDAndType(EntityUID entity_uid,
                                                     EntityType entity_type) const;

    [[nodiscard]] GeometryEntityPtr findByShape(const TopoDS_Shape& shape) const;

    [[nodiscard]] size_t entityCount() const;

    [[nodiscard]] size_t entityCountByType(EntityType entity_type) const;

    /**
     * @brief Add a directed parent->child edge.
     * @param parent_id Parent entity id.
     * @param child_id Child entity id.
     * @return true if the edge is added; false if ids are invalid, entities are missing,
     *         or the edge would create a cycle.
     */
    [[nodiscard]] bool addChildEdge(EntityId parent_id, EntityId child_id);

    /**
     * @brief Remove a directed parent->child edge.
     * @param parent_id Parent entity id.
     * @param child_id Child entity id.
     * @return true if the edge existed and was removed; false otherwise.
     */
    [[nodiscard]] bool removeChildEdge(EntityId parent_id, EntityId child_id);

private:
    GeometryDocument() = default;

private:
    EntityIndex m_entityIndex;
};

class GeometryDocumentManager : public Kangaroo::Util::NonCopyMoveable {
public:
    ~GeometryDocumentManager() = default;
    static GeometryDocumentManager& instance();

    [[nodiscard]] GeometryDocumentPtr currentDocument();

    [[nodiscard]] GeometryDocumentPtr newDocument();

protected:
    GeometryDocumentManager() = default;

private:
    GeometryDocumentPtr m_currentDocument;
};

} // namespace OpenGeoLab::Geometry