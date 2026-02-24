/**
 * @file geometry_document.hpp
 * @brief Geometry document interface for entity management and rendering
 *
 * Defines the public interface for geometry documents that manage
 * geometric entities and provide render data for visualization.
 */

#pragma once
#include "geometry/geometry_entity.hpp"
#include "geometry/geometry_types.hpp"

#include "render/render_data.hpp"
#include "util/progress_callback.hpp"
#include "util/signal.hpp"

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>
#include <memory>
#include <string>

class TopoDS_Shape;

namespace OpenGeoLab::Geometry {

class GeometryDocument;
using GeometryDocumentPtr = std::shared_ptr<GeometryDocument>;

/**
 * @brief Type of geometry change that occurred
 */
enum class GeometryChangeType : uint8_t {
    EntityAdded = 0,          ///< One or more entities were added
    EntityRemoved = 1,        ///< One or more entities were removed
    EntityModified = 2,       ///< Entity properties changed
    DocumentCleared = 3,      ///< All entities removed
    RenderDataInvalidated = 4 ///< Render data needs refresh
};

/**
 * @brief Information about a geometry change event
 */
struct GeometryChangeEvent {
    GeometryChangeType m_type{GeometryChangeType::EntityModified}; ///< Type of change
    std::vector<EntityId> m_affectedEntities; ///< IDs of affected entities (may be empty)

    GeometryChangeEvent() = default;
    explicit GeometryChangeEvent(GeometryChangeType type) : m_type(type) {}
    GeometryChangeEvent(GeometryChangeType type, EntityId entity_id)
        : m_type(type), m_affectedEntities{entity_id} {}
    GeometryChangeEvent(GeometryChangeType type, std::vector<EntityId> entities)
        : m_type(type), m_affectedEntities(std::move(entities)) {}
};

/**
 * @brief Result of a shape load operation
 */
struct LoadResult {
    bool m_success{false};                      ///< Whether the load succeeded
    std::string m_errorMessage;                 ///< Error message if failed
    EntityId m_rootEntityId{INVALID_ENTITY_ID}; ///< Root entity of loaded geometry
    size_t m_entityCount{0};                    ///< Total number of entities created

    /// Create a success result
    [[nodiscard]] static LoadResult success(EntityId root_id, size_t count) {
        LoadResult result;
        result.m_success = true;
        result.m_rootEntityId = root_id;
        result.m_entityCount = count;
        return result;
    }

    /// Create a failure result with error message
    [[nodiscard]] static LoadResult failure(const std::string& message) {
        LoadResult result;
        result.m_success = false;
        result.m_errorMessage = message;
        return result;
    }
};

/**
 * @brief Abstract geometry document interface
 *
 * A geometry document is the authoritative container of geometry entities.
 * Implementations are responsible for:
 * - Loading/appending OCC shapes and creating entity hierarchies
 * - Producing render data snapshots for visualization
 * - Emitting change notifications for UI/render synchronization
 */
class GeometryDocument : public Kangaroo::Util::NonCopyMoveable {
public:
    virtual ~GeometryDocument() = default;

    // -------------------------------------------------------------------------
    // Shape Loading
    // -------------------------------------------------------------------------

    /**
     * @brief Load geometry from an OCC shape (clears existing geometry)
     * @param shape Source OCC shape to load
     * @param name Name for the root part entity
     * @param progress Progress callback for reporting
     * @return LoadResult with success status and entity information
     *
     * @note This clears the document first, then loads the new geometry.
     *       Use this for file import operations.
     */
    [[nodiscard]] virtual LoadResult
    loadFromShape(const TopoDS_Shape& shape, // NOLINT
                  const std::string& name,
                  Util::ProgressCallback progress = Util::NO_PROGRESS_CALLBACK) = 0;
    /**
     * @brief Append geometry from an OCC shape (keeps existing geometry)
     * @param shape Source OCC shape to append
     * @param name Name for the root part entity
     * @param progress Progress callback for reporting
     * @return LoadResult with success status and entity information
     *
     * @note This adds new geometry without clearing existing content.
     *       Use this for creating new primitives in the scene.
     */
    [[nodiscard]] virtual LoadResult
    appendShape(const TopoDS_Shape& shape, // NOLINT
                const std::string& name,
                Util::ProgressCallback progress = Util::NO_PROGRESS_CALLBACK) = 0;
    // -------------------------------------------------------------------------
    // Render Data Access
    // -------------------------------------------------------------------------

