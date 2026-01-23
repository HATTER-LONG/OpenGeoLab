/**
 * @file geometry_document.hpp
 * @brief Geometry document and manager with fast entity indexing
 *
 * Provides:
 * - EntityIndex: High-performance O(1) lookup by ID, UID+Type, or shape
 * - GeometryDocument: Document for managing entity hierarchies
 * - GeometryManager: Singleton for global geometry data storage and lookup
 */

#pragma once

#include "geometry/geometry_entity.hpp"
#include "geometry/geometry_types.hpp"

#include <TopoDS_Shape.hxx>

#include <kangaroo/util/noncopyable.hpp>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace OpenGeoLab::Geometry {

class GeometryDocument;
class GeometryManager;

using GeometryDocumentPtr = std::shared_ptr<GeometryDocument>;
using GeometryDocumentWeakPtr = std::weak_ptr<GeometryDocument>;

// =============================================================================
// EntityIndex
// =============================================================================

/**
 * @brief High-performance entity indexing system
 *
 * Maintains multiple hash-based indices for O(1) lookup:
 * - By global EntityId
 * - By (EntityType, EntityUID) pair
 * - By OCC shape (for selection/picking)
 *
 * Automatically updated when entities are added/removed from the document.
 */
class EntityIndex : public Kangaroo::Util::NonCopyMoveable {
public:
    EntityIndex() = default;
    ~EntityIndex() = default;

    // -------------------------------------------------------------------------
    // Index Management
    // -------------------------------------------------------------------------

    /**
     * @brief Add an entity to all indices
     * @param entity Entity to index
     * @note Automatically called by GeometryDocument when entities are registered
     */
    void addEntity(const GeometryEntityPtr& entity);

    /**
     * @brief Remove an entity from all indices
     * @param entity Entity to remove
     * @return true if entity was found and removed
     */
    bool removeEntity(const GeometryEntityPtr& entity);

    /**
     * @brief Clear all indices
     */
    void clear();

    // -------------------------------------------------------------------------
    // Lookup by ID
    // -------------------------------------------------------------------------

    /**
     * @brief Find entity by global ID
     * @param id Global EntityId
     * @return Entity pointer, or nullptr if not found
     */
    [[nodiscard]] GeometryEntityPtr findById(EntityId id) const;

    /**
     * @brief Find entity by type and UID
     * @param type Entity type
     * @param uid Type-scoped UID
     * @return Entity pointer, or nullptr if not found
     */
    [[nodiscard]] GeometryEntityPtr findByTypeAndUID(EntityType type, EntityUID uid) const;

    /**
     * @brief Find entity by OCC shape
     * @param shape TopoDS_Shape to look up
     * @return Entity pointer, or nullptr if not found
     * @note Uses shape's TShape hash for lookup
     */
    [[nodiscard]] GeometryEntityPtr findByShape(const TopoDS_Shape& shape) const;

    // -------------------------------------------------------------------------
    // Bulk Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get all entities of a specific type
     * @param type Entity type to filter by
     * @return Vector of matching entities
     */
    [[nodiscard]] std::vector<GeometryEntityPtr> getEntitiesByType(EntityType type) const;

    /**
     * @brief Get total number of indexed entities
     * @return Entity count
     */
    [[nodiscard]] size_t entityCount() const { return m_byId.size(); }

    /**
     * @brief Get count of entities of a specific type
     * @param type Entity type
     * @return Count of entities with that type
     */
    [[nodiscard]] size_t entityCountByType(EntityType type) const;

private:
    /// Hash functor for (EntityType, EntityUID) pairs
    struct TypeUIDHash {
        size_t operator()(const std::pair<EntityType, EntityUID>& key) const {
            return std::hash<uint64_t>()(static_cast<uint64_t>(key.first) << 56 | key.second);
        }
    };

    /// Index by global EntityId
    std::unordered_map<EntityId, GeometryEntityPtr> m_byId;

    /// Index by (EntityType, EntityUID) pair
    std::unordered_map<std::pair<EntityType, EntityUID>, GeometryEntityPtr, TypeUIDHash>
        m_byTypeAndUID;

    /// Index by OCC shape TShape pointer (for selection)
    std::unordered_map<size_t, GeometryEntityPtr> m_byShapeHash;

    /// Per-type entity lists for bulk queries
    std::unordered_map<EntityType, std::vector<GeometryEntityPtr>> m_byType;
};

// =============================================================================
// GeometryDocument
// =============================================================================

/**
 * @brief Document managing geometry entities with topology hierarchy
 *
 * GeometryDocument provides:
 * - Entity registration and lifecycle management
 * - Fast entity lookup via EntityIndex
 * - Topology hierarchy building from OCC shapes
 *
 * Thread-safety: Not thread-safe. External synchronization required.
 */
