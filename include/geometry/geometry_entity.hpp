/**
 * @file geometry_entity.hpp
 * @brief Geometry entity management with OCC integration
 *
 * This file provides the core geometry entity classes that wrap OpenCASCADE
 * topological shapes and provide ID-based entity management. The entity system
 * supports both global EntityId and type-scoped EntityUID for flexible querying.
 */

#pragma once

#include "geometry_types.hpp"

#include <kangaroo/util/noncopyable.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

class TopoDS_Shape;

namespace OpenGeoLab::Geometry {
class GeometryEntity;
class GeometryManager;
class GeometryDocument;

using GeometryEntityPtr = std::shared_ptr<GeometryEntity>;
using GeometryEntityWeakPtr = std::weak_ptr<GeometryEntity>;

// =============================================================================
// GeometryEntity
// =============================================================================

/**
 * @brief Base class for all geometry entities wrapping OCC shapes
 *
 * GeometryEntity provides:
 * - Dual ID system (EntityId for global, EntityUID for type-scoped)
 * - OCC TopoDS_Shape storage and access
 * - Bounding box computation
 * - Parent-child relationships for topology hierarchy
 *
 * Thread-safety: Read operations are thread-safe. Modifications should be
 * synchronized externally.
 */
class GeometryEntity : public std::enable_shared_from_this<GeometryEntity>,
                       public Kangaroo::Util::NonCopyMoveable {
public:
    /**
     * @brief Destructor.
     *
     * Ensures parent/child edges are detached in a best-effort manner.
     *
     * @note Edges are normally detached eagerly by EntityIndex/GeometryDocument
     *       on entity removal. This destructor acts as a defensive fallback
     *       for cases where an entity outlives its document via external
     *       shared ownership.
     */
    virtual ~GeometryEntity();
    // -------------------------------------------------------------------------
    // Type Information
    // -------------------------------------------------------------------------

    /**
     * @brief Get the entity type
     * @return EntityType enumeration value
     */
    [[nodiscard]] virtual EntityType entityType() const = 0;

    /**
     * @brief Get the type name as string
     * @return Human-readable type name
     */
    [[nodiscard]] virtual const char* typeName() const = 0;

    // -------------------------------------------------------------------------
    // ID Accessors
    // -------------------------------------------------------------------------

    /**
     * @brief Get the global unique entity ID
     * @return Globally unique EntityId
     */
    [[nodiscard]] EntityId entityId() const { return m_entityId; }

    /**
     * @brief Get the type-scoped unique ID
     * @return EntityUID unique within this entity type
     */
    [[nodiscard]] EntityUID entityUID() const { return m_entityUID; }

    // -------------------------------------------------------------------------
    // Shape Accessors
    // -------------------------------------------------------------------------

    /**
     * @brief Get the underlying OCC shape
     * @return Const reference to TopoDS_Shape
     */
    [[nodiscard]] virtual const TopoDS_Shape& shape() const = 0;

    /**
     * @brief Check if entity has a valid shape
     * @return true if shape is not null
     */
    [[nodiscard]] bool hasShape() const;

    // -------------------------------------------------------------------------
    // Geometry Properties
    // -------------------------------------------------------------------------

    /**
     * @brief Compute or get cached bounding box
     * @return Axis-aligned bounding box
     */
    [[nodiscard]] BoundingBox3D boundingBox() const;

    /**
     * @brief Check if bounding box has been computed
     * @return true if bounding box is available
     */
    [[nodiscard]] bool hasBoundingBox() const { return m_boundingBoxValid; }

    /**
     * @brief Invalidate cached bounding box (force recomputation)
     */
    void invalidateBoundingBox();

    // -------------------------------------------------------------------------
    // Hierarchy Management
    // -------------------------------------------------------------------------

    /**
     * @brief Check whether a parent->child edge is allowed by type.
     * @param child_type Type of the prospective child.
     * @return true if this entity is allowed to have a child of the given type.
     * @note This is a pure type-level constraint. Document presence and entity
     *       existence are validated by GeometryDocument when creating edges.
     */
    [[nodiscard]] virtual bool canAddChildType(EntityType child_type) const = 0;

    /**
     * @brief Check whether a parent->child edge is allowed by type.
     * @param parent_type Type of the prospective parent.
     * @return true if this entity is allowed to have a parent of the given type.
     */
    [[nodiscard]] virtual bool canAddParentType(EntityType parent_type) const = 0;

    /**
     * @brief Get one parent entity (if any)
     *
     * Note: This entity may have multiple parents. This accessor returns an arbitrary
     * valid parent (if exists) and auto-prunes expired references.
     */
    /// Return any valid parent (if exists). Use when multiple parents are acceptable.
    [[nodiscard]] GeometryEntityWeakPtr anyParent() const;

    /// Return a valid parent only if it is unique; otherwise empty.
    [[nodiscard]] GeometryEntityWeakPtr singleParent() const;

    /// Get all parent entities (auto-prunes expired references).
    [[nodiscard]] std::vector<GeometryEntityPtr> parents() const;

    /// Get child entities (auto-prunes expired references).
    [[nodiscard]] std::vector<GeometryEntityPtr> children() const;

    /// O(1) membership check (does not resolve).
    [[nodiscard]] bool hasParentId(EntityId parent_id) const;

    /// O(1) membership check (does not resolve).
    [[nodiscard]] bool hasChildId(EntityId child_id) const;

    /// True if has no valid parents (auto-prunes expired references).
    [[nodiscard]] bool isRoot() const;

    /// True if has at least one valid child (auto-prunes expired references).
    [[nodiscard]] bool hasChildren() const;

    /// Count valid parents (auto-prunes expired references).
    [[nodiscard]] size_t parentCount() const;

    /// Count valid children (auto-prunes expired references).
    [[nodiscard]] size_t childCount() const;

    /**
     * @brief Add a child edge (this -> child), automatically syncing both sides.
     *
     * Fails on:
     * - self-parent/self-child
     * - relationship type constraints
     * - missing EntityIndex / missing entities
     */
    [[nodiscard]] bool addChild(EntityId child_id);

    /**
     * @brief Remove a child edge (this -> child), automatically syncing both sides.
     */
    [[nodiscard]] bool removeChild(EntityId child_id);

    /// Convenience overload (does not store the pointer; stores only EntityId).
    [[nodiscard]] bool addChild(const GeometryEntityPtr& child);

    /// Convenience overload (does not store the pointer; stores only EntityId).
    [[nodiscard]] bool removeChild(const GeometryEntityPtr& child);

    /**
     * @brief Add a parent edge (parent -> this), automatically syncing both sides.
     */
    [[nodiscard]] bool addParent(EntityId parent_id);

    /**
     * @brief Remove a parent edge (parent -> this), automatically syncing both sides.
     */
    [[nodiscard]] bool removeParent(EntityId parent_id);

    /// Visit valid children; auto-filters invalid and self-cleans.
    void visitChildren(const std::function<void(const GeometryEntityPtr&)>& visitor) const;

    /// Visit valid parents; auto-filters invalid and self-cleans.
    void visitParents(const std::function<void(const GeometryEntityPtr&)>& visitor) const;

    /**
     * @brief Remove expired parent/child ids.
     *
     * @warning This performs lookups against the owning GeometryDocument and may
     *          be expensive if called frequently on large graphs.
     * @note With correct lifetime management (edges detached on entity removal),
     *       relations should remain consistent and this should rarely be needed.
     */
    void pruneExpiredRelations() const;
    // -------------------------------------------------------------------------
    // Name/Label
    // -------------------------------------------------------------------------

    /**
     * @brief Get entity display name
     * @return User-visible name string
     */
    [[nodiscard]] const std::string& name() const { return m_name; }

    /**
     * @brief Set entity display name
     * @param name New display name
     */
    void setName(const std::string& name) { m_name = name; }

protected:
    /**
     * @brief Protected constructor for derived classes
     * @param type Entity type for UID generation
     */
    explicit GeometryEntity(EntityType type);

    /**
     * @brief Compute bounding box from OCC shape
     */
    void computeBoundingBox() const;

    /**
     * @brief Detect entity type from OCC shape
     * @param shape Shape to analyze
     * @return Detected EntityType
     */
    [[nodiscard]] static EntityType detectEntityType(const TopoDS_Shape& shape);

protected:
    EntityId m_entityId{INVALID_ENTITY_ID};    ///< Global unique ID
    EntityUID m_entityUID{INVALID_ENTITY_UID}; ///< Type-scoped unique ID

    mutable BoundingBox3D m_boundingBox;    ///< Cached bounding box
    mutable bool m_boundingBoxValid{false}; ///< Bounding box validity

    friend class EntityIndex;
    friend class GeometryDocument;

    // Set/clear by EntityIndex on add/remove; non-owning.
    void setDocument(std::weak_ptr<GeometryDocument> document) { m_document = std::move(document); }
    [[nodiscard]] std::shared_ptr<GeometryDocument> document() const { return m_document.lock(); }

    // Relationship internals (do not sync both sides)
    [[nodiscard]] bool addChildNoSync(EntityId child_id);
    [[nodiscard]] bool removeChildNoSync(EntityId child_id);
    [[nodiscard]] bool addParentNoSync(EntityId parent_id);
    [[nodiscard]] bool removeParentNoSync(EntityId parent_id);

    // Called by EntityIndex before the entity is removed, to eagerly detach edges.
    void detachAllRelations();

    std::weak_ptr<GeometryDocument> m_document;
    mutable std::unordered_set<EntityId> m_parentIds;
    mutable std::unordered_set<EntityId> m_childIds;

    std::string m_name; ///< Display name
};

} // namespace OpenGeoLab::Geometry