    [[nodiscard]] virtual const Render::RenderData&
    getRenderData(const Render::TessellationOptions& options) = 0;

    /**
     * @brief Invalidate cached render data
     *
     * Call this method to force regeneration of render data on the next
     * getRenderData() call. Automatically called when geometry changes.
     */
    virtual void invalidateRenderData() = 0;

    // -------------------------------------------------------------------------
    // Entity Lookup
    // -------------------------------------------------------------------------

    /**
     * @brief Find an entity by global id
     * @param entity_id Global entity id
     * @return Geometry entity pointer, or nullptr if not found
     */
    [[nodiscard]] virtual GeometryEntityPtr findById(EntityId entity_id) const = 0;

    /**
     * @brief Find an entity by (uid,type) handle
     * @param entity_uid Type-scoped uid
     * @param entity_type Entity type
     * @return Geometry entity pointer, or nullptr if not found
     */
    [[nodiscard]] virtual GeometryEntityPtr findByUIDAndType(EntityUID entity_uid,
                                                             EntityType entity_type) const = 0;

    /**
     * @brief Find an entity by its underlying shape
     * @param shape OCC shape to look for
     * @return Geometry entity pointer, or nullptr if not found
     */
    [[nodiscard]] virtual GeometryEntityPtr findByShape(const TopoDS_Shape& shape) const = 0;

    // -------------------------------------------------------------------------
    // Relationship Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Find related entities by type.
     *
     * This is a high-level query backed by the document's relationship index.
     * Typical use cases include mapping sub-entities (Face/Edge/Vertex) to their
     * owning Part/Solid/Wire, or enumerating descendants of a Part.
     *
     * @param entity_id Source entity id
     * @param related_type Related entity type to return
     * @return Vector of related entity ids (order unspecified)
     */
    [[nodiscard]] virtual std::vector<EntityKey>
    findRelatedEntities(EntityId entity_id, EntityType related_type) const = 0;

    /**
     * @brief Find related entities by type, starting from a (uid,type) handle.
     *
     * This is a convenience wrapper around the relationship index for external
     * callers that only have the pick handle (uid+type).
     *
     * @param entity_uid Source entity uid
     * @param entity_type Source entity type
     * @param related_type Related entity type to return
     * @return Vector of related entity keys (order unspecified)
     */
    [[nodiscard]] virtual std::vector<EntityKey> findRelatedEntities(
        EntityUID entity_uid, EntityType entity_type, EntityType related_type) const = 0;

    // -------------------------------------------------------------------------
    // Change Notification
    // -------------------------------------------------------------------------

    /**
     * @brief Subscribe to geometry change notifications
     * @param callback Function to call when geometry changes
     * @return ScopedConnection for automatic unsubscription
     *
     * @note The callback may be invoked from any thread. Subscribers should
     *       handle thread safety appropriately.
     */
    [[nodiscard]] virtual Util::ScopedConnection
    subscribeToChanges(std::function<void(const GeometryChangeEvent&)> callback) = 0;

protected:
    GeometryDocument() = default;
};

/**
 * @brief Singleton factory interface for GeometryDocument
 */
class GeometryDocumentSingletonFactory
    : public Kangaroo::Util::FactoryTraits<GeometryDocumentSingletonFactory, GeometryDocument> {
public:
    GeometryDocumentSingletonFactory() = default;
    virtual ~GeometryDocumentSingletonFactory() = default;

    /**
     * @brief Get the singleton instance of the document
     * @return Shared pointer to the document instance
     */
    virtual tObjectSharedPtr instance() const = 0;
};
} // namespace OpenGeoLab::Geometry
#define GeoDocumentInstance                                                                        \
    g_ComponentFactory.getInstanceObject<Geometry::GeometryDocumentSingletonFactory>()
