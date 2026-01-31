/**
 * @file geometry_document.hpp
 * @brief Public interface for geometry document management
 *
 * GeometryDocument is the primary container for geometry entities within
 * the application. This header defines the public interface for interacting
 * with geometry documents, including loading and rendering operations.
 *
 * Geometry editing operations (trim, offset, fillet, chamfer, etc.) are
 * handled by specialized Action classes following the command pattern.
 * @see geometry_action.hpp for the action framework
 */

#pragma once

#include "geometry_types.hpp"
#include "render_types.hpp"

#include <kangaroo/util/noncopyable.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

class TopoDS_Shape;

namespace OpenGeoLab::Geometry {
class GeometryDocument;
using GeometryDocumentPtr = std::shared_ptr<GeometryDocument>;

// =============================================================================
// Geometry Change Observer (Entity rebuild mechanism)
// =============================================================================

/**
 * @brief Type of geometry change that occurred
 */
enum class GeometryChangeType {
    ShapeLoaded,           ///< New shape loaded into document
    ShapeModified,         ///< Existing shape was modified
    ShapeDeleted,          ///< Shape was deleted
    RenderDataInvalidated, ///< Render data needs rebuild
    BoundingBoxChanged     ///< Bounding box changed
};

/**
 * @brief Information about a geometry change event
 */
struct GeometryChangeEvent {
    GeometryChangeType m_type;                ///< Type of change
    std::vector<EntityId> m_affectedEntities; ///< Entities affected by change
    std::string m_message;                    ///< Optional description
};

/**
 * @brief Observer interface for geometry change notifications
 *
 * Classes implementing this interface can register with GeometryDocument
 * to receive notifications when geometry changes. This enables:
 * - Render layer to automatically rebuild meshes
 * - UI layer to update entity displays
 * - Undo/redo system to track changes
 */
class IGeometryChangeObserver {
public:
    virtual ~IGeometryChangeObserver() = default;

    /**
     * @brief Called when geometry changes in the document
     * @param event Information about the change
     */
    virtual void onGeometryChanged(const GeometryChangeEvent& event) = 0;
};

// =============================================================================
// Load Result
// =============================================================================

/**
 * @brief Result of a shape load operation
 */
struct LoadResult {
    bool m_success{false};                      ///< Whether the load succeeded
    std::string m_errorMessage;                 ///< Error message if failed
    EntityId m_rootEntityId{INVALID_ENTITY_ID}; ///< Root entity of loaded geometry
    size_t m_entityCount{0};                    ///< Total number of entities created

