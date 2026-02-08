/**
 * @file mesh_document.hpp
 * @brief Mesh document interface for mesh data management and rendering
 *
 * Defines the public interface for mesh documents that store mesh nodes
 * and elements, map them to geometry entities, and produce render data
 * for visualization.
 */

#pragma once

#include "mesh/mesh_types.hpp"
#include "render/render_data.hpp"

#include <kangaroo/util/noncopyable.hpp>
#include <memory>
#include <vector>

namespace OpenGeoLab::Mesh {

class MeshDocument;
using MeshDocumentPtr = std::shared_ptr<MeshDocument>;

/**
 * @brief Abstract mesh document interface
 *
 * A mesh document manages mesh nodes and elements generated from
 * geometry entities. It provides:
 * - Storage and lookup for nodes and elements by id/uid
 * - Mapping between mesh entities and geometry entities
 * - Render data generation for OpenGL visualization
 */
class MeshDocument : public Kangaroo::Util::NonCopyMoveable {
public:
    MeshDocument() = default;
    virtual ~MeshDocument() = default;

    // -------------------------------------------------------------------------
    // Node Management
    // -------------------------------------------------------------------------

    /**
     * @brief Add a mesh node
     * @param node Node to add
     * @return true if added successfully
     */
    virtual bool addNode(const MeshNode& node) = 0;

    /**
     * @brief Find a node by its global ID
     * @param id Global node ID
     * @return Pointer to the node, or nullptr
     */
    [[nodiscard]] virtual const MeshNode* findNodeById(MeshElementId id) const = 0;

    /**
     * @brief Find a node by its type-scoped UID
     * @param uid Node UID
     * @return Pointer to the node, or nullptr
     */
    [[nodiscard]] virtual const MeshNode* findNodeByUID(MeshElementUID uid) const = 0;

    /**
     * @brief Get total number of nodes
     */
    [[nodiscard]] virtual size_t nodeCount() const = 0;

    /**
     * @brief Get all nodes
     */
    [[nodiscard]] virtual std::vector<MeshNode> allNodes() const = 0;

    // -------------------------------------------------------------------------
    // Element Management
    // -------------------------------------------------------------------------

    /**
     * @brief Add a mesh element
     * @param element Element to add
     * @return true if added successfully
     */
    virtual bool addElement(const MeshElement& element) = 0;

    /**
     * @brief Find an element by its global ID
     * @param id Global element ID
     * @return Pointer to the element, or nullptr
     */
    [[nodiscard]] virtual const MeshElement* findElementById(MeshElementId id) const = 0;

    /**
     * @brief Find an element by its type-scoped UID
     * @param uid Element UID
     * @param type Element type
     * @return Pointer to the element, or nullptr
     */
    [[nodiscard]] virtual const MeshElement* findElementByUID(MeshElementUID uid,
                                                              MeshEntityType type) const = 0;

    /**
     * @brief Get total number of elements
     */
    [[nodiscard]] virtual size_t elementCount() const = 0;

    /**
     * @brief Get all elements
     */
    [[nodiscard]] virtual std::vector<MeshElement> allElements() const = 0;

    /**
     * @brief Get elements of a specific type
     * @param type Element type to filter by
     */
    [[nodiscard]] virtual std::vector<MeshElement> elementsByType(MeshEntityType type) const = 0;

    // -------------------------------------------------------------------------
    // Geometry-Mesh Mapping
    // -------------------------------------------------------------------------

    /**
     * @brief Find all mesh elements associated with a geometry entity
     * @param geo_key Geometry entity key
     * @return Vector of element keys on the given geometry entity
     */
    [[nodiscard]] virtual std::vector<MeshElementKey>
    findElementsByGeometry(const Geometry::EntityKey& geo_key) const = 0;

    /**
     * @brief Find all mesh nodes associated with a geometry entity
     * @param geo_key Geometry entity key
     * @return Vector of node keys on the given geometry entity
     */
    [[nodiscard]] virtual std::vector<MeshElementKey>
    findNodesByGeometry(const Geometry::EntityKey& geo_key) const = 0;

    /**
     * @brief Get the geometry entity key for a mesh element
     * @param elem_uid Element UID
     * @param elem_type Element type
     * @return Geometry entity key (may be invalid if not mapped)
     */
    [[nodiscard]] virtual Geometry::EntityKey
    geometryForElement(MeshElementUID elem_uid, MeshEntityType elem_type) const = 0;

    /**
     * @brief Get the geometry entity key for a mesh node
     * @param node_uid Node UID
     * @return Geometry entity key (may be invalid if not mapped)
     */
    [[nodiscard]] virtual Geometry::EntityKey geometryForNode(MeshElementUID node_uid) const = 0;

    // -------------------------------------------------------------------------
    // Render Data
    // -------------------------------------------------------------------------

    /**
     * @brief Get render data for OpenGL visualization of the mesh
     * @return Complete render data for mesh display
     *
     * @note Produces triangulated faces, wireframe edges, and node points
     *       suitable for direct GPU upload.
     */
    [[nodiscard]] virtual Render::DocumentRenderData getMeshRenderData() const = 0;

    // -------------------------------------------------------------------------
    // Document Operations
    // -------------------------------------------------------------------------

    /**
     * @brief Clear all mesh data
     */
    virtual void clear() = 0;

    /**
     * @brief Check if the document is empty
     */
    [[nodiscard]] virtual bool isEmpty() const = 0;
};

} // namespace OpenGeoLab::Mesh
