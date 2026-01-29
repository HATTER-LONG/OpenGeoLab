/**
 * @file geometry_document.hpp
 * @brief Geometry document container for entity management
 *
 * GeometryDocument is the primary container for geometry entities within
 * the application. Each document represents an independent model or assembly
 * with its own entity index and relationship graph.
 *
 * Features:
 * - PartEntity-based hierarchy with full sub-shape relationships
 * - Render data generation for OpenGL visualization
 * - Mesh metadata extraction for mesh generation
 * - Support for appending shapes and creating new models
 */

#pragma once

#include <kangaroo/util/noncopyable.hpp>

#include "geometry/entity_index.hpp"
#include "geometry/mesh_metadata.hpp"
#include "geometry/part_entity.hpp"
#include "geometry/render_data.hpp"

#include <TopoDS_Shape.hxx>

#include <functional>
#include <memory>
#include <string>

namespace OpenGeoLab::Geometry {

// Forward declarations
class ShapeBuilder;

using GeometryDocumentPtr = std::shared_ptr<GeometryDocument>;

/**
 * @brief Progress callback for document operations
 * @param progress Current progress [0, 1]
 * @param message Status message
 * @return false to cancel, true to continue
 */
using DocumentProgressCallback = std::function<bool(double progress, const std::string& message)>;

/**
 * @brief Geometry document holding the authoritative entity index.
 *
 * The document is the only owner and user of EntityIndex. Entities keep a
 * weak back-reference to the document for relationship resolution.
 *
 * Key capabilities:
 * - Part-based entity hierarchy management
 * - Render data generation for OpenGL visualization
 * - Mesh metadata extraction for mesh generation
 * - Shape import and primitive creation
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

    // =========================================================================
    // Part Management
    // =========================================================================

    /**
     * @brief Get all root part entities in the document
     * @return Vector of part entity pointers
     */
    [[nodiscard]] std::vector<PartEntityPtr> getAllParts() const;

    /**
     * @brief Get part count in the document
     * @return Number of part entities
     */
    [[nodiscard]] size_t partCount() const;

    /**
     * @brief Find a part by name
     * @param name Part name to search for
     * @return Part entity if found, nullptr otherwise
     */
    [[nodiscard]] PartEntityPtr findPartByName(const std::string& name) const;

    /**
     * @brief Append a shape to the document as a new Part
     * @param shape The OCC shape to add
     * @param part_name Name for the new part (auto-generated if empty)
     * @param progress_callback Optional progress callback
     * @return Created part entity, or nullptr on failure
     *
     * This method creates a complete entity hierarchy from the shape,
     * including all sub-shapes (solids, faces, edges, vertices).
     */
    [[nodiscard]] PartEntityPtr appendShape(const TopoDS_Shape& shape,
                                            const std::string& part_name = "",
                                            DocumentProgressCallback progress_callback = nullptr);

    /**
     * @brief Create a box primitive and add to document
     * @param dx Box dimension in X
     * @param dy Box dimension in Y
     * @param dz Box dimension in Z
     * @param part_name Name for the new part
     * @return Created part entity, or nullptr on failure
     */
    [[nodiscard]] PartEntityPtr
    createBox(double dx, double dy, double dz, const std::string& part_name = "Box");

    /**
     * @brief Create a sphere primitive and add to document
     * @param radius Sphere radius
     * @param part_name Name for the new part
     * @return Created part entity, or nullptr on failure
     */
    [[nodiscard]] PartEntityPtr createSphere(double radius,
                                             const std::string& part_name = "Sphere");

    /**
     * @brief Create a cylinder primitive and add to document
     * @param radius Cylinder radius
     * @param height Cylinder height
     * @param part_name Name for the new part
     * @return Created part entity, or nullptr on failure
     */
    [[nodiscard]] PartEntityPtr
    createCylinder(double radius, double height, const std::string& part_name = "Cylinder");

    /**
     * @brief Create a cone primitive and add to document
     * @param radius1 Bottom radius
     * @param radius2 Top radius
     * @param height Cone height
     * @param part_name Name for the new part
     * @return Created part entity, or nullptr on failure
     */
    [[nodiscard]] PartEntityPtr createCone(double radius1,
                                           double radius2,
                                           double height,
                                           const std::string& part_name = "Cone");

    // =========================================================================
    // Render Data Generation
    // =========================================================================

    /**
     * @brief Generate render data for all parts in the document
     * @param params Tessellation parameters
     * @param progress_callback Optional progress callback
     * @return Complete document render data
     */
    [[nodiscard]] DocumentRenderDataPtr
    generateRenderData(const TessellationParams& params = TessellationParams(),
                       DocumentProgressCallback progress_callback = nullptr);

    /**
     * @brief Generate render data for a specific part
     * @param part_id Part entity ID
     * @param params Tessellation parameters
     * @return Part render data, or nullptr if part not found
     */
    [[nodiscard]] PartRenderDataPtr
    generatePartRenderData(EntityId part_id,
                           const TessellationParams& params = TessellationParams());

    // =========================================================================
    // Mesh Metadata Generation
    // =========================================================================

    /**
     * @brief Generate mesh metadata for all parts in the document
     * @param progress_callback Optional progress callback
     * @return Complete document mesh metadata
     */
    [[nodiscard]] DocumentMeshMetadataPtr
    generateMeshMetadata(DocumentProgressCallback progress_callback = nullptr);

    /**
     * @brief Generate mesh metadata for a specific part
     * @param part_id Part entity ID
     * @return Part mesh metadata, or nullptr if part not found
     */
    [[nodiscard]] PartMeshMetadataPtr generatePartMeshMetadata(EntityId part_id);

    // =========================================================================
    // Document Bounding Box
    // =========================================================================

    /**
     * @brief Get the combined bounding box of all parts
     * @return Scene bounding box
     */
    [[nodiscard]] BoundingBox3D sceneBoundingBox() const;

private:
    GeometryDocument() = default;

    /**
     * @brief Generate a unique part name
     * @param base_name Base name for the part
     * @return Unique name (with suffix if needed)
     */
    [[nodiscard]] std::string generateUniquePartName(const std::string& base_name);

private:
    EntityIndex m_entityIndex;
    size_t m_partNameCounter{0}; ///< Counter for auto-generating part names
};

class GeometryDocumentManager : public Kangaroo::Util::NonCopyMoveable {
public:
    ~GeometryDocumentManager() = default;
    static GeometryDocumentManager& instance();

    /**
     * @brief Get the current active document
     * @return Current document (creates one if none exists)
     */
    [[nodiscard]] GeometryDocumentPtr currentDocument();

    /**
     * @brief Create a new empty document and set as current
     * @return The new document
     *
     * This clears the previous document and creates a fresh one.
     * Use this to implement "New Model" functionality.
     */
    [[nodiscard]] GeometryDocumentPtr newDocument();

    /**
     * @brief Check if a document exists
     * @return true if there is a current document
     */
    [[nodiscard]] bool hasDocument() const { return m_currentDocument != nullptr; }

    /**
     * @brief Get total part count across the current document
     * @return Part count, 0 if no document
     */
    [[nodiscard]] size_t partCount() const;

    /**
     * @brief Clear the current document (remove all entities)
     *
     * This keeps the document but removes all content.
     */
    void clearCurrentDocument();

protected:
    GeometryDocumentManager() = default;

private:
    GeometryDocumentPtr m_currentDocument;
};

} // namespace OpenGeoLab::Geometry