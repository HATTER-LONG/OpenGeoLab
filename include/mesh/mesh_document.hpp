/**
 * @file mesh_document.hpp
 * @brief Abstract mesh document interface for node and element management
 *
 * MeshDocument is the primary container for FEM mesh data. It stores
 * mesh nodes and elements, supports queries by various keys, and
 * provides render data for visualization.
 */

#pragma once

#include "mesh/mesh_element.hpp"
#include "mesh/mesh_node.hpp"
#include "render/render_data.hpp"

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>
#include <memory>
#include <vector>

namespace OpenGeoLab::Mesh {

class MeshDocument;
using MeshDocumentPtr = std::shared_ptr<MeshDocument>;

/**
 * @brief Abstract mesh document interface
 *
 * A mesh document stores FEM mesh nodes and elements. Implementations
 * provide storage, queries by MeshElementKey/MeshElementRef, render data
 * generation, and association with source geometry entities.
 */
class MeshDocument : public Kangaroo::Util::NonCopyable {
public:
    MeshDocument() = default;
    virtual ~MeshDocument() = default;

    // -------------------------------------------------------------------------
    // Node Management
    // -------------------------------------------------------------------------

    /**
     * @brief Add a node to the document.
     * @param node Node to add (copied).
     * @return true if added; false on duplicate id.
     */
    virtual bool addNode(const MeshNode& node) = 0;

    /**
     * @brief Find a node by id.
     * @param node_id Node id to find.
     * @return Pointer to the node, or nullptr if not found.
     */
    [[nodiscard]] virtual const MeshNode* findNodeById(MeshNodeId node_id) const = 0;

    /**
     * @brief Get total node count.
     */
    [[nodiscard]] virtual size_t nodeCount() const = 0;

    // -------------------------------------------------------------------------
    // Element Management
    // -------------------------------------------------------------------------

    /**
     * @brief Add an element to the document.
     * @param element Element to add (moved).
     * @return true if added; false on duplicate id.
     */
    virtual bool addElement(MeshElement element) = 0;

    /**
     * @brief Find an element by global id.
     * @param element_id Global element id.
     * @return Pointer to the element, or nullptr if not found.
     */
    [[nodiscard]] virtual const MeshElement* findElementById(MeshElementId element_id) const = 0;

    /**
     * @brief Find an element by (uid, type) reference.
     * @param ref Element reference with uid + type.
     * @return Pointer to the element, or nullptr if not found.
     */
    [[nodiscard]] virtual const MeshElement* findElementByRef(const MeshElementRef& ref) const = 0;

    /**
     * @brief Get total element count.
     */
    [[nodiscard]] virtual size_t elementCount() const = 0;

    /**
     * @brief Get all elements of a specific type.
     * @param type Element type to filter by.
     * @return Vector of pointers to matching elements.
     */
    [[nodiscard]] virtual std::vector<const MeshElement*>
    elementsByType(MeshElementType type) const = 0;

    // -------------------------------------------------------------------------
    // Clear
    // -------------------------------------------------------------------------

    /**
     * @brief Clear all nodes and elements.
     */
    virtual void clear() = 0;

    // -------------------------------------------------------------------------
    // Render Data
    // -------------------------------------------------------------------------

    /**
     * @brief Generate render data for mesh visualization.
     * @return Render data containing mesh element and node meshes.
     *
     * @note The returned data includes wireframe edges for mesh elements
     *       and point data for mesh nodes. Mesh elements use EntityType::MeshElement
     *       and nodes use EntityType::MeshNode for picking compatibility.
     */
    [[nodiscard]] virtual Render::DocumentRenderData getRenderData() const = 0;
};

/**
 * @brief Singleton factory interface for MeshDocument
 */
class MeshDocumentSingletonFactory
    : public Kangaroo::Util::FactoryTraits<MeshDocumentSingletonFactory, MeshDocument> {
public:
    MeshDocumentSingletonFactory() = default;
    virtual ~MeshDocumentSingletonFactory() = default;

    virtual tObjectSharedPtr instance() const = 0;
};

} // namespace OpenGeoLab::Mesh

#define MeshDocumentInstance                                                                       \
    g_ComponentFactory.getInstanceObject<Mesh::MeshDocumentSingletonFactory>()