class GeometryDocument : public std::enable_shared_from_this<GeometryDocument>,
                         public Kangaroo::Util::NonCopyMoveable {
public:
    GeometryDocument();
    ~GeometryDocument();

    // -------------------------------------------------------------------------
    // Entity Registration
    // -------------------------------------------------------------------------

    /**
     * @brief Register an entity with the document
     * @param entity Entity to register
     * @note Entity will be added to all indices
     */
    void registerEntity(const GeometryEntityPtr& entity);

    /**
     * @brief Unregister an entity from the document
     * @param entity Entity to unregister
     * @return true if entity was found and removed
     * @note Also removes all child entities recursively
     */
    bool unregisterEntity(const GeometryEntityPtr& entity);

    /**
     * @brief Clear all entities from the document
     */
    void clear();

    // -------------------------------------------------------------------------
    // Entity Access
    // -------------------------------------------------------------------------

    /**
     * @brief Get the entity index for queries
     * @return Const reference to EntityIndex
     */
    [[nodiscard]] const EntityIndex& index() const { return m_index; }

    /**
     * @brief Find entity by global ID
     * @param id Global EntityId
     * @return Entity pointer, or nullptr if not found
     */
    [[nodiscard]] GeometryEntityPtr findEntityById(EntityId id) const {
        return m_index.findById(id);
    }

    /**
     * @brief Find entity by type and UID
     * @param type Entity type
     * @param uid Type-scoped UID
     * @return Entity pointer, or nullptr if not found
     */
    [[nodiscard]] GeometryEntityPtr findEntityByTypeAndUID(EntityType type, EntityUID uid) const {
        return m_index.findByTypeAndUID(type, uid);
    }

    /**
     * @brief Find entity by OCC shape
     * @param shape TopoDS_Shape to look up
     * @return Entity pointer, or nullptr if not found
     */
    [[nodiscard]] GeometryEntityPtr findEntityByShape(const TopoDS_Shape& shape) const {
        return m_index.findByShape(shape);
    }

    /**
     * @brief Get all root entities (entities without parents)
     * @return Vector of root entities
     */
    [[nodiscard]] const std::vector<GeometryEntityPtr>& rootEntities() const {
        return m_rootEntities;
    }

    // -------------------------------------------------------------------------
    // Topology Building
    // -------------------------------------------------------------------------

    /**
     * @brief Build complete topology hierarchy from an OCC shape
     * @param shape Root OCC shape
     * @param name Display name for the root entity
     * @return Root entity with complete sub-entity hierarchy
     *
     * Creates a complete entity tree by exploring the OCC topology:
     * Compound -> Solid -> Shell -> Face -> Wire -> Edge -> Vertex
     *
     * All created entities are automatically registered with the document.
     */
    [[nodiscard]] GeometryEntityPtr buildTopologyHierarchy(const TopoDS_Shape& shape,
                                                           const std::string& name = "");

    /**
     * @brief Build sub-entities for an existing entity
     * @param entity Parent entity to build children for
     *
     * Explores the OCC shape and creates child entities for all
     * sub-shapes (faces, edges, vertices, etc.).
     */
    void buildSubEntities(const GeometryEntityPtr& entity);

    // -------------------------------------------------------------------------
    // Document Properties
    // -------------------------------------------------------------------------

    /**
     * @brief Get document name
     * @return Document name string
     */
    [[nodiscard]] const std::string& name() const { return m_name; }

    /**
     * @brief Set document name
     * @param name New document name
     */
    void setName(const std::string& name) { m_name = name; }

    /**
     * @brief Check if document has been modified
     * @return true if document has unsaved changes
     */
    [[nodiscard]] bool isModified() const { return m_modified; }

    /**
     * @brief Mark document as modified or saved
     * @param modified New modified state
     */
    void setModified(bool modified) { m_modified = modified; }

private:
    /**
     * @brief Recursively build topology from a shape
     * @param shape OCC shape to process
     * @param parent Parent entity (may be null for roots)
     * @return Created entity
     */
    GeometryEntityPtr buildEntityFromShape(const TopoDS_Shape& shape,
                                           const GeometryEntityPtr& parent);

    /**
     * @brief Get sub-shapes of a specific type from a shape
     * @param shape Parent shape
     * @param subType Type of sub-shapes to extract
     * @return Vector of sub-shapes
     */
    [[nodiscard]] std::vector<TopoDS_Shape> getSubShapes(const TopoDS_Shape& shape,
                                                         TopAbs_ShapeEnum sub_type) const;

    /**
     * @brief Unregister entity and all children recursively
     * @param entity Entity to unregister
     */
    void unregisterEntityRecursive(const GeometryEntityPtr& entity);

private:
    std::string m_name{"Untitled"};                ///< Document name
    bool m_modified{false};                        ///< Modified flag
    EntityIndex m_index;                           ///< Entity index for fast lookups
    std::vector<GeometryEntityPtr> m_rootEntities; ///< Root-level entities
};

// =============================================================================
// GeometryManager - Singleton for Global Geometry Storage
// =============================================================================