    /// Create a success result
    [[nodiscard]] static LoadResult success(EntityId rootId, size_t count) {
        LoadResult result;
        result.m_success = true;
        result.m_rootEntityId = rootId;
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

// =============================================================================
// Progress Callback
// =============================================================================

/**
 * @brief Progress callback for long-running operations
 * @param progress Progress value in [0, 1] range
 * @param message Status message
 * @return false to request cancellation, true to continue
 */
using DocumentProgressCallback = std::function<bool(double progress, const std::string& message)>;

/// No-operation progress callback that never cancels
inline bool noProgressCallback(double /*progress*/, const std::string& /*message*/) { return true; }

// =============================================================================
// GeometryDocument Interface
// =============================================================================

/**
 * @brief Abstract interface for geometry document operations
 *
 * GeometryDocument provides the public interface for:
 * - Loading geometry from OCC shapes (for file readers)
 * - Geometry editing operations (trim, offset, fillet, etc.)
 * - Generating render data for visualization
 * - Entity queries and management
 *
 * Thread-safety: Read operations are thread-safe. Modifications should be
 * synchronized externally or performed on the main thread.
 */
class GeometryDocument : public Kangaroo::Util::NonCopyMoveable {
public:
    GeometryDocument() = default;
    virtual ~GeometryDocument() = default;

    // -------------------------------------------------------------------------
    // Shape Loading (for io/reader)
    // -------------------------------------------------------------------------

    /**
     * @brief Load geometry from an OCC shape
     * @param shape Source OCC shape to load
     * @param name Name for the root part entity
     * @param progress Progress callback for reporting
     * @return LoadResult with success status and entity information
     *
     * @note This is the primary entry point for file readers to add
     *       geometry to the document.
     */
    [[nodiscard]] virtual LoadResult
    loadFromShape(const TopoDS_Shape& shape,
                  const std::string& name = "Part",
                  DocumentProgressCallback progress = noProgressCallback) = 0;

    /**
     * @brief Build/rebuild internal shape representations
     * @return true if build succeeded
     * @note Called after modifications to ensure internal state is consistent
     */
    [[nodiscard]] virtual bool buildShapes() = 0;

    // -------------------------------------------------------------------------
    // Entity Management (for Action classes)
    // -------------------------------------------------------------------------

    /**
     * @brief Delete entities from the document
     * @param entityIds Entity IDs to delete
     * @param deleteChildren Whether to also delete child entities
     * @return true if entities were deleted successfully
     *
     * @note This method is used by Action classes for geometry operations.
     *       It triggers geometry change notifications to registered observers.
     */
    [[nodiscard]] virtual bool deleteEntities(const std::vector<EntityId>& entityIds,
                                              bool deleteChildren = true) = 0;

    // -------------------------------------------------------------------------
    // Observer Pattern (Geometry Change Notifications)
    // -------------------------------------------------------------------------

    /**
     * @brief Register an observer for geometry change events
     * @param observer Observer to register
     *
     * Observers are notified when:
     * - New shapes are loaded (ShapeLoaded)
     * - Shapes are modified by actions (ShapeModified)
     * - Shapes are deleted (ShapeDeleted)
     * - Render data is invalidated (RenderDataInvalidated)
     */
    virtual void addChangeObserver(IGeometryChangeObserver* observer) = 0;

    /**
     * @brief Unregister an observer
     * @param observer Observer to remove
     */
    virtual void removeChangeObserver(IGeometryChangeObserver* observer) = 0;

    /**
     * @brief Notify observers of a geometry change
     * @param event Change event information
     * @note Typically called internally by document operations
     */
    virtual void notifyGeometryChanged(const GeometryChangeEvent& event) = 0;

    // -------------------------------------------------------------------------
    // Render Data Interface
    // -------------------------------------------------------------------------

    /**
     * @brief Get render context for OpenGL visualization
     * @return Pointer to RenderContext (valid while document exists)
     * @note The returned pointer remains valid until document modification
     */
    [[nodiscard]] virtual const RenderContext* getRenderContext() const = 0;

    /**
     * @brief Rebuild render data from current geometry
     * @param progress Progress callback
     * @return true if rebuild succeeded
     * @note Call this after geometry modifications to update render data
     */
    [[nodiscard]] virtual bool
    rebuildRenderData(DocumentProgressCallback progress = noProgressCallback) = 0;

    /**
     * @brief Get render mesh for a specific face entity
     * @param faceEntityId Face entity ID
     * @return Render mesh data, or empty mesh if not found
     */
    [[nodiscard]] virtual RenderMesh getRenderMeshForFace(EntityId faceEntityId) const = 0;

    /**
     * @brief Get render edge for a specific edge entity
     * @param edgeEntityId Edge entity ID
     * @return Render edge data, or empty edge if not found
     */
    [[nodiscard]] virtual RenderEdge getRenderEdgeForEdge(EntityId edgeEntityId) const = 0;

    // -------------------------------------------------------------------------
    // Entity Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get total entity count in document
     * @return Number of entities
     */
    [[nodiscard]] virtual size_t entityCount() const = 0;

    /**
     * @brief Get entity count by type
     * @param type Entity type to count
     * @return Number of entities of specified type
     */
    [[nodiscard]] virtual size_t entityCountByType(EntityType type) const = 0;

    /**
     * @brief Get overall bounding box of all geometry
     * @return Document bounding box
     */
    [[nodiscard]] virtual BoundingBox3D boundingBox() const = 0;

    /**
     * @brief Check if document has any geometry
     * @return true if document contains entities
     */
    [[nodiscard]] virtual bool isEmpty() const = 0;

    /**
     * @brief Clear all geometry from document
     */
    virtual void clear() = 0;

    // -------------------------------------------------------------------------
    // Selection Support
    // -------------------------------------------------------------------------

    /**
     * @brief Find entity at screen coordinates (for picking)
     * @param screenX Screen X coordinate
     * @param screenY Screen Y coordinate
     * @param mode Selection mode filter
     * @return Entity ID at location, or INVALID_ENTITY_ID if none
     * @note Requires render context to be built
     */
    [[nodiscard]] virtual EntityId
    pickEntity(int screenX, int screenY, SelectionMode mode) const = 0;

    /**
     * @brief Find entities within a screen rectangle
     * @param x1 Rectangle left
     * @param y1 Rectangle top
     * @param x2 Rectangle right
     * @param y2 Rectangle bottom
     * @param mode Selection mode filter
     * @return Vector of entity IDs within rectangle
     */
    [[nodiscard]] virtual std::vector<EntityId>
    pickEntitiesInRect(int x1, int y1, int x2, int y2, SelectionMode mode) const = 0;
};

} // namespace OpenGeoLab::Geometry