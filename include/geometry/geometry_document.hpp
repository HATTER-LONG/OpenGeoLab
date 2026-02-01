/**
 * @file geometry_document.hpp
 * @brief Geometry document interface for entity management and rendering
 *
 * Defines the public interface for geometry documents that manage
 * geometric entities and provide render data for visualization.
 */

#pragma once
#include "geometry/geometry_types.hpp"
#include "render/render_data.hpp"
#include "util/progress_callback.hpp"
#include "util/signal.hpp"

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

class GeometryDocument : public Kangaroo::Util::NonCopyMoveable {
public:
    GeometryDocument() = default;
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

    /**
     * @brief Get render data for OpenGL visualization
     * @param options Tessellation options for mesh generation
     * @return Complete render data including faces, edges, and vertices
     *
     * @note This method may cache tessellated data. Call invalidateRenderData()
     *       to force regeneration after geometry changes.
     */
    [[nodiscard]] virtual Render::DocumentRenderData
    getRenderData(const Render::TessellationOptions& options) = 0;

    /**
     * @brief Invalidate cached render data
     *
     * Call this method to force regeneration of render data on the next
     * getRenderData() call. Automatically called when geometry changes.
     */
    virtual void invalidateRenderData() = 0;

    // -------------------------------------------------------------------------
    // Entity Query
    // -------------------------------------------------------------------------

    /**
     * @brief Get total entity count in the document
     * @return Number of entities
     */
    [[nodiscard]] virtual size_t entityCount() const = 0;

    /**
     * @brief Get entity count by type
     * @param type Entity type to count
     * @return Number of entities of the specified type
     */
    [[nodiscard]] virtual size_t entityCountByType(EntityType type) const = 0;

    /**
     * @brief Information about a Part entity for UI display
     */
    struct PartSummary {
        EntityId m_entityId{INVALID_ENTITY_ID}; ///< Part entity ID
        std::string m_name;                     ///< Part display name
        size_t m_vertexCount{0};                ///< Number of vertex descendants
        size_t m_edgeCount{0};                  ///< Number of edge descendants
        size_t m_wireCount{0};                  ///< Number of wire descendants
        size_t m_faceCount{0};                  ///< Number of face descendants
        size_t m_shellCount{0};                 ///< Number of shell descendants
        size_t m_solidCount{0};                 ///< Number of solid descendants
    };

    /**
     * @brief Get summary information for all Part entities
     * @return Vector of PartSummary for each Part in the document
     * @note This method caches results internally for efficiency
     */
    [[nodiscard]] virtual std::vector<PartSummary> getPartSummaries() = 0;

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
};

} // namespace OpenGeoLab::Geometry