/**
 * @brief Singleton manager for global geometry data storage and lookup
 *
 * GeometryManager serves as the central repository for all geometry entities
 * in the application. It provides:
 * - Global entity index for fast lookups by ID, Type+UID, or shape
 * - Document management (active document concept)
 * - Thread-safe entity registration
 *
 * Usage patterns:
 * @code
 * // Get the singleton instance
 * auto& mgr = GeometryManager::instance();
 *
 * // Find entity by ID
 * auto entity = mgr.findById(entityId);
 *
 * // Find entity by type and UID
 * auto face = mgr.findByTypeAndUID(EntityType::Face, faceUid);
 *
 * // Find entity from OCC shape (for picking/selection)
 * auto picked = mgr.findByShape(pickedShape);
 *
 * // Get all faces
 * auto allFaces = mgr.getEntitiesByType(EntityType::Face);
 * @endcode
 *
 * Thread-safety: findById, findByTypeAndUID, findByShape are thread-safe.
 * Modifications require external synchronization.
 */
class GeometryManager : public Kangaroo::Util::NonCopyMoveable {
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the GeometryManager singleton
     */
    static GeometryManager& instance();

    // -------------------------------------------------------------------------
    // Entity Lookup (Thread-safe reads)
    // -------------------------------------------------------------------------

    /**
     * @brief Find entity by global ID
     * @param id Global EntityId
     * @return Entity pointer, or nullptr if not found
     * @note Thread-safe
     */
    [[nodiscard]] GeometryEntityPtr findById(EntityId id) const;

    /**
     * @brief Find entity by type and UID
     * @param type Entity type
     * @param uid Type-scoped UID
     * @return Entity pointer, or nullptr if not found
     * @note Thread-safe
     */
    [[nodiscard]] GeometryEntityPtr findByTypeAndUID(EntityType type, EntityUID uid) const;

    /**
     * @brief Find entity by OCC shape
     * @param shape TopoDS_Shape to look up
     * @return Entity pointer, or nullptr if not found
     * @note Thread-safe. Uses shape's TShape pointer for lookup.
     */
    [[nodiscard]] GeometryEntityPtr findByShape(const TopoDS_Shape& shape) const;

    // -------------------------------------------------------------------------
    // Bulk Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get all entities of a specific type
     * @param type Entity type to filter by
     * @return Vector of matching entities
     */
    [[nodiscard]] std::vector<GeometryEntityPtr> getEntitiesByType(EntityType type) const;

    /**
     * @brief Get total number of registered entities
     * @return Entity count
     */
    [[nodiscard]] size_t entityCount() const;

    /**
     * @brief Get count of entities of a specific type
     * @param type Entity type
     * @return Count of entities with that type
     */
    [[nodiscard]] size_t entityCountByType(EntityType type) const;

    // -------------------------------------------------------------------------
    // Entity Registration
    // -------------------------------------------------------------------------

    /**
     * @brief Register an entity with the global index
     * @param entity Entity to register
     * @note Not thread-safe, requires external synchronization
     */
    void registerEntity(const GeometryEntityPtr& entity);

    /**
     * @brief Unregister an entity from the global index
     * @param entity Entity to unregister
     * @return true if entity was found and removed
     * @note Not thread-safe, requires external synchronization
     */
    bool unregisterEntity(const GeometryEntityPtr& entity);

    /**
     * @brief Clear all registered entities
     * @note Not thread-safe, requires external synchronization
     */
    void clear();

    // -------------------------------------------------------------------------
    // Document Management
    // -------------------------------------------------------------------------

    /**
     * @brief Get the active document
     * @return Active document pointer, or nullptr if none
     */
    [[nodiscard]] GeometryDocumentPtr activeDocument() const { return m_activeDocument; }

    /**
     * @brief Set the active document
     * @param document New active document
     */
    void setActiveDocument(const GeometryDocumentPtr& document) { m_activeDocument = document; }

    /**
     * @brief Create a new document and set as active
     * @param name Document name
     * @return New document pointer
     */
    GeometryDocumentPtr createDocument(const std::string& name = "Untitled");

    /**
     * @brief Get all managed documents
     * @return Vector of document pointers
     */
    [[nodiscard]] const std::vector<GeometryDocumentPtr>& documents() const { return m_documents; }

    // -------------------------------------------------------------------------
    // Topology Building (Convenience)
    // -------------------------------------------------------------------------

    /**
     * @brief Build topology hierarchy and register all entities
     * @param shape Root OCC shape
     * @param name Display name for the root entity
     * @return Root entity with complete hierarchy
     *
     * Convenience method that:
     * 1. Creates document if none active
     * 2. Builds topology hierarchy
     * 3. Registers all entities with global index
     */
    GeometryEntityPtr buildTopologyHierarchy(const TopoDS_Shape& shape,
                                             const std::string& name = "");

private:
    GeometryManager() = default;
    ~GeometryManager() = default;

    /**
     * @brief Recursively register entity and all children
     * @param entity Entity to register
     */
    void registerEntityRecursive(const GeometryEntityPtr& entity);

    /**
     * @brief Recursively unregister entity and all children
     * @param entity Entity to unregister
     */
    void unregisterEntityRecursive(const GeometryEntityPtr& entity);

private:
    mutable std::mutex m_mutex;                   ///< Mutex for thread-safe reads
    EntityIndex m_globalIndex;                    ///< Global entity index
    GeometryDocumentPtr m_activeDocument;         ///< Active document
    std::vector<GeometryDocumentPtr> m_documents; ///< All managed documents
};

} // namespace OpenGeoLab::Geometry
