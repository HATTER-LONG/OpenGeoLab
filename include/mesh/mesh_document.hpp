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
#include "util/signal.hpp"

#include <kangaroo/util/component_factory.hpp>
#include <kangaroo/util/noncopyable.hpp>
#include <memory>

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
     * @param node Node to add (moved).
     * @return true if added; false on duplicate id.
     */
    virtual bool addNode(MeshNode node) = 0;

    /**
     * @brief Find a node by id.
     * @param node_id Node id to find.
     * @return Copy of the node, or throws if not found.
     */
    [[nodiscard]] virtual MeshNode findNodeById(MeshNodeId node_id) const = 0;

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
     * @return Copy of the element, or throws if not found.
     */
    [[nodiscard]] virtual MeshElement findElementById(MeshElementId element_id) const = 0;

    /**
     * @brief Find an element by (uid, type) reference.
     * @param ref Element reference with uid + type.
     * @return Copy of the element, or throws if not found.
     */
    [[nodiscard]] virtual MeshElement findElementByRef(const MeshElementRef& ref) const = 0;

    /**
     * @brief Get total element count.
     * @return Total number of elements in the document.
     */
    [[nodiscard]] virtual size_t elementCount() const = 0;

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

    [[nodiscard]] virtual const Render::RenderData& getRenderData() = 0;

    // -------------------------------------------------------------------------
    // Change Notification
    // -------------------------------------------------------------------------

    /**
     * @brief Subscribe to mesh data changes.
     * @param callback Callback executed when mesh data changes.
     * @return Scoped connection that disconnects on destruction.
     */
    [[nodiscard]] virtual Util::ScopedConnection
    subscribeToChanges(std::function<void()> callback) = 0;

    /**
     * @brief Notify that mesh data has changed.
     *
     * Call after bulk operations (e.g., mesh generation) to trigger
     * render updates. Individual addNode/addElement calls should NOT
     * trigger this; callers should call notifyChanged() explicitly
     * after completing bulk operations.
     */
    virtual void notifyChanged() = 0;
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
